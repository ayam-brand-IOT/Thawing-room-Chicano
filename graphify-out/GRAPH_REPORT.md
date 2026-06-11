# Graph Report - .  (2026-06-10)

## Corpus Check
- 158 files · ~225,334 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1704 nodes · 3838 edges · 80 communities detected
- Extraction: 67% EXTRACTED · 33% INFERRED · 0% AMBIGUOUS · INFERRED: 1282 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_LoRaWAN Regions|LoRaWAN Regions]]
- [[_COMMUNITY_AsyncWebServer Handlers|AsyncWebServer Handlers]]
- [[_COMMUNITY_AES Cryptography|AES Cryptography]]
- [[_COMMUNITY_RTC & WiFi Core|RTC & WiFi Core]]
- [[_COMMUNITY_SX126x LoRa Radio|SX126x LoRa Radio]]
- [[_COMMUNITY_TCP Socket Layer|TCP Socket Layer]]
- [[_COMMUNITY_DEPG0290 Display|DEPG0290 Display]]
- [[_COMMUNITY_Async Network IO|Async Network I/O]]
- [[_COMMUNITY_MCU Board Control|MCU Board Control]]
- [[_COMMUNITY_DEPG0150 Display|DEPG0150 Display]]
- [[_COMMUNITY_SD Card IO|SD Card I/O]]
- [[_COMMUNITY_AsyncTCP Buffer|AsyncTCP Buffer]]
- [[_COMMUNITY_AES Crypto Utils|AES Crypto Utils]]
- [[_COMMUNITY_External Libraries|External Libraries]]
- [[_COMMUNITY_HTTP Responses|HTTP Responses]]
- [[_COMMUNITY_Web Parameters|Web Parameters]]
- [[_COMMUNITY_Web Response Core|Web Response Core]]
- [[_COMMUNITY_Web Handler Core|Web Handler Core]]
- [[_COMMUNITY_WebSerial Frontend|WebSerial Frontend]]
- [[_COMMUNITY_SD Card Header|SD Card Header]]
- [[_COMMUNITY_Debug Utilities|Debug Utilities]]
- [[_COMMUNITY_SPIFFS Editor|SPIFFS Editor]]
- [[_COMMUNITY_WebSerialLite Lib|WebSerialLite Lib]]
- [[_COMMUNITY_LoRaWan App Class|LoRaWan App Class]]
- [[_COMMUNITY_MCU Class Wrapper|MCU Class Wrapper]]
- [[_COMMUNITY_Thawing Room Core|Thawing Room Core]]
- [[_COMMUNITY_Stage Management|Stage Management]]
- [[_COMMUNITY_Temperature Sensors|Temperature Sensors]]
- [[_COMMUNITY_Display Drivers|Display Drivers]]
- [[_COMMUNITY_Config Management|Config Management]]
- [[_COMMUNITY_Code Module 30|Code Module 30]]
- [[_COMMUNITY_Code Module 31|Code Module 31]]
- [[_COMMUNITY_Code Module 32|Code Module 32]]
- [[_COMMUNITY_Code Module 33|Code Module 33]]
- [[_COMMUNITY_Code Module 34|Code Module 34]]
- [[_COMMUNITY_Code Module 35|Code Module 35]]
- [[_COMMUNITY_Code Module 36|Code Module 36]]
- [[_COMMUNITY_Code Module 37|Code Module 37]]
- [[_COMMUNITY_Code Module 38|Code Module 38]]
- [[_COMMUNITY_Code Module 39|Code Module 39]]
- [[_COMMUNITY_Code Module 40|Code Module 40]]
- [[_COMMUNITY_Code Module 41|Code Module 41]]
- [[_COMMUNITY_Code Module 42|Code Module 42]]
- [[_COMMUNITY_Code Module 43|Code Module 43]]
- [[_COMMUNITY_Code Module 44|Code Module 44]]
- [[_COMMUNITY_Code Module 45|Code Module 45]]
- [[_COMMUNITY_Code Module 46|Code Module 46]]
- [[_COMMUNITY_Code Module 47|Code Module 47]]
- [[_COMMUNITY_Code Module 48|Code Module 48]]
- [[_COMMUNITY_Code Module 49|Code Module 49]]
- [[_COMMUNITY_Code Module 50|Code Module 50]]
- [[_COMMUNITY_Code Module 51|Code Module 51]]
- [[_COMMUNITY_Code Module 52|Code Module 52]]
- [[_COMMUNITY_Code Module 53|Code Module 53]]
- [[_COMMUNITY_Code Module 54|Code Module 54]]
- [[_COMMUNITY_Code Module 55|Code Module 55]]
- [[_COMMUNITY_Code Module 56|Code Module 56]]
- [[_COMMUNITY_Code Module 57|Code Module 57]]
- [[_COMMUNITY_Code Module 58|Code Module 58]]
- [[_COMMUNITY_Code Module 59|Code Module 59]]
- [[_COMMUNITY_Code Module 60|Code Module 60]]
- [[_COMMUNITY_Code Module 61|Code Module 61]]
- [[_COMMUNITY_Code Module 62|Code Module 62]]
- [[_COMMUNITY_Code Module 63|Code Module 63]]
- [[_COMMUNITY_Code Module 64|Code Module 64]]
- [[_COMMUNITY_Code Module 65|Code Module 65]]
- [[_COMMUNITY_Code Module 66|Code Module 66]]
- [[_COMMUNITY_Code Module 67|Code Module 67]]
- [[_COMMUNITY_Code Module 68|Code Module 68]]
- [[_COMMUNITY_Code Module 69|Code Module 69]]
- [[_COMMUNITY_Code Module 70|Code Module 70]]
- [[_COMMUNITY_Code Module 71|Code Module 71]]
- [[_COMMUNITY_Code Module 72|Code Module 72]]
- [[_COMMUNITY_Code Module 73|Code Module 73]]
- [[_COMMUNITY_Code Module 74|Code Module 74]]
- [[_COMMUNITY_Code Module 75|Code Module 75]]
- [[_COMMUNITY_Code Module 76|Code Module 76]]
- [[_COMMUNITY_Code Module 77|Code Module 77]]
- [[_COMMUNITY_Code Module 78|Code Module 78]]
- [[_COMMUNITY_Code Module 79|Code Module 79]]

