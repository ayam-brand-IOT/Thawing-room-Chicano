#ifndef LOGGER_H
#define LOGGER_H

#include "SD.h"
#include <RTClib.h> 
#include <Arduino.h>

#define SCK   36                //SCK on SPI3
#define MOSI  35                //MISO on SPI3 
#define MISO  37                //MOSI on SPI3
#define SS    38

// WebSerial eliminado en la migración a arduino-esp32 3.x (WebSerialLite no es
// compatible con IDF5). El logger va siempre a Serial; la opción WEBSERIAL del
// enum se mantiene por compatibilidad de API pero rutea a Serial.

#define DEFAULT_LOG_FILE "/log.txt"
#define LOG_FOLDER_PATH "/logs"

//Defining messages of ERRORS
#define ERR_NOT_SDCARD "No SD card found"

#define SD_Logs true

class Logger {
public:
    enum OutputType { HW_SERIAL, WEBSERIAL };

private:
    enum ErrorType { NOT_SDCARD, NUM_ERRORS };
    String filename = DEFAULT_LOG_FILE;
    bool theresSD = false;
    const String errorMessages[NUM_ERRORS] = {ERR_NOT_SDCARD};

protected:

public:
    OutputType currentOutput;
    
    Logger();
    bool setupSD();
    bool retrySD();
    bool hasSD();
    void getSDInfo();
    String getFileName();
    void setFileName(DateTime now);
    void writeSD(const String &message,  DateTime now);
    void init(unsigned long baudRate = 115200);
    void setOutput(OutputType output);
    void printError(uint8_t errorType);
    void printError(const String &message);
    void print(const String &message);
    void println(const String &message);
    void printValue(const String &key, const String &value);
    void printTime(const String &prefix, int hour, int minute, int day, int month);
};

extern Logger logger; 

#endif // LOGGER_H