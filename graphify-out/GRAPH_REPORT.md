# Graph Report - /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new  (2026-10-30)

## Corpus Check
- 41 files · ~93,842 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 403 nodes · 1036 edges · 30 communities detected
- Extraction: 62% EXTRACTED · 38% INFERRED · 0% AMBIGUOUS · INFERRED: 392 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 5|Community 5]]
- [[_COMMUNITY_Community 6|Community 6]]
- [[_COMMUNITY_Community 7|Community 7]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]
- [[_COMMUNITY_Community 12|Community 12]]
- [[_COMMUNITY_Community 13|Community 13]]
- [[_COMMUNITY_Community 14|Community 14]]
- [[_COMMUNITY_Community 15|Community 15]]
- [[_COMMUNITY_Community 16|Community 16]]
- [[_COMMUNITY_Community 17|Community 17]]
- [[_COMMUNITY_Community 18|Community 18]]
- [[_COMMUNITY_Community 19|Community 19]]
- [[_COMMUNITY_Community 20|Community 20]]
- [[_COMMUNITY_Community 21|Community 21]]
- [[_COMMUNITY_Community 22|Community 22]]
- [[_COMMUNITY_Community 23|Community 23]]
- [[_COMMUNITY_Community 24|Community 24]]
- [[_COMMUNITY_Community 25|Community 25]]
- [[_COMMUNITY_Community 26|Community 26]]
- [[_COMMUNITY_Community 27|Community 27]]
- [[_COMMUNITY_Community 28|Community 28]]
- [[_COMMUNITY_Community 29|Community 29]]

## God Nodes (most connected - your core abstractions)
1. `println()` - 47 edges
2. `setup()` - 27 edges
3. `begin()` - 26 edges
4. `write()` - 25 edges
5. `handleStage1()` - 21 edges
6. `serviceRemoteControl()` - 21 edges
7. `setUpWebServer()` - 21 edges
8. `getDateTime()` - 19 edges
9. `handleStage2()` - 18 edges
10. `DEBUG()` - 18 edges

