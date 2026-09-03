# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware for an **industrial thawing-room controller** running on an ESP32-S3 (Heltec WiFi LoRa 32 V3), built with PlatformIO / arduino-esp32 3.x (ESP-IDF 5). It cycles a reversible fan (F1), a sprinkler valve (S1), an air damper and a PID-driven air PWM through three sequential stages to thaw product, supervised over MQTT and a local web UI.

`README.md` is detailed and current — read it for the full rationale behind the reliability design, the TLS setup, and the chronological "Stability Improvements Log".

### Physical context (not derivable from the code)

Thaws **sardine and mackerel**. Plant is in **Taiping, Malaysia**; development happens in **Ensenada, Mexico** — hence `config_TAIPING.txt` / `config_ENSENADA.txt` and the UTC+8 default with UTC-7 samples. **Target: product ends between -3 °C and +2 °C.** There are **9 rooms** on one broker.

| Probe | What it measures |
|---|---|
| `Ta` | room air |
| `Tc` | **core** — physically driven into a perforated fish |
| `Ts` | **surface** — under one of the fish plates |
| `Ti` | **dead code.** Old infeed-temperature idea; never assigned, publishes a constant 0 |

All probes are **PT-100 with a 4-20 mA transmitter integrated in the M12 connector** (Turck ref. 9910550, span -50…+50 °C, no adjustment possible). The DS18B20/OneWire path and the `DeviceAddress` constants in `Controller.h` are **legacy** — OneWire was abandoned over wiring and lifespan problems. The MLX IR sensor is **out of service** (it ended up tracking `Ta` instead of `Ts`).

Actuation: the room has blowers and fans. **The PID drives the blowers' air infeed via PWM**; `coef_pid_fwd` is for air coming in, `coef_pid_rev` for when flow is reversed to *pull air out* of the room, which is sometimes needed to move product temperature. There are no VFDs — the blower is DC accepting PWM, the fans run on contactors and interlocks.

**S1 (sprinklers) is not part of thermal control** — it only flushes the blood draining off the fish carts. That is why stage 3's ratio (1 min on / 15 off) looks nothing like stage 2's (1/1). It is hygiene, not process.

Telemetry is consumed by an **ECAVA SCADA**.

## Commands

```bash
pio run                      # build firmware (default env heltec_wifi_lora_32_V3)
pio run -t upload            # flash the board (port autodetected — do not hardcode)
pio device monitor           # 115200, esp32_exception_decoder filter enabled
pio run -t uploadfs          # upload data/ to SPIFFS (rarely needed; config is editable via web UI)

pio test -e native           # native ConfigStore unit tests — runs on the PC, no board
pio test -e native -f test_configstore   # single test suite
```

CI (`.github/workflows/build.yml`) runs `platformio run` only — tests are not in CI.

Certificates (see `certs/README.md`):
```bash
cd certs && ./gen-certs.sh <broker-ip-or-hostname> && ./embed-ca.sh
```
`embed-ca.sh` regenerates `src/mqtt_certs.h`. The CA is created once and reused; re-running `gen-certs.sh` only re-issues the server cert.

## Architecture

### Execution model — two cores, one control loop

- **Core 1 (`loop()` in `src/Thawing-room-chicano.cpp`)** owns the process: reads sensors, services `TaskScheduler` (`runner.execute()`), handles buttons, and runs the stage state machine. It is fed to a **task watchdog** (`WDT_TIMEOUT_MS` 15 s, armed at the *end* of `setup()`) — anything that can block this loop for longer will reboot the device.
- **Core 0 (`backgroundTasks`, pinned via `xTaskCreatePinnedToCore`)** owns connectivity only: WiFi reconnect, OTA, and the captive-DNS pump in AP mode. The WDT deliberately does **not** watch idle cores so legitimate WiFi/OTA blocking doesn't trip it.
- Web-server handlers run in yet another task. **This is why `ConfigStore` guards `read()`/`write()` with a recursive mutex** — async handlers write SPIFFS while `loop()` reads it.

`loop()` must stay non-blocking. `delay()` in stage code is a bug; the codebase uses `hasIntervalPassed(timer, interval, to_min)` and non-blocking init guards (`timers.stageN.init_delay`) instead.

### Stage state machine

