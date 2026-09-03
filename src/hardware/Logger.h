#ifndef LOGGER_H
#define LOGGER_H

#include "SD.h"
#include <RTClib.h>
#include <Arduino.h>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

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

// ============================================================================
// Logger con niveles, procedencia y escritor único.
//
// El problema que resuelve: se loguea desde tres tasks a la vez -el loop de
// control (core 1), backgroundTasks (core 0) y los handlers del AsyncWebServer-
// y la SD cuelga de un bus SPI que no se puede tocar desde varias a la vez.
// Además `Serial.println()` BLOQUEA: uart_write_bytes() espera lugar en el ring
// buffer de TX y toma un mutex global de UART, así que un log puede frenar el
// lazo de control.
//
// Diseño:
//   log()  -> formatea, escribe a Serial SIN BLOQUEAR, y encola para la SD
//   drain() -> lo llama UNA sola task (backgroundTasks, core 0) y es la única
//              que toca la SD. Sin concurrencia sobre el bus no hace falta mutex.
//
// La cola nunca bloquea al que loguea: si está llena se descarta la línea y se
// cuenta. Frenar el proceso para no perder un log sería el error opuesto.
// ============================================================================

enum LogLevel : uint8_t {
    LOG_LVL_DEBUG = 0,
    LOG_LVL_INFO,
    LOG_LVL_WARN,
    LOG_LVL_ERROR,
    LOG_LVL_CRIT,
    LOG_LVL_NONE     // como umbral: apaga el destino por completo
};

// Tags de procedencia. Se recortan a LOG_TAG_MAX-1 al formatear.
#define TAG_APP   "APP"
#define TAG_ADC   "ADC"
#define TAG_MQTT  "MQTT"
#define TAG_WIFI  "WIFI"
#define TAG_RTC   "RTC"
#define TAG_CFG   "CFG"
#define TAG_STAGE "STAGE"
#define TAG_CTRL  "CTRL"
#define TAG_SD    "SD"
#define TAG_LOG   "LOG"

#define LOG_TAG_MAX     8
#define LOG_MSG_MAX     144
#define LOG_QUEUE_LEN   32     // ~5 KB de RAM con el record completo
#define LOG_DRAIN_BATCH 8      // registros por llamada a drain(): acota el I/O por vuelta
#define LOG_MAX_FILE_BYTES (1024UL * 1024UL)   // 1 MB por archivo antes de rotar

struct LogRecord {
    uint32_t ms;                  // millis() al ENCOLAR, no al escribir
    uint8_t  level;
    char     tag[LOG_TAG_MAX];
    char     msg[LOG_MSG_MAX];
};

class Logger {
public:
    enum OutputType { HW_SERIAL, WEBSERIAL };

private:
    enum ErrorType { NOT_SDCARD, NUM_ERRORS };
    String filename = DEFAULT_LOG_FILE;
    bool theresSD = false;
    const String errorMessages[NUM_ERRORS] = {ERR_NOT_SDCARD};

    QueueHandle_t queue = nullptr;
    LogLevel serial_level = LOG_LVL_DEBUG;
    LogLevel sd_level     = LOG_LVL_INFO;
    bool     serial_on    = true;
    uint32_t dropped_queue  = 0;   // no entró a la cola (cola llena)
    uint32_t dropped_serial = 0;   // no entró al TX de UART (buffer lleno)
    uint32_t last_drop_report_ms = 0;

    // Instantánea del reloj: epoch + el millis() en que se tomó. La alimenta el loop
    // de control (core 1) vía setClock(); drain() la usa para fechar sin tocar el RTC.
    //
    // Antes esto era un std::function que llamaba a getDateTime() desde drain(), o sea
    // desde el core 0: eso metía I2C en el camino del log y, peor, mutaba estado de
    // Controller (rtc_last_valid_*, reintentos de reconexión) desde dos cores sin
    // sincronizar. El bus I2C tiene su propio mutex, pero esos miembros no.
    uint32_t clock_epoch = 0;   // 0 = todavía sin hora de pared
    uint32_t clock_ms    = 0;
    portMUX_TYPE clock_mux = portMUX_INITIALIZER_UNLOCKED;

    void writeSerial(const LogRecord &rec);
    void writeRecordToSD(const LogRecord &rec, uint32_t epoch, uint32_t epoch_ms, File &file);
    void rotateIfNeeded(const String &path);
    static const char* levelName(uint8_t level);

public:
    OutputType currentOutput;

    Logger();

    // --- API nueva ---------------------------------------------------------
    // Crea la cola. Llamar una vez en setup(), antes de loguear en serio.
    bool begin();
    // Formatea estilo printf con vsnprintf acotado: reemplaza los ~30 sitios de
    // `char buffer[N]; sprintf(...)` repartidos por el proyecto, que es de donde
    // salen los overflows.
    void log(LogLevel level, const char* tag, const char* fmt, ...);
    // La llama SOLO backgroundTasks (core 0). Única puerta de escritura a la SD.
    void drain();
    // La llama el loop de control (core 1), que ya lee getDateTime() cada vuelta.
    // El drenador fecha con esta instantánea en vez de tocar el RTC desde el core 0.
    void setClock(const DateTime &now);
    void setSerialLevel(LogLevel level) { serial_level = level; }
    void setSDLevel(LogLevel level)     { sd_level = level; }
    // Apagar Serial en equipos donde nadie enchufa un cable: en esta placa Serial
    // es UART0 y no hay forma de detectar si alguien escucha (operator bool está
    // hardcodeado a true), así que se decide por configuración.
    void setSerialEnabled(bool enabled) { serial_on = enabled; }
    bool isSerialEnabled() const        { return serial_on; }
    uint32_t getDroppedQueue()  const   { return dropped_queue; }
    uint32_t getDroppedSerial() const   { return dropped_serial; }

    // --- API vieja (se mantiene: migración incremental) ---------------------
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

// Macros de conveniencia. El tag va explícito como primer argumento: sin reglas
// de orden de include ni variables mágicas, y se puede grepear.
#define LOG_D(tag, ...) logger.log(LOG_LVL_DEBUG, tag, __VA_ARGS__)
#define LOG_I(tag, ...) logger.log(LOG_LVL_INFO,  tag, __VA_ARGS__)
#define LOG_W(tag, ...) logger.log(LOG_LVL_WARN,  tag, __VA_ARGS__)
#define LOG_E(tag, ...) logger.log(LOG_LVL_ERROR, tag, __VA_ARGS__)
#define LOG_C(tag, ...) logger.log(LOG_LVL_CRIT,  tag, __VA_ARGS__)

#endif // LOGGER_H