## Surprising Connections (you probably didn't know these)
- `rename()` --calls--> `end()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/test/mock/FS.h → lib/SD/src/SD.cpp
- `sdCommand()` --calls--> `CRC7()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/sd_diskio_crc.c
- `sdcard_uninit()` --calls--> `end()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/SD.cpp
- `sdcard_init()` --calls--> `begin()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/SD.cpp
- `sdcard_unmount()` --calls--> `end()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/SD.cpp

## Communities

### Community 0 - "Community 0"
Cohesion: 0.08
Nodes (51): begin(), buildFallbackDateTime(), checkAndInsertBottomTemps(), Controller(), DEBUG(), ERROR(), forceNTPSync(), getAvgBottomTemp() (+43 more)

### Community 1 - "Community 1"
Cohesion: 0.11
Nodes (42): getFanState(), getRemainingMinutesToStage2(), readDigitalInput(), turnOnFan(), writeAnalogOutput(), writeDigitalOutput(), print(), println() (+34 more)

### Community 2 - "Community 2"
Cohesion: 0.11
Nodes (37): getSDInfo(), begin(), cardSize(), cardType(), CRC16(), CRC7(), ff_sd_initialize(), ff_sd_ioctl() (+29 more)

### Community 3 - "Community 3"
Cohesion: 0.11
Nodes (34): exists(), isValidJson(), migrateFromSD(), read(), readRaw(), write(), setUpDefaultParameters(), begin() (+26 more)

### Community 4 - "Community 4"
Cohesion: 0.1
Nodes (30): sample(), service(), setContext(), findAdcMedian(), getAdcDiag(), getAdcMedian(), hasIRSensor(), isLoraTc() (+22 more)

### Community 5 - "Community 5"
Cohesion: 0.1
Nodes (28): channelKey(), connectToWiFi(), isWiFiConnected(), startConfigPortal(), WiFiLoop(), setClock(), backgroundTasks(), loop() (+20 more)

### Community 6 - "Community 6"
Cohesion: 0.12
Nodes (30): isRTCConnected(), isStage2TimeSet(), appendSD(), cycleEnd(), cycleResume(), cycleStart(), drain(), emit() (+22 more)

### Community 7 - "Community 7"
Cohesion: 0.13
Nodes (25): begin(), factoryOffset(), factorySlope(), parseCal(), post(), printCal(), putCal(), request() (+17 more)

### Community 8 - "Community 8"
Cohesion: 0.2
Nodes (17): saveLogToSD(), begin(), drain(), ensureLogFolder(), getFileName(), init(), levelName(), log() (+9 more)

### Community 9 - "Community 9"
Cohesion: 0.14
Nodes (18): AsyncTCP Library, ESPAsyncTCP Library, ESPAsyncWebServer, DS3231 RTC, ESP32-S3 (Heltec WiFi LoRa 32 V3), I2C Bus Recovery, Non-Blocking Stage Init, NTP Synchronization (+10 more)

### Community 10 - "Community 10"
Cohesion: 0.23
Nodes (13): connect(), DEBUG(), fullTopic(), isConnected(), isServiceAvailable(), isTopicEqual(), loop(), onConnect() (+5 more)

### Community 11 - "Community 11"
Cohesion: 0.5
Nodes (4): xSemaphoreCreateRecursiveMutex(), xSemaphoreGiveRecursive(), xSemaphoreTakeRecursive(), ScopedLock()

### Community 12 - "Community 12"
Cohesion: 1.0
Nodes (0): 

### Community 13 - "Community 13"
Cohesion: 1.0
Nodes (0): 

### Community 14 - "Community 14"
Cohesion: 1.0
Nodes (0): 

### Community 15 - "Community 15"
Cohesion: 1.0
Nodes (0): 

### Community 16 - "Community 16"
Cohesion: 1.0
Nodes (0): 

### Community 17 - "Community 17"
Cohesion: 1.0
Nodes (0): 

### Community 18 - "Community 18"
Cohesion: 1.0
Nodes (0): 

### Community 19 - "Community 19"
Cohesion: 1.0
Nodes (0): 

### Community 20 - "Community 20"
Cohesion: 1.0
Nodes (0): 

### Community 21 - "Community 21"
Cohesion: 1.0
Nodes (0): 

### Community 22 - "Community 22"
Cohesion: 1.0
Nodes (0): 

### Community 23 - "Community 23"
Cohesion: 1.0
Nodes (0): 

### Community 24 - "Community 24"
Cohesion: 1.0
Nodes (0): 

### Community 25 - "Community 25"
Cohesion: 1.0
Nodes (0): 

### Community 26 - "Community 26"
Cohesion: 1.0
Nodes (0): 

### Community 27 - "Community 27"
Cohesion: 1.0
Nodes (0): 

### Community 28 - "Community 28"
Cohesion: 1.0
Nodes (0): 

### Community 29 - "Community 29"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **8 isolated node(s):** `ESP32-S3 (Heltec WiFi LoRa 32 V3)`, `Watchdog Timer (5 min timeout)`, `I2C Bus Recovery`, `Non-Blocking Stage Init`, `Safe Actuator State on Boot` (+3 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 12`** (2 nodes): `SD.h`, `fs()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 13`** (2 nodes): `MqttClient()`, `MqttClient.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 14`** (2 nodes): `WIFI.h`, `WIFI()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 15`** (2 nodes): `EventLog()`, `EventLog.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 16`** (2 nodes): `SensorCalibration()`, `Calibration.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 17`** (2 nodes): `ConfigStore()`, `ConfigStore.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 18`** (2 nodes): `RemoteControl()`, `RemoteControl.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `SD.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `SPIFFS.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `types.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 23`** (1 nodes): `sd_defines.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 24`** (1 nodes): `sd_diskio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 25`** (1 nodes): `mqtt_certs.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 26`** (1 nodes): `Thawing-room-chicano.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 27`** (1 nodes): `CalibrationMath.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 28`** (1 nodes): `WebFiles.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 29`** (1 nodes): `WebFiles.cpp`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `println()` connect `Community 1` to `Community 0`, `Community 2`, `Community 3`, `Community 4`, `Community 5`, `Community 6`, `Community 8`, `Community 10`?**
  _High betweenness centrality (0.159) - this node is a cross-community bridge._
- **Why does `write()` connect `Community 3` to `Community 0`, `Community 1`, `Community 2`, `Community 4`, `Community 5`, `Community 7`, `Community 8`?**
  _High betweenness centrality (0.132) - this node is a cross-community bridge._
- **Why does `end()` connect `Community 0` to `Community 2`, `Community 3`, `Community 6`?**
  _High betweenness centrality (0.075) - this node is a cross-community bridge._
- **Are the 37 inferred relationships involving `println()` (e.g. with `printStackUsage()` and `setup()`) actually correct?**
  _`println()` has 37 INFERRED edges - model-reasoned connections that need verification._
- **Are the 20 inferred relationships involving `setup()` (e.g. with `init()` and `begin()`) actually correct?**
  _`setup()` has 20 INFERRED edges - model-reasoned connections that need verification._
- **Are the 25 inferred relationships involving `begin()` (e.g. with `setUp()` and `setup()`) actually correct?**
  _`begin()` has 25 INFERRED edges - model-reasoned connections that need verification._
- **Are the 19 inferred relationships involving `write()` (e.g. with `test_write_rechaza_json_invalido()` and `test_write_crea_archivo_y_sin_residuos()`) actually correct?**
  _`write()` has 19 INFERRED edges - model-reasoned connections that need verification._