## God Nodes (most connected - your core abstractions)
1. `RegionCommonValueInRange()` - 48 edges
2. `length()` - 43 edges
3. `println()` - 43 edges
4. `free()` - 40 edges
5. `begin()` - 38 edges
6. `SX126xWriteCommand()` - 33 edges
7. `close()` - 31 edges
8. `RegionCommonChanMaskCopy()` - 28 edges
9. `randr()` - 23 edges
10. `SX1276Write()` - 23 edges

## Surprising Connections (you probably didn't know these)
- `runConfigFile()` --calls--> `readBytes()`  [INFERRED]
  src/hardware/Controller.cpp → lib/ESPAsyncTCP/src/ESPAsyncTCPbuffer.cpp
- `getSDInfo()` --calls--> `cardType()`  [INFERRED]
  src/hardware/Logger.cpp → lib/SD/src/SD.cpp
- `getSDInfo()` --calls--> `cardSize()`  [INFERRED]
  src/hardware/Logger.cpp → lib/SD/src/SD.cpp
- `connect()` --calls--> `println()`  [INFERRED]
  lib/AsyncTCP/src/AsyncTCP.cpp → src/hardware/Logger.cpp
- `reconnect()` --calls--> `flush()`  [INFERRED]
  src/MqttClient.cpp → lib/ESPAsyncTCP/src/SyncClient.cpp

## Communities