`IDLE → STAGE1 (delayed start) → STAGE2 (active thaw) → STAGE3 (hold) → IDLE`, driven by `handleStage()` → `handleStage1/2/3()`. `Stage` (`src/Stage.h`) is a thin step counter with init/destroy callbacks; the actual logic lives in the `handleStageN()` step branches, all of which follow the same shape:

```
step 0 → init          step 1 → F1 forward + PID    step 2 → F1 off
step 3 → F1 reverse + PID (coef_pid_rev)            step 4 → F1 off → back to step 1
```

Sprinkler cycling runs orthogonally to the fan steps via `asyncLoopSprinkler()`.

Transitions:
- **STAGE1 → STAGE2**: `controller.hasStage2TimeElapsed(now)` — a Unix epoch, not an hour/minute compare (the old version overflowed across midnight).
- **STAGE2 → STAGE3**: `TS_F >= temp_set.ts && TC_F >= temp_set.tc`.
- **STAGE3 has no automatic end, by design** — product stays thawing until production presses STOP as the fish leaves the room.
- In STAGE2/3, the PID setpoint is recomputed from the room parameters as `Setpoint = B - A * avg_ts`, PID input is `TA_F`, and the output is scaled by `coef_pid_fwd` / `coef_pid_rev` before hitting the air PWM.

#### Parameter loading fails silently — always check the effective values

`setUpDefaultParameters()` reads every key without reporting what it did or didn't find. Three traps follow, all verified in code (not assumptions about any particular deployed file — **get the live `defaultParameters.txt` verbatim before concluding anything about a given room**):

1. **A missing or misspelled key yields `0`, not a default.** Only `coef_pid_*` use `| 100`; everything else (`f1RevOntime`, all the fan/sprinkler times) is a bare assignment, so a typo silently means zero. For `fanRevONTime` that is quietly fatal: with a 0-minute interval `hasIntervalPassed()` returns true on the same iteration the reverse fan starts, and the next step shuts it off — it pulses for milliseconds instead of minutes. The README's fix #26 was this same class of bug.
2. **`| 100` rejects string values.** ArduinoJson 6's `operator|` tests `is<T>()`, which is type-strict and returns false for a JSON *string*. Every other parameter is read by plain assignment, which uses `as<T>()` and *does* parse strings — so `"1"` works everywhere except the two coefficients. `"coef_pid_fwd": "100"` (a string, as written in `samples/defaultParameters.sample.json`) is **silently discarded and forced to 100**. Only an unquoted number is honored. Saving from the device writes numbers, so this bites hand-edited and legacy files.
3. **The PID output range does not match the PWM.** There is no `SetOutputLimits()` call anywhere; the library defaults to 0-255 while `ledcAttach(AIR_PIN, FREQ, RESOLUTION)` uses `RESOLUTION 6` → 0-63. At `coef_pid_* = 100` anything above ~25 % is full blast, so the air loop is effectively bang-bang.

#### The control loop is bypassed on purpose — do not "fix" it

Read this before proposing any change to the PID, `SetOutputLimits`, or the stage setpoints. Plant staff stopped trusting the control layer: they run the air **full on** and switch stages by hand.

- **High `tsSet`/`tcSet`** (20/18 °C against product finishing at -3…+2 °C) are a **deliberate safety net**, not a misconfiguration. A saturated probe reads ~43 °C, which is *inside* the valid range and therefore accepted as a real reading, so a high setpoint makes a failed sensor **push the process forward** instead of stalling it.
- That net only covers **half** the failure modes. A low-side failure (open loop → 0 counts → out of range) makes `updateSensorReading()` hold the last good value, which sits far below the setpoint, so the cycle **stalls**. And the transition needs *both* probes, so one low-side failure defeats it.
- **"Full on" is emergent, not explicit.** It depends on three stacked accidents: `B = 70` being unreachable for the Ta channel, integral windup pinning `Output` at 255, and **`ledcWrite()` not clamping** an out-of-range duty (it only special-cases `duty == max_duty`; anything larger goes to the register and the comparator never fires, leaving the pin permanently high). An arduino-esp32 update that adds clamping would silently change plant behavior.

The real ordering is that **control was abandoned because measurement is untrustworthy**. Rebuild confidence in the sensors first (raw counts, distinguishing an open loop from a genuinely cold probe); only then is restoring the loop worth proposing.

