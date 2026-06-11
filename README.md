# Thawing Room Controller — Chicano

ESP32-based (Heltec WiFi LoRa 32 V3) automated thawing room controller.

---

## Table of Contents

- [Hardware](#hardware)
- [Configuration Storage (SPIFFS)](#configuration-storage-spiffs)
  - [Atomic Writes & Backup](#atomic-writes--backup)
  - [SD → SPIFFS Auto-Migration](#sd--spiffs-auto-migration)
  - [Optional SD Card](#optional-sd-card)
- [Access Point Config Portal](#access-point-config-portal)
- [Reliability & Safety](#reliability--safety)
  - [Boot Cause Logging](#boot-cause-logging)
  - [Non-Blocking Stage Init](#non-blocking-stage-init)
  - [Safe Actuator State on Boot](#safe-actuator-state-on-boot)
- [RTC Architecture](#rtc-architecture)
  - [Time Source Hierarchy](#time-source-hierarchy)
  - [NTP Synchronization](#ntp-synchronization)
  - [I2C Bus Recovery](#i2c-bus-recovery)
- [Stage Scheduling](#stage-scheduling)
  - [Stage2 Epoch Persistence](#stage2-epoch-persistence)
- [Web Interface](#web-interface)
- [Configuration](#configuration)
- [Task Scheduler](#task-scheduler)

---

## Hardware

| Component | Detail |
|---|---|
| MCU | ESP32-S3 (Heltec WiFi LoRa 32 V3) |
| External RTC | DS3231 on dedicated `TwoWire(0)` bus (`I2C_SDA` / `I2C_SCL`) |
| Config storage | **SPIFFS** (internal flash) — `config.txt`, `defaultParameters.txt` |
| Log storage | SD card (optional — device runs without it) |
| Sensors | DS18B20 (Ta, Ts, Tc, Ti), optional MLX90614 IR (Ts), optional LoRa (Tc) |
| Actuators | Fan F1 (fwd/rev), sprinkler valve S1, air damper, air PWM |

---

## Configuration Storage (SPIFFS)

Defective SD cards were a recurring field failure. To remove that single point of failure, **all configuration now lives in SPIFFS** (the ESP32's internal flash) via the `ConfigStore` module. The SD card is used **only for logs** and is fully optional.

| File | Location | Purpose |
|---|---|---|
| `config.txt` | SPIFFS | WiFi / MQTT / device settings |
| `defaultParameters.txt` | SPIFFS | Stage timings, setpoints, target temps |
| `/logs/*.txt` | SD card | Process logs (optional) |

### Atomic Writes & Backup

Every config write goes through a crash-safe sequence so a power loss mid-write can never corrupt the good file:

```
1. Write new content to "<file>.tmp"
2. Validate it parses back as JSON      ──► fail ► abort, keep current file
3. Rename current good file to "<file>.bak"
4. Promote "<file>.tmp"  →  "<file>"
```

On **read**, if the primary file is missing or unparseable, `ConfigStore` automatically falls back to `<file>.bak` and restores it. If both are gone, `defaultParameters.txt` falls back to **compile-time embedded defaults** (`EMBEDDED_DEFAULT_PARAMS`) so the room always has valid parameters.

### SD → SPIFFS Auto-Migration

Already-deployed units have their config on the SD card. On boot, `ConfigStore::migrateFromSD()` copies each config file from SD → SPIFFS **only if** SPIFFS doesn't already have it and the SD does. This is idempotent and needs no field intervention — the first boot on this firmware migrates, every later boot is a no-op.

### Optional SD Card

`Logger::setupSD()` uses a **bounded** retry (3 attempts) instead of the old infinite loop. If no SD is present, the device logs a notice and keeps running; `writeSD()` and the `/logs` & `/download_log` web routes are guarded and return `503` rather than blocking. A background `retrySD()` hook can re-detect a card inserted later.

---

## Access Point Config Portal

If the device can't get online, it becomes its own WiFi Access Point hosting the existing web UI so a technician can reconfigure it on site — **the room control loop keeps running the whole time** (stages, fans, sensors, sprinkler). The portal is for reconfiguration only; it never pauses the process.

**Two boot-time triggers:**

| Trigger | Condition |
|---|---|
| No config | `config.txt` not found / unparseable in SPIFFS (or `.bak`) |
| WiFi failed | Station mode failed to connect after `AP_MAX_BOOT_WIFI_TRIES` (3) boot attempts |

> The 3-attempt count reuses the existing EEPROM retry counter. The AP is a **boot-only** fallback: a WiFi drop *during* operation does **not** trigger AP — it just retries reconnecting in the background.

**How it works:**

```
WiFi.softAP("ThawingRoom-<hostname>", AP_PASSWORD)   // WPA2
DNSServer on port 53, "*" → AP IP                    // captive portal
→ any URL redirects to /edit-config                  // page opens automatically
```

The AP SSID, WPA2 password (`AP_PASSWORD`), and trigger threshold are configured in [include/config.h](include/config.h). The captive DNS is serviced from the background task (`loopAP()` → `dnsServer.processNextRequest()`), independent of the control loop.

---

## Reliability & Safety

This is an industrial machine. The firmware is designed so that **the process never stops unexpectedly** and any failure is handled gracefully.

### Boot Cause Logging

Every time the system starts, `logResetReason()` reads the ESP32 reset reason (`esp_reset_reason()`) and writes the cause to the SD log. This gives visibility into *why* the device restarted — power cycle, planned reboot, crash, or a system-level fault:

| Cause | Log message |
|---|---|
| Normal power-on | `[BOOT] Power-on reset` |
| `ESP.restart()` | `[BOOT] Software restart` |
| Panic / crash | `[CRITICAL] Reset by panic / crash` |
| Interrupt / task WDT (system level) | `[CRITICAL] Reset by interrupt WDT` / `[CRITICAL] Reset by task WDT` |
| Brownout (low voltage) | `[CRITICAL] Reset by brownout` |

> **Note:** these WDT causes are reported by the ESP-IDF/hardware reset reason. The firmware does **not** currently run an application-level `esp_task_wdt` feeding the main loop — an earlier 5-minute software watchdog was removed (see [Stability Improvements Log](#stability-improvements-log)).

### Non-Blocking Stage Init

Stage 2 and Stage 3 previously called `delay(5000)` during initialization, which froze the entire loop for 5 seconds — making the STOP button, MQTT messages, and temperature reads unresponsive.

This is now replaced with a **non-blocking timer guard**:

```cpp
// Step 0: record the init timestamp
timers.stage2.init_delay = millis();

// Step 1: return early until 5s have elapsed — loop keeps running normally
if ((millis() - timers.stage2.init_delay) < 5000) return;
```

During the 5-second window the loop continues executing: STOP commands, MQTT, temperature updates and all other tasks remain fully responsive.

### Safe Actuator State on Boot

`setUpDigitalOutputs()` drives all output pins `LOW` during `init()`, before any stage logic runs. This ensures that on any reboot — whether planned, WDT-triggered, or brownout — all actuators (fan, valve, damper, PWM) start in the **off** state.

---

## RTC Architecture

### Time Source Hierarchy

`getDateTime()` returns the current local time using a 3-layer fallback:

```
1. External DS3231 RTC  (primary)
       │
       ├─ Valid?  → return local time (UTC + TIME_ZONE_OFFSET_HRS)
       └─ Invalid / disconnected
              │
2. ESP32 Internal RTC   (secondary — set via settimeofday)
       │
       ├─ Valid?  → return local time
       └─ Never synced
              │
3. Last valid time + millis() delta  (last resort)
              └─ returns approximate time, logs warning once
```

All three sources return **local time** (`UTC + TIME_ZONE_OFFSET_HRS`).  
The internal RTC stores **UTC** — timezone offset is applied only at the `getDateTime()` return point.

### NTP Synchronization

Three scenarios trigger an NTP sync:

| Trigger | Condition |
|---|---|
| External RTC reconnected | `rtc_needs_ntp_sync == true` after `tryConnectRTC()` recovers |
| First boot without external RTC | `!ntp_synced_to_internal && wifi.isConnected()` |
| **Periodic 24-hour sync** | `TaskScheduler` fires `ntp_sync_task` every 24 h |

The periodic sync is handled by `Controller::forceNTPSync()`, called by `TaskScheduler` (see [Task Scheduler](#task-scheduler)):

```
WiFi available?
  └─ External RTC present?
        ├─ YES  →  NTP → DS3231  +  NTP → ESP32 internal RTC
        └─ NO   →  NTP → ESP32 internal RTC only
```

NTP server: `pool.ntp.org`, UTC offset 0 (offset applied in software).  
Up to 5 attempts per sync call with a fresh local `WiFiUDP` + `NTPClient` instance.

### I2C Bus Recovery

When the DS3231 fails to respond, `resetI2CBus()` runs before a retry:

1. `rtc_i2c.end()` — release the bus
2. 9 manual SCL clock pulses — unlock any device stuck mid-transaction
3. Software START + STOP conditions — force I2C IDLE state
4. `rtc_i2c.begin(I2C_SDA, I2C_SCL)` — reinitialize

Reconnect attempts are rate-limited to **once every 5 seconds** to avoid blocking the main loop.  
State-change logs are printed only when the connection status actually changes.

---

## Stage Scheduling

The machine runs three sequential stages:

```
IDLE → STAGE1 (delayed start) → STAGE2 (active thawing) → STAGE3 (hold) → IDLE
```

### Stage1 → Stage2 transition

The target transition time is stored as a **Unix epoch** (`uint32_t g_stage2_epoch`).  
This replaces the previous fragile hour/minute comparison that suffered from uint32 overflow across midnight.

Transition check in `handleStage1()`:
```cpp
if (controller.hasStage2TimeElapsed(current_date)) {
    stage_1.destroy();
    setStage(STAGE2);
}
```

Remaining time published to MQTT:
```cpp
const long remaining_mins = controller.getRemainingMinutesToStage2(current_date);
mqtt.publishData(REMAINING_D_START, remaining_mins > 0 ? remaining_mins : 0);
```

### Stage2 Epoch Persistence

The epoch survives two failure scenarios:

| Failure | Storage used | Survival |
|---|---|---|
| `ESP.restart()` / watchdog reset | `RTC_DATA_ATTR` (ESP32 RTC RAM) | ✅ |
| Power-off | `Preferences("stage2")` NVS flash | ✅ |

Recovery hierarchy on startup (inside `handleStage1()`):
```
1. RTC_DATA_ATTR g_stage2_set == true  →  use RAM epoch  (soft reset)
2. Preferences "stage2"/"set" == true  →  load from flash (power-off recovery)
3. Neither available                   →  recalculate from current parameters
```

---

## Web Interface

Served by `ESPAsyncWebServer` on port 80 with HTTP Basic Auth.

| Route | Method | Description |
|---|---|---|
| `/` | GET | Dashboard |
| `/logs` | GET | List SD log files |
| `/download_log?file=<name>` | GET | Download a log file from SD |
| `/edit-config` | GET | Config file editor UI |
| `/edit-settings` | GET | Parameters editor UI |
| `/update-config` | POST | Save config / default parameters to SPIFFS (atomic) |
| `/replace-config` | POST | Upload and replace `config.txt` (atomic) |
| `/update` | POST | OTA firmware update |
| `/reset` | POST | Restart ESP32 |
| `/toggle-output` | GET | Toggle logger between Serial / WebSerial |

> **Note on file downloads:** `ESPAsyncWebServer` closes the SD `File` handle automatically after the async transfer completes. Do **not** call `file.close()` after `request->send(file, ...)`.

> **Note when no SD is present:** `/logs` and `/download_log` return `503`; all config routes still work since config lives in SPIFFS. In AP mode, any unknown URL redirects to `/edit-config` (captive portal).

---

## Configuration

Stored in **SPIFFS** at `/config.txt` (JSON format). See [Configuration Storage (SPIFFS)](#configuration-storage-spiffs) for the atomic-write and migration details.

Key parameters:

| Key | Type | Description |
|---|---|---|
| `SSID` | string | WiFi network name |
| `WIFI_PASSWORD` | string | WiFi password |
| `TIME_ZONE_OFFSET_HRS` | int | Local timezone offset from UTC (e.g. `8` for UTC+8) |
| `HOST_NAME` | string | mDNS hostname (`<name>.local`) |
| `IP_ADDRESS` | string | MQTT broker IP |
| `PORT` | int | MQTT broker port |

Default process parameters are stored separately in `/defaultParameters.txt` (also in SPIFFS).

---

## Task Scheduler

Uses [TaskScheduler](https://github.com/arkhipenko/TaskScheduler) library.

| Task | Interval | Description |
|---|---|---|
| `high_priority_msgs` | 3 s | Publish temperatures to MQTT |
| `low_priority_msgs` | 10 s | Publish stage state, remaining time, PID, save SD log |
| `ntp_sync_task` | 24 h | Periodic NTP sync — calls `controller.forceNTPSync()` |
| `turn_on_flush` | one-shot | Turn on flush valve |
| `turn_off_flush` | one-shot (20 ms delay) | Turn off flush valve |

All tasks are registered and enabled in `setup()`. Every `loop()` call runs `runner.execute()` to service the scheduler.

---

## Stability Improvements Log

Chronological record of hardening changes made to this codebase.

| # | Change | Problem solved |
|---|---|---|
| 1 | 3-layer RTC fallback (ext → internal → millis) | Single point of failure on DS3231 disconnect |
| 2 | I2C bus reset (9 SCL pulses) | Frozen I2C bus after glitch |
| 3 | Non-blocking RTC reconnect (5s cooldown) | Blocking retry stalled the loop |
| 4 | NTP → internal RTC on first WiFi connect | No real time available without ext RTC |
| 5 | `ntp_sync_task` every 24h via TaskScheduler | DS3231 drift accumulation (~5 s/month) |
| 6 | Stage2 epoch stored as Unix timestamp | uint32 overflow crossing midnight |
| 7 | Epoch persisted to NVS flash | Epoch lost on power-off |
| 8 | `loadStage2StartTime()` recovery hierarchy | Epoch lost after WDT reboot |
| 9 | `delay(5000)` → non-blocking init guard | STOP button ignored for 5s on stage init |
| 10 | Watchdog timer (5 min timeout) | Frozen loop → actuators stuck indefinitely |
| 11 | `logResetReason()` on every boot | No visibility into unexpected reboots |
| 12 | All GPIOs driven LOW in `setUpDigitalOutputs()` | Undefined actuator state after reboot |
| 13 | `clearStage2Time()` called in `stopRoutine()` | Stale epoch used on next process cycle |
| 14 | SD file download — removed premature `file.close()` | Large log files truncated during download |
| 15 | Watchdog timer removed (reverts #10) | App-level `esp_task_wdt` no longer feeds the loop; reset cause still logged via `logResetReason()` |
| 16 | Config moved SD → SPIFFS (`ConfigStore`) | Defective SD cards corrupted/lost configuration in the field |
| 17 | Atomic config writes (`.tmp`→validate→`.bak`→promote) | Power loss mid-write corrupted the config file |
| 18 | One-time SD → SPIFFS auto-migration on boot | Deployed units upgrade without manual config re-entry |
| 19 | SD card made optional (bounded retry, guarded routes) | Missing/failed SD hung the boot in an infinite loop |
| 20 | Embedded compile-time default parameters | Room had no parameters if every params file was lost |
| 21 | AP config portal on no-config / 3 failed WiFi boots | Unconfigured or offline unit was unreachable on site |
| 22 | Removed 3 infinite `while(true)` config/SD loops | Any missing file froze the entire firmware at boot |