### Community 0 - "LoRaWAN Regions"
Cohesion: 0.01
Nodes (239): CountNbOfEnabledChannels(), GetBandwidth(), GetNextLowerTxDr(), LimitTxPower(), RegionAS923AdrNext(), RegionAS923ApplyCFList(), RegionAS923CalcBackOff(), RegionAS923ChanMaskSet() (+231 more)

### Community 1 - "AsyncWebServer Handlers"
Cohesion: 0.02
Nodes (200): ack(), _addClient(), AsyncEventSource(), AsyncEventSourceClient(), AsyncEventSourceMessage(), AsyncEventSourceResponse(), avgPacketsWaiting(), canHandle() (+192 more)

### Community 2 - "AES Cryptography"
Cohesion: 0.03
Nodes (165): lorawan_aes_set_key(), printf(), AES_CMAC_Final(), AES_CMAC_Init(), AES_CMAC_SetKey(), AES_CMAC_Update(), AddMacCommand(), beforeTxCadDone() (+157 more)

### Community 3 - "RTC & WiFi Core"
Cohesion: 0.03
Nodes (148): buildFallbackDateTime(), checkAndInsertBottomTemps(), connectToWiFi(), Controller(), DEBUG(), ERROR(), forceNTPSync(), getAvgBottomTemp() (+140 more)

### Community 4 - "SX126x LoRa Radio"
Cohesion: 0.05
Nodes (105): GpioWrite(), RadioGetFskBandwidthRegValue(), RadioGetStatus(), RadioGetWakeupTime(), RadioInit(), RadioIrqProcess(), RadioIsChannelFree(), RadioOnCadTimeoutIrq() (+97 more)

### Community 5 - "TCP Socket Layer"
Cohesion: 0.03
Nodes (73): abort(), _accept(), _accepted(), ack(), ackPacket(), add(), _async_service_task(), AsyncClient() (+65 more)

### Community 6 - "DEPG0290 Display"
Cohesion: 0.04
Nodes (68): connect(), DEPG0290BxS800FxX_BW(), display(), sendCommand(), sendInitCommands(), sendScreenRotateCommand(), stop(), WaitUntilIdle() (+60 more)

### Community 7 - "Async Network I/O"
Cohesion: 0.04
Nodes (64): abort(), ACErrorTracker(), AsyncClient(), AsyncServer(), begin(), beginSecure(), _cert(), clearTcpCallbacks() (+56 more)

### Community 8 - "MCU Board Control"
Cohesion: 0.05
Nodes (59): BoardDisableIrq(), BoardEnableIrq(), GetBoardPowerSource(), DelayMs(), GpioMcuInit(), GpioMcuRead(), GpioMcuRemoveInterrupt(), GpioMcuSetInterrupt() (+51 more)

### Community 9 - "DEPG0150 Display"
Cohesion: 0.04
Nodes (61): connect(), DEPG0150BxS810FxX_BW(), display(), sendCommand(), sendData(), sendInitCommands(), stop(), WaitUntilIdle() (+53 more)

### Community 10 - "SD Card I/O"
Cohesion: 0.11
Nodes (37): begin(), cardSize(), cardType(), CRC16(), CRC7(), ff_sd_initialize(), ff_sd_ioctl(), ff_sd_read() (+29 more)

### Community 11 - "AsyncTCP Buffer"
Cohesion: 0.11
Nodes (30): AsyncTCPbuffer(), _attachCallbacks(), close(), connected(), flush(), _handleRxBuffer(), onData(), onDisconnect() (+22 more)

### Community 12 - "AES Crypto Utils"
Cohesion: 0.18
Nodes (21): add_round_key(), aes_cbc_decrypt(), aes_decrypt(), aes_decrypt_128(), aes_decrypt_256(), aes_encrypt_128(), aes_encrypt_256(), copy_and_key() (+13 more)