**The stage2 epoch is persisted in two places** (`Controller::saveStage2StartTime` / `loadStage2StartTime`): `RTC_DATA_ATTR` RAM survives a watchdog reset, NVS `Preferences("stage2")` survives a power cut. Recovery order is RAM → flash → recalculate. `stopRoutine()` must clear it or the next cycle reuses a stale target.

### Modules

| File | Responsibility |
|---|---|
| `src/Thawing-room-chicano.cpp` | `setup()`/`loop()`, stage machine, MQTT `callback()` (every settable parameter), publishing |
| `src/hardware/Controller.cpp` | Everything hardware: GPIO, ADC, RTC/NTP/time, config loading, and a facade over `WIFI` |
| `src/hardware/WIFI.cpp` | STA connect, AP config portal, the whole AsyncWebServer (routes + form handling), OTA |
| `src/hardware/ConfigStore.cpp` | Atomic SPIFFS persistence — the only correct way to touch config files |
| `src/hardware/Logger.cpp` | Serial + optional SD logging |
| `src/MqttClient.cpp/.h` | PubSubClient wrapper + **all topic names as `#define`s, which are bare SUFFIXES** — the per-room prefix is prepended at runtime (see below) |
| `src/hardware/resources/WebFiles.cpp` | Web UI HTML/CSS/JS embedded in flash — no CDN (units run offline) |
| `include/config.h` | Pin map, ADC calibration slopes/offsets, AP settings, `EMBEDDED_DEFAULT_PARAMS` |
| `include/types.h` | All shared structs/enums (`StageState`, `stage_parameters`, `room_parameters`, `SystemTimers`) |

Board pin names (`PORT_A3`, `PORT_C0`, `AI_0`…) come from the vendored `WS_V2.h` in the `Wireless-Stick-Lite-V2` git dependency, not from this repo.

### MQTT topic prefix

The plant has **9 rooms on one broker**, so every topic is namespaced. The `#define`s in `MqttClient.h` are **suffixes only** (`"Ta"`, `"stop"`); `MqttClient` prepends the room's prefix in the three places it matters — `publishData()`, `subscribeRoutine()` and `isTopicEqual()` — composing into a reusable buffer rather than the heap. That buffer is not reentrant and doesn't need to be: all MQTT traffic runs on the loop task (core 1).

The prefix comes from the **`TOPIC` key in `config.txt`**, falls back to `DEFAULT_TOPIC_PREFIX` (`"cfpp4/"`) when absent so deployed units don't change topics on upgrade, and gets a trailing `/` appended if missing. Set it via the AP portal — **one binary now serves all 9 rooms**, which matters because deployment is manual.

When adding a topic, define it as a bare suffix and reach the broker only through `MqttClient`; never build a topic string at the call site.

### Logging

`Logger` has levels (`LOG_LVL_DEBUG…CRIT`), an explicit provenance tag, and printf formatting via `vsnprintf`. Use the macros — `LOG_W(TAG_ADC, "TC open loop: raw=%u", raw)` — not `char buf[N]; sprintf(...)`, which is the pattern that produced the buffer-overflow fix in `94f3ed3`.

The important structural rule: **`drain()` is the only code that touches the SD, and only `backgroundTasks` (core 0) calls it.** Logging happens from three tasks (control loop, background, web handlers), and the SD hangs off a shared SPI bus, so `log()` only formats, writes Serial, and enqueues; a single consumer does the I/O. Single-writer means no mutex is needed and the control loop never blocks on the card.

Two consequences worth preserving:

- **Nothing in the log path blocks.** The queue is posted with timeout 0 and drops on full (counted, reported once a minute) — stalling the control loop to save a log line would be the wrong trade. Serial writes check `availableForWrite()` first, because `uart_write_bytes()` waits for TX room *and* takes a global UART mutex.
- **The clock is pushed, not pulled.** `loop()` calls `logger.setClock(current_date)` with the `DateTime` it already reads. `drain()` timestamps from that snapshot, correcting for queue delay. It must not call `getDateTime()` itself: that would put I2C on the logging path and mutate `Controller`'s RTC state from two cores.

`LOG_SERIAL` in `config.txt` turns serial output off for plant units. There is no way to auto-detect a listener here — the board leaves `Serial` on UART0 (`ARDUINO_USB_CDC_ON_BOOT` is unset) and `HardwareSerial::operator bool()` is hardcoded to `return true`. The native USB CDC (`HWCDC`) *does* have real detection if the console ever moves there.

