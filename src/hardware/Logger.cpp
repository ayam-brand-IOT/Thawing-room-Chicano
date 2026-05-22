#include "Logger.h"

Logger logger; 

Logger::Logger() : currentOutput(HW_SERIAL) {} // Inicializa con SERIAL por defecto

void Logger::init(unsigned long baudRate) {
    if (currentOutput == HW_SERIAL) {
        Serial.begin(baudRate);
    } else {
        // find way to init the webserial from here
    }
}

void Logger::setFileName(DateTime now) {
    filename = "/log_" + String(now.year()) + "_" + String(now.month()) + "_" + String(now.day()) + "_" + String(now.hour()) + "-" + String(now.minute()) + ".txt";
}

String Logger::getFileName() {
    return filename;
}

void Logger::writeSD(const String &message, DateTime now) {
    if (!theresSD) {
        Serial.println("No SD card found");
        return;
    }

    if (!SD.exists(LOG_FOLDER_PATH)) SD.mkdir(LOG_FOLDER_PATH);
    
    const String path = LOG_FOLDER_PATH + filename;

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

   // print message with timestamp
    file.print(now.year(), DEC);
    file.print('/');
    file.print(now.month(), DEC);
    file.print('/');
    file.print(now.day(), DEC);
    file.print(" ");
    file.print(now.hour(), DEC);
    file.print(':');
    file.print(now.minute(), DEC);
    file.print(':');
    file.print(now.second(), DEC);
    file.print(" - ");
    file.println(message);

    file.flush();
}

void Logger::setOutput(OutputType output) {
    currentOutput = output;
}

void Logger::print(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial.print(message);
    } else {
        WebSerial.print(message);
    }
}

void Logger::println(const String &message) {
    if (currentOutput == HW_SERIAL) {
        Serial.println(message);
    } else {
        WebSerial.println(message);
    }
}

void Logger::printError(uint8_t errorType) {
    const String message = errorMessages[errorType];
    if (currentOutput == WEBSERIAL) {
        WebSerial.println("[ERROR -> LOGGER]: " + message);
    } 
    Serial.println("[ERROR -> LOGGER]: " + message);
}

void Logger::printError(const String &message) {
    if (currentOutput == WEBSERIAL) {
        WebSerial.println("[ERROR " + message);
    } 
    Serial.println("[ERROR " + message);
}

void Logger::printValue(const String &key, const String &value) {
    if (currentOutput == HW_SERIAL) {
        Serial.println(key + ": " + value);
    } else {
        WebSerial.println(key + ": " + value);
    }
}

void Logger::printTime(const String &prefix, int hour, int minute, int day, int month) {
    String timeStr = prefix + ": " + 
                     String(hour) + "h " + 
                     String(minute) + "min " + 
                     String(day) + "day " + 
                     String(month) + "month";
    if (currentOutput == HW_SERIAL) {
        Serial.println(timeStr);
    } else {
        WebSerial.println(timeStr);
    }
}

void Logger::setupSD() {
    SPI.begin(SCK, MISO, MOSI, SS);
    pinMode(SS, OUTPUT);
    digitalWrite(SS, HIGH); // Set CS to HIGH initially (inverted logic)

    // Manually control CS for the SD card initialization
    digitalWrite(SS, LOW); // Set CS LOW to select the SD card (inverted logic)
    uint8_t attempts = 0;
    while (attempts < 3 && !SD.begin(SS)) {
        printError(NOT_SDCARD);
        digitalWrite(SS, HIGH); // Set CS HIGH to deselect the SD card (inverted logic)
        attempts++;
        delay(2000);
        digitalWrite(SS, LOW);
    }

    if (SD.begin(SS)) {
        theresSD = true;
    } else {
        theresSD = false;
    }

    digitalWrite(SS, HIGH); // Set CS HIGH to deselect the SD card (inverted logic)
}

bool Logger::isSdAvailable() {
    return theresSD;
}

void Logger::getSDInfo() {
    if (!theresSD) {
        Serial.println("No SD card found");
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No CARD");  
        return;
    }

    Serial.print("Card type:");

    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } 
    else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } 
    else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } 
    else {
        Serial.println("UNKNOWN");  
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("Card capacity: %lluMB\n", cardSize);
}