### Community 13 - "External Libraries"
Cohesion: 0.14
Nodes (18): AsyncTCP Library, ESPAsyncTCP Library, ESPAsyncWebServer, DS3231 RTC, ESP32-S3 (Heltec WiFi LoRa 32 V3), I2C Bus Recovery, Non-Blocking Stage Init, NTP Synchronization (+10 more)

### Community 14 - "HTTP Responses"
Cohesion: 0.25
Nodes (0): 

### Community 15 - "Web Parameters"
Cohesion: 0.4
Nodes (1): AsyncWebServerRequest()

### Community 16 - "Web Response Core"
Cohesion: 0.5
Nodes (2): AsyncAbstractResponse(), AsyncStreamResponse()

### Community 17 - "Web Handler Core"
Cohesion: 0.67
Nodes (0): 

### Community 18 - "WebSerial Frontend"
Cohesion: 1.0
Nodes (2): addLineBreaks(), chunkArray()

### Community 19 - "SD Card Header"
Cohesion: 1.0
Nodes (0): 

### Community 20 - "Debug Utilities"
Cohesion: 1.0
Nodes (0): 

### Community 21 - "SPIFFS Editor"
Cohesion: 1.0
Nodes (0): 

### Community 22 - "WebSerialLite Lib"
Cohesion: 1.0
Nodes (0): 

### Community 23 - "LoRaWan App Class"
Cohesion: 1.0
Nodes (0): 

### Community 24 - "MCU Class Wrapper"
Cohesion: 1.0
Nodes (0): 

### Community 25 - "Thawing Room Core"
Cohesion: 1.0
Nodes (0): 

### Community 26 - "Stage Management"
Cohesion: 1.0
Nodes (0): 

### Community 27 - "Temperature Sensors"
Cohesion: 1.0
Nodes (0): 

### Community 28 - "Display Drivers"
Cohesion: 1.0
Nodes (0): 

### Community 29 - "Config Management"
Cohesion: 1.0
Nodes (0): 

### Community 30 - "Code Module 30"
Cohesion: 1.0
Nodes (0): 

### Community 31 - "Code Module 31"
Cohesion: 1.0
Nodes (0): 

### Community 32 - "Code Module 32"
Cohesion: 1.0
Nodes (0): 

### Community 33 - "Code Module 33"
Cohesion: 1.0
Nodes (0): 

### Community 34 - "Code Module 34"
Cohesion: 1.0
Nodes (0): 

### Community 35 - "Code Module 35"
Cohesion: 1.0
Nodes (0): 

### Community 36 - "Code Module 36"
Cohesion: 1.0
Nodes (0): 

### Community 37 - "Code Module 37"
Cohesion: 1.0
Nodes (0): 

### Community 38 - "Code Module 38"
Cohesion: 1.0
Nodes (0): 

### Community 39 - "Code Module 39"
Cohesion: 1.0
Nodes (0): 

### Community 40 - "Code Module 40"
Cohesion: 1.0
Nodes (0): 

### Community 41 - "Code Module 41"
Cohesion: 1.0
Nodes (0): 

### Community 42 - "Code Module 42"
Cohesion: 1.0
Nodes (0): 

### Community 43 - "Code Module 43"
Cohesion: 1.0
Nodes (0): 

### Community 44 - "Code Module 44"
Cohesion: 1.0
Nodes (0): 

### Community 45 - "Code Module 45"
Cohesion: 1.0
Nodes (0): 

### Community 46 - "Code Module 46"
Cohesion: 1.0
Nodes (0): 

### Community 47 - "Code Module 47"
Cohesion: 1.0
Nodes (0): 

### Community 48 - "Code Module 48"
Cohesion: 1.0
Nodes (0): 

### Community 49 - "Code Module 49"
Cohesion: 1.0
Nodes (0): 

### Community 50 - "Code Module 50"
Cohesion: 1.0
Nodes (0): 

### Community 51 - "Code Module 51"
Cohesion: 1.0
Nodes (0): 