The old `println()`/`printError()` API now routes **through** the new pipeline, so the ~120 existing call sites already get tags, timestamps and level filtering without being rewritten. `println()` maps to **DEBUG on purpose**: with `sd_level` at INFO those calls still go only to Serial, exactly as before — no change in volume, no filling the card. What persists is what has been explicitly promoted to `LOG_I`/`LOG_W`/`LOG_E` (stage transitions, process stop, reset cause, sensor faults, time source). Drop `sd_level` to DEBUG to capture everything while diagnosing.

`print()` (no newline) stays raw Serial — it can't go through `log()`, which emits whole records.

### RTC and the time source

Three-layer fallback: DS3231 → ESP32 internal RTC → last valid time + `millis()` delta. Two things about it are easy to get wrong:

- **Every source stores LOCAL time**, and `getDateTime()` returns them without adding any offset. `TIME_ZONE_OFFSET_HRS` is applied in exactly one place: the `setTimeOffset()` on each NTP client. If you add a new time source, it must store local too. (The canonical design — UTC everywhere, offset at the return — is better, but deployed DS3231s hold local, so switching would jump every unit by the offset until its next NTP sync.)
- **The internal RTC runs off the internal RC oscillator** (`CONFIG_RTC_CLK_SRC_INT_RC=y`, no 32 kHz crystal on this board). It's calibrated at boot but drifts with temperature — in a room that swings from freezing to 20 °C. `millis()` comes off the 40 MHz crystal, so the *third* fallback is plausibly more accurate than the second. Worth measuring before trusting the hierarchy.

`getDateTime()` records where the time came from: `getTimeSource()` and `isTimeTrusted()`. Untrusted means it's the **compile-date seed** returned by `buildFallbackDateTime()` when no source has ever been valid — it passes the year-range check and looks entirely plausible. Anything scheduling into the future (delayed start) should check `isTimeTrusted()`; `saveStage2StartTime()` already logs loudly when it doesn't hold.

### Configuration & persistence

- Config lives in **SPIFFS**, not the SD card — defective SD cards were a recurring field failure. `/config.txt` (network/MQTT) and `/defaultParameters.txt` (process parameters). SD is **optional** and used only for logs; every SD path must stay guarded (`/logs` and `/download_log` return 503 without a card).
- Never write config with raw `SPIFFS.open()`. Go through `ConfigStore::write()` — it does `.tmp → validate JSON → rotate .bak → promote`, and `read()` falls back to `.bak` and restores it.
- `samples/` holds reference shapes for both files. `data/` is gitignored (real deployment configs).
- Three layers of fallback keep the room running: primary → `.bak` → `EMBEDDED_DEFAULT_PARAMS` compiled into `config.h`.
- Process parameters are settable at runtime from three directions (MQTT topics, the `/edit-settings` web form, and the params file). **Any new parameter needs all three wired up**, plus the ACK topic — a write/read JSON key-case mismatch already silently broke `f1RevOntime` once.

### Time

`Controller::getDateTime()` returns **local** time via DS3231 → ESP32 internal RTC → last-valid + `millis()` delta. The internal RTC stores UTC; `TIME_ZONE_OFFSET_HRS` is applied only at the return point. TLS certificate validity depends on this, so `forceNTPSync()` is called before the MQTT connect when TLS is on.

## The measurement chain (read before touching anything analog)

`PT-100 + transmitter (M12, 4-20 mA)` → ~2 m cable → **`M5S-AIA03020B3`** module (isolated, 0-20 mA in → 0-3.3 V out) → ESP32 ADC. Board is a **ZD ESP32-MIO-16** with a custom adapter so a Heltec S3 sits where the original ESP32 went.

The arithmetic that follows from that, and that most of the analog code does *not* yet honor:

| Current | Volts | **ADC counts** | °C (true, from the -50…+50 span) |
|---|---|---|---|
| 4 mA | 0.66 V | **819** | -50 |
| 20 mA | 3.30 V | **4095** | +50 |

