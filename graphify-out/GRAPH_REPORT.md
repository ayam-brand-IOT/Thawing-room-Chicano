# Graph Report - /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new  (2026-08-03)

## Corpus Check
- 32 files · ~52,643 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 304 nodes · 732 edges · 23 communities detected
- Extraction: 64% EXTRACTED · 36% INFERRED · 0% AMBIGUOUS · INFERRED: 261 edges (avg confidence: 0.8)
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

## God Nodes (most connected - your core abstractions)
1. `println()` - 44 edges
2. `begin()` - 24 edges
3. `handleStage1()` - 21 edges
4. `write()` - 21 edges
5. `setup()` - 20 edges
6. `handleStage2()` - 18 edges
7. `DEBUG()` - 18 edges
8. `getDateTime()` - 17 edges
9. `handleStage3()` - 15 edges
10. `read()` - 14 edges

## Surprising Connections (you probably didn't know these)
- `println()` --calls--> `ERROR()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/src/hardware/Logger.cpp → src/Stage.cpp
- `println()` --calls--> `DEBUG()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/src/hardware/Logger.cpp → src/Stage.cpp
- `rename()` --calls--> `end()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/test/mock/FS.h → lib/SD/src/SD.cpp
- `sdCommand()` --calls--> `CRC7()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/sd_diskio_crc.c
- `sdcard_uninit()` --calls--> `end()`  [INFERRED]
  /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/lib/SD/src/sd_diskio.cpp → lib/SD/src/SD.cpp

## Communities

### Community 0 - "Community 0"
Cohesion: 0.08
Nodes (53): getFanState(), getRemainingMinutesToStage2(), hasIRSensor(), hasStage2TimeElapsed(), isLoraTc(), isStage2TimeSet(), isTsContactLess(), jsonBuilder() (+45 more)

### Community 1 - "Community 1"
Cohesion: 0.07
Nodes (51): begin(), buildFallbackDateTime(), checkAndInsertBottomTemps(), connectToWiFi(), Controller(), DEBUG(), ERROR(), forceNTPSync() (+43 more)

### Community 2 - "Community 2"
Cohesion: 0.11
Nodes (36): begin(), cardSize(), cardType(), CRC16(), CRC7(), ff_sd_initialize(), ff_sd_ioctl(), ff_sd_read() (+28 more)

### Community 3 - "Community 3"
Cohesion: 0.12
Nodes (29): exists(), isValidJson(), migrateFromSD(), read(), readRaw(), write(), setUpDefaultParameters(), updateDefaultParameters() (+21 more)

### Community 4 - "Community 4"
Cohesion: 0.11
Nodes (19): isWiFiConnected(), reconnectWiFi(), startConfigPortal(), WiFiLoop(), backgroundTasks(), DEBUG(), ERROR(), getIP() (+11 more)

### Community 5 - "Community 5"
Cohesion: 0.16
Nodes (17): connect(), DEBUG(), ERROR(), isConnected(), isServiceAvailable(), isTopicEqual(), loop(), onConnect() (+9 more)

### Community 6 - "Community 6"
Cohesion: 0.19
Nodes (15): saveLogToSD(), getFileName(), getSDInfo(), hasSD(), Logger(), print(), printError(), println() (+7 more)

### Community 7 - "Community 7"
Cohesion: 0.14
Nodes (18): AsyncTCP Library, ESPAsyncTCP Library, ESPAsyncWebServer, DS3231 RTC, ESP32-S3 (Heltec WiFi LoRa 32 V3), I2C Bus Recovery, Non-Blocking Stage Init, NTP Synchronization (+10 more)

### Community 8 - "Community 8"
Cohesion: 0.5
Nodes (4): xSemaphoreCreateRecursiveMutex(), xSemaphoreGiveRecursive(), xSemaphoreTakeRecursive(), ScopedLock()

### Community 9 - "Community 9"
Cohesion: 1.0
Nodes (0): 

### Community 10 - "Community 10"
Cohesion: 1.0
Nodes (0): 

### Community 11 - "Community 11"
Cohesion: 1.0
Nodes (0): 

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

## Knowledge Gaps
- **8 isolated node(s):** `ESP32-S3 (Heltec WiFi LoRa 32 V3)`, `Watchdog Timer (5 min timeout)`, `I2C Bus Recovery`, `Non-Blocking Stage Init`, `Safe Actuator State on Boot` (+3 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 9`** (2 nodes): `SD.h`, `fs()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 10`** (2 nodes): `MqttClient()`, `MqttClient.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 11`** (2 nodes): `WIFI.h`, `WIFI()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 12`** (2 nodes): `ConfigStore()`, `ConfigStore.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 13`** (1 nodes): `SD.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 14`** (1 nodes): `SPIFFS.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 15`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 16`** (1 nodes): `types.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 17`** (1 nodes): `sd_defines.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 18`** (1 nodes): `sd_diskio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `mqtt_certs.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `Thawing-room-chicano.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 21`** (1 nodes): `WebFiles.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 22`** (1 nodes): `WebFiles.cpp`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `println()` connect `Community 6` to `Community 0`, `Community 1`, `Community 3`, `Community 4`, `Community 5`?**
  _High betweenness centrality (0.219) - this node is a cross-community bridge._
- **Why does `write()` connect `Community 3` to `Community 1`, `Community 2`, `Community 4`, `Community 6`?**
  _High betweenness centrality (0.154) - this node is a cross-community bridge._
- **Why does `end()` connect `Community 1` to `Community 0`, `Community 2`, `Community 3`?**
  _High betweenness centrality (0.085) - this node is a cross-community bridge._
- **Are the 35 inferred relationships involving `println()` (e.g. with `DEBUG()` and `ERROR()`) actually correct?**
  _`println()` has 35 INFERRED edges - model-reasoned connections that need verification._
- **Are the 23 inferred relationships involving `begin()` (e.g. with `setUp()` and `setup()`) actually correct?**
  _`begin()` has 23 INFERRED edges - model-reasoned connections that need verification._
- **Are the 15 inferred relationships involving `handleStage1()` (e.g. with `getCurrentStep()` and `isStage2TimeSet()`) actually correct?**
  _`handleStage1()` has 15 INFERRED edges - model-reasoned connections that need verification._
- **Are the 15 inferred relationships involving `write()` (e.g. with `test_write_rechaza_json_invalido()` and `test_write_crea_archivo_y_sin_residuos()`) actually correct?**
  _`write()` has 15 INFERRED edges - model-reasoned connections that need verification._