### Community 52 - "Code Module 52"
Cohesion: 1.0
Nodes (0): 

### Community 53 - "Code Module 53"
Cohesion: 1.0
Nodes (0): 

### Community 54 - "Code Module 54"
Cohesion: 1.0
Nodes (0): 

### Community 55 - "Code Module 55"
Cohesion: 1.0
Nodes (0): 

### Community 56 - "Code Module 56"
Cohesion: 1.0
Nodes (0): 

### Community 57 - "Code Module 57"
Cohesion: 1.0
Nodes (0): 

### Community 58 - "Code Module 58"
Cohesion: 1.0
Nodes (0): 

### Community 59 - "Code Module 59"
Cohesion: 1.0
Nodes (0): 

### Community 60 - "Code Module 60"
Cohesion: 1.0
Nodes (0): 

### Community 61 - "Code Module 61"
Cohesion: 1.0
Nodes (0): 

### Community 62 - "Code Module 62"
Cohesion: 1.0
Nodes (0): 

### Community 63 - "Code Module 63"
Cohesion: 1.0
Nodes (0): 

### Community 64 - "Code Module 64"
Cohesion: 1.0
Nodes (0): 

### Community 65 - "Code Module 65"
Cohesion: 1.0
Nodes (0): 

### Community 66 - "Code Module 66"
Cohesion: 1.0
Nodes (0): 

### Community 67 - "Code Module 67"
Cohesion: 1.0
Nodes (0): 

### Community 68 - "Code Module 68"
Cohesion: 1.0
Nodes (0): 

### Community 69 - "Code Module 69"
Cohesion: 1.0
Nodes (0): 

### Community 70 - "Code Module 70"
Cohesion: 1.0
Nodes (0): 

### Community 71 - "Code Module 71"
Cohesion: 1.0
Nodes (0): 

### Community 72 - "Code Module 72"
Cohesion: 1.0
Nodes (0): 

### Community 73 - "Code Module 73"
Cohesion: 1.0
Nodes (0): 

### Community 74 - "Code Module 74"
Cohesion: 1.0
Nodes (0): 

### Community 75 - "Code Module 75"
Cohesion: 1.0
Nodes (0): 

### Community 76 - "Code Module 76"
Cohesion: 1.0
Nodes (0): 

### Community 77 - "Code Module 77"
Cohesion: 1.0
Nodes (0): 

### Community 78 - "Code Module 78"
Cohesion: 1.0
Nodes (0): 

### Community 79 - "Code Module 79"
Cohesion: 1.0
Nodes (0): 