- **A healthy 4-20 mA loop never reads below ~819 counts.** Anything under that is an open loop — dead transmitter, cut cable, or moisture bridging. `raw == 0` is the extreme case and is exactly the **-64.5 °C** operators see in the SCADA today.
- **Full scale is a fault signature too**: a pegged transmitter reads +50 °C true (43.2 °C through the current ramp) and **passes the `T*_MAX = 60` validation**, which can end stage 2 instantly. In the field this shows up as "forty-something".
- The isolation in the `B3` modules is done with **optocouplers**, so the module is **non-linear by design** (the ZD manual lists `B3` as "none linear"). That — not the ESP32 ADC — is why the ramp had to be fitted in Excel. The in-use ramp `raw*0.0263 - 64.5` matches the true curve to 0.1 °C at mid-scale and drifts **~7 °C at both ends**. The process works anyway because the product window (-3…+2 °C) sits at mid-scale.
- Optocoupler CTR **degrades with age**, so any fitted calibration goes stale on its own. Calibration belongs in SPIFFS (editable per room, per channel), not compiled into `config.h`.
- Module error alone is **±0.54 °C at 25 °C and ±1.08 °C** across ambient range — against a 5 °C-wide acceptance window.
- The analog **output** module (Port A-03, `M5S-AOV03010C3`) is **linear** and fine; it is also the only **non-isolated** module in the chain, sharing ground with the switching blower and sitting on `GPIO16 = ADC2_CH5`, right next to `Ta` on `GPIO15`.

Use the **`cfpp4/adc_raw`** topic (published every 10 s by `publishAdcDiagnostics()`) to see counts directly — converting to °C is exactly what's under suspicion.

## Gotchas

- **`Ta` reads on ADC2** (AI_0 = GPIO15), which is arbitrated with the radio. Under WiFi/TLS load `analogRead()` silently returns 0 counts, which the calibration ramp turns into −64.5 °C — glitches always go *downward*. This is filtered by a **sliding median** (`ADC_MEDIAN_*` in `Controller.h`), one sample per loop iteration, never an average. Don't "simplify" it to a mean. `readTempFrom()` is unlocked on purpose — call it only from `loop()` (core 1). Note this is an artifact of the **custom S3 adapter**, not of the ZD board: `Ts`/`Tc` land on ADC1.
- **Calibration is per channel in `config.h` — except `Tc`, which has no ramp of its own** and silently falls through to a hardcoded default identical to `Ta`'s, despite the commit named "Tc coeff != Ta coeff".
- **Deployment is manual through the web server** (`/update`), not OTA — plant IT blocks the IoT network. Worst case Tristán stops production and flashes over cable in Malaysia. That makes `WIFI.cpp` / `WebFiles.cpp` and the `/update` route operationally critical: break them and 9 rooms become unupdatable from 15,000 km away.
- Out-of-range readings are treated as *sensor faults*: the last good value is kept and an alarm is published once per transition, **without stopping the cycle** (`updateSensorReading()`).
- **MQTT connect is gated behind `online`** — in AP mode `IP_ADDRESS` is empty and a TLS connect to an unreachable host hangs ~30 s.
- **No LoRa/Heltec display code**, no WebSerial — all removed in the IDF5 migration. Don't reintroduce `libheltec`, `LoraWan102`, `DISPLAY` or `WebSerialLite`.
- `platformio.ini` intentionally does not pin `upload_port`/`monitor_port` (breaks on Windows COM ports and other Macs), and sets `test_ignore = test_configstore` on the board env so `pio test` doesn't try to cross-compile the native suite.
- The build force-includes `soc/gpio_struct.h` because OneWire 2.3.8 pokes GPIO registers directly and IDF5 no longer pulls in the `GPIO` symbol.
- **`ESPAsyncWebServer` closes the SD `File` after `request->send(file, …)`** — calling `file.close()` yourself truncates large downloads.
- Don't add `while(true)` retry loops at boot. Every one of them (SD mount, missing config) has already been removed for freezing the firmware in the field; use bounded retries plus a degraded mode.

## Native tests

`[env:native]` compiles **only** `src/hardware/ConfigStore.cpp` (`build_src_filter`) against in-RAM mocks in `test/mock/` — the FS is a `std::map<path, content>`, `String` is aliased to `std::string`, the mutex is a no-op shim, and the real `Logger.h` is neutralized by force-including the mock. Coverage: invalid-JSON rejection, clean promote, `.bak` rotation, corrupt-primary recovery, both-corrupt, and a dangling `.tmp` from a simulated power cut. Anything that needs the ESP32 core cannot go in this env.
