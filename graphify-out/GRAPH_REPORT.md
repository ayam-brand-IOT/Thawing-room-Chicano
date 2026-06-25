# Graph Report - /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new  (2026-06-24)

## Corpus Check
- 27 files · ~47,357 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 268 nodes · 656 edges · 21 communities detected
- Extraction: 65% EXTRACTED · 35% INFERRED · 0% AMBIGUOUS · INFERRED: 227 edges (avg confidence: 0.8)
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

## God Nodes (most connected - your core abstractions)
1. `println()` - 41 edges
2. `begin()` - 22 edges
3. `handleStage1()` - 21 edges
4. `setup()` - 19 edges
5. `handleStage2()` - 18 edges
6. `DEBUG()` - 18 edges
7. `getDateTime()` - 17 edges
8. `handleStage3()` - 16 edges
9. `write()` - 15 edges
10. `callback()` - 13 edges

## Surprising Connections (you probably didn't know these)
- `ERROR()` --calls--> `println()`  [INFERRED]
  src/Stage.cpp → /Users/elw/Documents/Programacion/Fish_proyect/Projects/Thawing Room/ZDAuto Version/Thawing-room-Chicano_new/src/hardware/Logger.cpp
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
Nodes (47): begin(), buildFallbackDateTime(), checkAndInsertBottomTemps(), Controller(), DEBUG(), forceNTPSync(), getAvgBottomTemp(), getDateTime() (+39 more)

### Community 1 - "Community 1"
Cohesion: 0.13
Nodes (38): getFanState(), getRemainingMinutesToStage2(), readDigitalInput(), turnOnFan(), writeAnalogOutput(), writeDigitalOutput(), publishData(), DEBUG() (+30 more)

### Community 2 - "Community 2"
Cohesion: 0.11
Nodes (36): begin(), cardSize(), cardType(), CRC16(), CRC7(), ff_sd_initialize(), ff_sd_ioctl(), ff_sd_read() (+28 more)

### Community 3 - "Community 3"
Cohesion: 0.12
Nodes (29): exists(), isValidJson(), migrateFromSD(), read(), readRaw(), write(), ERROR(), runConfigFile() (+21 more)

### Community 4 - "Community 4"
Cohesion: 0.13
Nodes (18): connectToWiFi(), startConfigPortal(), hasSD(), backgroundTasks(), DEBUG(), ERROR(), getIP(), handleFileUpload() (+10 more)

### Community 5 - "Community 5"
Cohesion: 0.16
Nodes (17): setTsContactLess(), connect(), DEBUG(), ERROR(), isConnected(), isServiceAvailable(), isTopicEqual(), loop() (+9 more)

### Community 6 - "Community 6"
Cohesion: 0.14
Nodes (18): AsyncTCP Library, ESPAsyncTCP Library, ESPAsyncWebServer, DS3231 RTC, ESP32-S3 (Heltec WiFi LoRa 32 V3), I2C Bus Recovery, Non-Blocking Stage Init, NTP Synchronization (+10 more)

### Community 7 - "Community 7"
Cohesion: 0.16
Nodes (12): hasIRSensor(), isLoraTc(), isTsContactLess(), jsonBuilder(), readTempFrom(), addValue(), getAverage(), getTempAvg() (+4 more)

### Community 8 - "Community 8"
Cohesion: 1.0
Nodes (0): 

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

## Knowledge Gaps
- **8 isolated node(s):** `ESP32-S3 (Heltec WiFi LoRa 32 V3)`, `Watchdog Timer (5 min timeout)`, `I2C Bus Recovery`, `Non-Blocking Stage Init`, `Safe Actuator State on Boot` (+3 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Community 8`** (2 nodes): `SD.h`, `fs()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 9`** (2 nodes): `MqttClient()`, `MqttClient.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 10`** (2 nodes): `WIFI.h`, `WIFI()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 11`** (2 nodes): `ConfigStore()`, `ConfigStore.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 12`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 13`** (1 nodes): `types.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 14`** (1 nodes): `WS_V2.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 15`** (1 nodes): `sd_defines.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 16`** (1 nodes): `sd_diskio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 17`** (1 nodes): `mqtt_certs.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 18`** (1 nodes): `Thawing-room-chicano.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 19`** (1 nodes): `WebFiles.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Community 20`** (1 nodes): `WebFiles.cpp`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `println()` connect `Community 3` to `Community 0`, `Community 1`, `Community 4`, `Community 5`, `Community 7`?**
  _High betweenness centrality (0.211) - this node is a cross-community bridge._
- **Why does `write()` connect `Community 3` to `Community 0`, `Community 2`, `Community 4`?**
  _High betweenness centrality (0.118) - this node is a cross-community bridge._
- **Why does `end()` connect `Community 0` to `Community 2`?**
  _High betweenness centrality (0.091) - this node is a cross-community bridge._
- **Are the 33 inferred relationships involving `println()` (e.g. with `DEBUG()` and `ERROR()`) actually correct?**
  _`println()` has 33 INFERRED edges - model-reasoned connections that need verification._
- **Are the 21 inferred relationships involving `begin()` (e.g. with `setup()` and `init()`) actually correct?**
  _`begin()` has 21 INFERRED edges - model-reasoned connections that need verification._
- **Are the 15 inferred relationships involving `handleStage1()` (e.g. with `getCurrentStep()` and `isStage2TimeSet()`) actually correct?**
  _`handleStage1()` has 15 INFERRED edges - model-reasoned connections that need verification._
- **Are the 17 inferred relationships involving `setup()` (e.g. with `init()` and `runConfigFile()`) actually correct?**
  _`setup()` has 17 INFERRED edges - model-reasoned connections that need verification._