## Knowledge Gaps
- **8 isolated node(s):** `ESP32-S3 (Heltec WiFi LoRa 32 V3)`, `Watchdog Timer (5 min timeout)`, `I2C Bus Recovery`, `Non-Blocking Stage Init`, `Safe Actuator State on Boot` (+3 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `SD Card Header`** (2 nodes): `SD.h`, `fs()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Debug Utilities`** (2 nodes): `debugTimeStamp()`, `DebugPrintMacros.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `SPIFFS Editor`** (2 nodes): `SPIFFSEditor.h`, `AsyncWebHandler()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `WebSerialLite Lib`** (2 nodes): `WebSerialLite.h`, `WebSerialClass()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `LoRaWan App Class`** (2 nodes): `LoRaWan_APP.h`, `LoRaWanClass()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `MCU Class Wrapper`** (2 nodes): `McuClass()`, `ESP32_Mcu.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Thawing Room Core`** (2 nodes): `MqttClient()`, `MqttClient.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Stage Management`** (2 nodes): `WIFI.h`, `WIFI()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Temperature Sensors`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Display Drivers`** (1 nodes): `types.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Config Management`** (1 nodes): `sd_defines.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 30`** (1 nodes): `sd_diskio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 31`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 32`** (1 nodes): `config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 33`** (1 nodes): `tcp_axtls.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 34`** (1 nodes): `async_config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 35`** (1 nodes): `WebAuthentication.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 36`** (1 nodes): `HT_DisplayFonts.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 37`** (1 nodes): `WebSerialWebPage.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 38`** (1 nodes): `ESP32_LoRaWan_102.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 39`** (1 nodes): `radio_sx127x.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 40`** (1 nodes): `radio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 41`** (1 nodes): `LoRaMacTest.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 42`** (1 nodes): `LoRaMacClassBConfig.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 43`** (1 nodes): `LoRaMacClassB.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 44`** (1 nodes): `utilities.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 45`** (1 nodes): `LoRaMacConfirmQueue.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 46`** (1 nodes): `Commissioning.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 47`** (1 nodes): `LoRaMac.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 48`** (1 nodes): `aes.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 49`** (1 nodes): `LoRaMacCrypto.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 50`** (1 nodes): `cmac.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 51`** (1 nodes): `RegionEU433.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 52`** (1 nodes): `RegionKR920.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 53`** (1 nodes): `RegionEU868.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 54`** (1 nodes): `RegionCommon.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 55`** (1 nodes): `RegionUS915.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 56`** (1 nodes): `RegionIN865.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 57`** (1 nodes): `RegionAU915.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 58`** (1 nodes): `RegionUS915-Hybrid.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 59`** (1 nodes): `RegionAS923.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 60`** (1 nodes): `RegionCN470.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 61`** (1 nodes): `Region.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 62`** (1 nodes): `RegionCN779.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 63`** (1 nodes): `sx126x-board.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 64`** (1 nodes): `debug.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 65`** (1 nodes): `sx1276Regs-Fsk.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 66`** (1 nodes): `sx1276-board.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 67`** (1 nodes): `sx1276.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 68`** (1 nodes): `gpio.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 69`** (1 nodes): `rtc-board.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 70`** (1 nodes): `board-config.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 71`** (1 nodes): `lorawan_spi.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 72`** (1 nodes): `delay.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 73`** (1 nodes): `sx126x.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 74`** (1 nodes): `board.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 75`** (1 nodes): `sx1276Regs-LoRa.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 76`** (1 nodes): `gpio-board.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 77`** (1 nodes): `Thawing-room-chicano.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 78`** (1 nodes): `WebFiles.h`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Code Module 79`** (1 nodes): `WebFiles.cpp`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `randr()` connect `LoRaWAN Regions` to `AES Cryptography`?**
  _High betweenness centrality (0.259) - this node is a cross-community bridge._
- **Why does `println()` connect `RTC & WiFi Core` to `AsyncWebServer Handlers`, `AES Cryptography`, `TCP Socket Layer`?**
  _High betweenness centrality (0.232) - this node is a cross-community bridge._
- **Why does `ifskipjoin()` connect `AES Cryptography` to `LoRaWAN Regions`, `RTC & WiFi Core`?**
  _High betweenness centrality (0.203) - this node is a cross-community bridge._
- **Are the 46 inferred relationships involving `RegionCommonValueInRange()` (e.g. with `CountNbOfEnabledChannels()` and `RegionUS915HybridVerify()`) actually correct?**
  _`RegionCommonValueInRange()` has 46 INFERRED edges - model-reasoned connections that need verification._
- **Are the 42 inferred relationships involving `length()` (e.g. with `write()` and `AsyncEventSourceClient()`) actually correct?**
  _`length()` has 42 INFERRED edges - model-reasoned connections that need verification._
- **Are the 37 inferred relationships involving `println()` (e.g. with `connect()` and `begin()`) actually correct?**
  _`println()` has 37 INFERRED edges - model-reasoned connections that need verification._
- **Are the 39 inferred relationships involving `free()` (e.g. with `sdcard_uninit()` and `sdcard_mount()`) actually correct?**
  _`free()` has 39 INFERRED edges - model-reasoned connections that need verification._