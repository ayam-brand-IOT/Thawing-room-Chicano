# Sample configuration files

Reference JSON for the two config files the firmware reads from **SPIFFS**. The real
files live under `data/` (git-ignored) and are flashed with `pio run -t uploadfs`, or
edited live from the web UI (`/edit-config?file=config.txt` / `?file=defaultParameters.txt`).

Both are written **atomically** by `ConfigStore` (`.tmp` → validate → `.bak` → promote),
so a power cut mid-write never corrupts the good file. Values are stored as JSON strings
for numbers (e.g. `"8883"`, `"-7"`) and as JSON booleans for flags (`true` / `false`),
matching how the firmware parses them.

---

## `config.sample.json` → SPIFFS `/config.txt`

Device, Wi-Fi and MQTT settings. Parsed in `Controller::runConfigFile()`.

| Key | Type | Required | Description |
|---|---|---|---|
| `SSID` | string | yes | Wi-Fi network name |
| `WIFI_PASSWORD` | string | yes | Wi-Fi password |
| `HOST_NAME` | string | yes | mDNS hostname → `<name>.local` |
| `STATIC_IP` | string | optional | Static IP for the device. Used **with** `GATEWAY` |
| `GATEWAY` | string | optional | Gateway IP. Only applied when `STATIC_IP` is also present (subnet defaults to 255.255.255.0) |
| `IP_ADDRESS` | string | yes | MQTT **broker** IP/host |
| `PORT` | string | yes | MQTT broker port — `"1883"` plaintext, `"8883"` TLS |
| `USE_TLS` | bool | optional | Enable TLS for MQTT (verifies broker against the embedded CA). Default `true` |
| `MQTT_ID` | string | yes | MQTT client id |
| `USERNAME` | string | yes | MQTT username |
| `MQTT_PASSWORD` | string | yes | MQTT password |
| `TIME_ZONE_OFFSET_HRS` | string | optional | Local UTC offset, e.g. `"-7"` Ensenada, `"8"` Taiping |
| `IR_TS` | bool | optional | Use the IR (MLX90640) sensor for the Ts reading |
| `LoRa_Tc` | bool | optional | Receive Tc over LoRa |

> `WEB_SERIAL` was removed (WebSerial dropped in the arduino-esp32 3.x migration). The key
> is ignored if present.

If `config.txt` is missing or unparseable, the device boots straight into the **AP config
portal** instead of trying credentials.

---

## `defaultParameters.sample.json` → SPIFFS `/defaultParameters.txt`

Per-stage process defaults loaded at boot in `Controller::setUpDefaultParameters()` and
re-written by `Controller::updateDefaultParameters()`. If the file is missing, the firmware
falls back to `EMBEDDED_DEFAULT_PARAMS` (in `include/config.h`) and re-seeds SPIFFS.

Each `stageN` object (fan = F1, sprinkler = S1), times in **minutes**:

| Key | Description |
|---|---|
| `f1Ontime` | Fan ON time (forward) |
| `f1RevOntime` | Fan ON time in **reverse** (CCW) |
| `f1Offtime` | Fan OFF time |
| `s1Ontime` | Sprinkler ON time |
| `s1Offtime` | Sprinkler OFF time (clamped to `MIN_OFFTIME_STAGEn`) |

`setPoint`:

| Key | Description |
|---|---|
| `A`, `B` | Room set-point coefficients |
| `coef_pid_fwd` | PID forward coefficient (default 100) |
| `coef_pid_rev` | PID reverse coefficient (default 100) |

`tset`: `tsSet` / `tcSet` — Ts and Tc target temperatures (°C).

> Every value can also be overridden at runtime over MQTT (topics under `cfpp4/…`),
> but `defaultParameters.txt` is what the controller loads on boot.
