#include "Logger.h"
#include <stdarg.h>

Logger logger;

Logger::Logger() : currentOutput(HW_SERIAL) {} // Inicializa con SERIAL por defecto

// ============================================================================
// Logger con niveles / procedencia / escritor único (ver el comentario grande
// en Logger.h para el porqué del diseño).
// ============================================================================

bool Logger::begin() {
    if (queue != nullptr) return true;
    queue = xQueueCreate(LOG_QUEUE_LEN, sizeof(LogRecord));
    if (queue == nullptr) {
        Serial.println("[LOG ] no se pudo crear la cola: los logs no llegaran a la SD");
        return false;
    }
    return true;
}

void Logger::setClock(const DateTime &now, bool trusted) {
    // Sección crítica mínima: epoch y ms son dos palabras y drain() las lee desde el
    // otro core. Sin esto un lote podría fecharse con un epoch nuevo y un ms viejo.
    portENTER_CRITICAL(&clock_mux);
    clock_epoch   = now.unixtime();
    clock_ms      = millis();
    clock_trusted = trusted;
    portEXIT_CRITICAL(&clock_mux);
}

uint32_t Logger::epochAt(uint32_t ms, bool *trusted) {
    portENTER_CRITICAL(&clock_mux);
    const uint32_t epoch    = clock_epoch;
    const uint32_t epoch_ms = clock_ms;
    const bool     ok       = clock_trusted;
    portEXIT_CRITICAL(&clock_mux);

    if (trusted) *trusted = ok && epoch != 0;
    if (epoch == 0) return 0;
    // Con signo por lo mismo que en writeRecordToSD(): `ms` puede ser posterior a la
    // instantánea (se encoló entre dos vueltas del loop).
    const int32_t delta_ms = (int32_t)(ms - epoch_ms);
    return (uint32_t)((int64_t)epoch + delta_ms / 1000);
}

const char* Logger::levelName(uint8_t level) {
    switch (level) {
        case LOG_LVL_DEBUG: return "DEBUG";
        case LOG_LVL_INFO:  return "INFO ";
        case LOG_LVL_WARN:  return "WARN ";
        case LOG_LVL_ERROR: return "ERROR";
        case LOG_LVL_CRIT:  return "CRIT ";
        default:            return "?????";
    }
}

void Logger::log(LogLevel level, const char* tag, const char* fmt, ...) {
    // Si ningún destino lo quiere, ni se formatea.
    const bool to_serial = serial_on && level >= serial_level;
    const bool to_sd     = level >= sd_level;
    if (!to_serial && !to_sd) return;

    LogRecord rec;
    rec.ms    = millis();
    rec.level = (uint8_t)level;
    strlcpy(rec.tag, tag ? tag : TAG_APP, sizeof(rec.tag));

    va_list args;
    va_start(args, fmt);
    vsnprintf(rec.msg, sizeof(rec.msg), fmt, args);   // acotado: nunca desborda
    va_end(args);

    if (to_serial) writeSerial(rec);

    // Encolado NO bloqueante (timeout 0): perder una línea es preferible a frenar
    // el lazo de control esperando lugar en la cola.
    if (to_sd && queue != nullptr) {
        if (xQueueSend(queue, &rec, 0) != pdTRUE) dropped_queue++;
    }
}

void Logger::writeSerial(const LogRecord &rec) {
    char line[LOG_TAG_MAX + LOG_MSG_MAX + 32];
    const int len = snprintf(line, sizeof(line), "%-5s [%-5s] %s\r\n",
                             levelName(rec.level), rec.tag, rec.msg);
    if (len <= 0) return;

    // NO bloqueante: uart_write_bytes() espera lugar en el ring de TX y toma un
    // mutex global de UART, así que escribir a ciegas puede frenar la task. Si no
    // hay lugar se descarta y se cuenta.
    if (Serial.availableForWrite() < len) { dropped_serial++; return; }
    Serial.write((const uint8_t*)line, (size_t)len);
}

void Logger::drain() {
    if (queue == nullptr) return;

    // Reportar descartes de vez en cuando: el silencio no debe confundirse con calma.
    const uint32_t now_ms = millis();
    if ((dropped_queue || dropped_serial) && (now_ms - last_drop_report_ms) > 60000) {
        last_drop_report_ms = now_ms;
        const uint32_t dq = dropped_queue, ds = dropped_serial;
        dropped_queue = dropped_serial = 0;
        log(LOG_LVL_WARN, TAG_LOG, "descartadas %lu lineas (cola) y %lu (serial)",
            (unsigned long)dq, (unsigned long)ds);
    }

    if (!theresSD) {   // sin tarjeta: vaciar igual para que la cola no se tape
        LogRecord discard;
        while (xQueueReceive(queue, &discard, 0) == pdTRUE) { }
        return;
    }

    if (uxQueueMessagesWaiting(queue) == 0) return;

    // Instantánea del reloj tomada por el loop. Cero I2C acá.
    portENTER_CRITICAL(&clock_mux);
    const uint32_t epoch    = clock_epoch;
    const uint32_t epoch_ms = clock_ms;
    portEXIT_CRITICAL(&clock_mux);

    if (!ensureLogFolder()) return;   // la tarjeta pudo cambiarse en caliente
    const String path = LOG_FOLDER_PATH + filename;
    rotateIfNeeded(path);

    // UNA apertura por lote, no por línea: abrir en SD es lo caro, y con 8 registros
    // por vuelta la diferencia es de 8 aperturas a 1.
    File file = SD.open(path, FILE_APPEND);
    if (!file) return;   // sin log de error acá: drain() corre cada 50 ms y spamearía

    for (uint8_t i = 0; i < LOG_DRAIN_BATCH; i++) {
        LogRecord rec;
        if (xQueueReceive(queue, &rec, 0) != pdTRUE) break;
        writeRecordToSD(rec, epoch, epoch_ms, file);
    }

    file.close();   // el close hace el flush; sin él una caída pierde el lote
}

void Logger::rotateIfNeeded(const String &path) {
    File probe = SD.open(path, FILE_READ);
    if (!probe) return;
    const size_t size = probe.size();
    probe.close();
    if (size < LOG_MAX_FILE_BYTES) return;

    // Rotar en vez de dejar crecer sin límite: una tarjeta llena deja de loguear
    // justo cuando más falta hace.
    for (uint8_t i = 1; i < 100; i++) {
        const String rolled = path + "." + String(i);
        if (!SD.exists(rolled)) { SD.rename(path, rolled); return; }
    }
}

void Logger::writeRecordToSD(const LogRecord &rec, uint32_t epoch, uint32_t epoch_ms, File &file) {
    char line[LOG_TAG_MAX + LOG_MSG_MAX + 64];

    if (epoch == 0) {
        // Todavía sin hora de pared (arranque, RTC sin sincronizar). Fechar con el
        // uptime es más útil que escribir un 1970 que confunde al que lea el archivo.
        snprintf(line, sizeof(line), "boot+%lus              %-5s [%-5s] %s",
                 (unsigned long)(rec.ms / 1000), levelName(rec.level), rec.tag, rec.msg);
        file.println(line);
        return;
    }

    // La hora es la del ENCOLADO, no la de la escritura: se parte de la instantánea
    // del reloj y se corrige por lo que la línea esperó en la cola. Así el log dice
    // cuándo pasó y no cuándo la SD se dignó a escribir.
    //
    // El cast a int32_t es obligatorio: ambos son uint32_t y el registro puede ser MÁS
    // NUEVO que la instantánea -se encola desde el core 0 y desde los handlers web, entre
    // dos vueltas del loop-. Sin signo, esa resta da la vuelta y produce ~4294967 s, o sea
    // un salto de 49.7 días hacia atrás. El cast también deja correcto el wrap real de
    // millis() a los 49.7 días, que es el mismo mecanismo.
    int32_t waited_ms = (int32_t)(epoch_ms - rec.ms);
    if (waited_ms < 0) waited_ms = 0;   // registro posterior a la instantánea: sin corrección

    const DateTime stamp = DateTime(epoch) - TimeSpan(waited_ms / 1000);

    snprintf(line, sizeof(line), "%04d/%02d/%02d %02d:%02d:%02d  %-5s [%-5s] %s",
             stamp.year(), stamp.month(), stamp.day(),
             stamp.hour(), stamp.minute(), stamp.second(),
             levelName(rec.level), rec.tag, rec.msg);

    file.println(line);
}

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

    if (!ensureLogFolder()) return;

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

// ---------------------------------------------------------------------------
// API vieja. Ahora entra por el pipeline nuevo, así que gana tag, timestamp y
// filtrado por nivel sin tocar los ~120 call sites que ya existen.
//
// Va a DEBUG a propósito, no a INFO: con el umbral de SD en INFO estas llamadas
// siguen yendo SOLO a Serial, exactamente como antes. Nada cambia de volumen ni
// llena la tarjeta; lo que se persiste es lo que se migre explícitamente a
// LOG_I/LOG_W/LOG_E. Bajando sd_level a DEBUG se captura todo para diagnosticar.
// ---------------------------------------------------------------------------

void Logger::print(const String &message) {
    // Sin salto de línea: no puede pasar por log(), que emite registros completos.
    // Se respeta el interruptor de Serial y se sigue sin bloquear.
    if (!serial_on || serial_level > LOG_LVL_DEBUG) return;
    const int len = message.length();
    if (Serial.availableForWrite() < len) { dropped_serial++; return; }
    Serial.print(message);
}

void Logger::println(const String &message) {
    log(LOG_LVL_DEBUG, TAG_APP, "%s", message.c_str());
}

void Logger::printError(uint8_t errorType) {
    log(LOG_LVL_ERROR, TAG_LOG, "%s", errorMessages[errorType].c_str());
}

void Logger::printError(const String &message) {
    log(LOG_LVL_ERROR, TAG_APP, "%s", message.c_str());
}

void Logger::printValue(const String &key, const String &value) {
    log(LOG_LVL_DEBUG, TAG_APP, "%s: %s", key.c_str(), value.c_str());
}

void Logger::printTime(const String &prefix, int hour, int minute, int day, int month) {
    log(LOG_LVL_DEBUG, TAG_APP, "%s: %dh %dmin %dday %dmonth",
        prefix.c_str(), hour, minute, day, month);
}

bool Logger::setupSD() {
    SPI.begin(SCK, MISO, MOSI, SS);
    pinMode(SS, OUTPUT);
    digitalWrite(SS, HIGH); // Set CS to HIGH initially (inverted logic)

    // Manually control CS for the SD card initialization
    digitalWrite(SS, LOW); // Set CS LOW to select the SD card (inverted logic)

    // Reintento ACOTADO: la SD es opcional (solo logs). Si no aparece, el equipo
    // sigue operando sin ella en vez de colgarse en un loop infinito.
    const uint8_t max_attempts = 3;
    theresSD = false;
    for (uint8_t attempt = 1; attempt <= max_attempts; attempt++) {
        if (SD.begin(SS)) {
            theresSD = true;
            break;
        }
        printError(NOT_SDCARD);
        delay(500);
    }

    digitalWrite(SS, HIGH); // Set CS HIGH to deselect the SD card (inverted logic)

    if (!theresSD) {
        Serial.println("[Logger] No SD card - continuing without logging to SD");
    } else {
        ensureLogFolder();
    }
    return theresSD;
}

bool Logger::retrySD() {
    if (theresSD) return true;
    digitalWrite(SS, LOW);
    if (SD.begin(SS)) {
        theresSD = true;
        Serial.println("[Logger] SD card detected");
    }
    digitalWrite(SS, HIGH);
    if (theresSD) ensureLogFolder();
    return theresSD;
}

bool Logger::hasSD() {
    return theresSD;
}

// La carpeta se crea al montar la tarjeta, no solo cuando drain() tiene algo que
// escribir: en una SD recién formateada /logs no existía y la página de logs daba 500
// hasta la primera línea. Los escritores la vuelven a pedir por si cambiaron la tarjeta.
bool Logger::ensureLogFolder() {
    if (!theresSD) return false;
    if (SD.exists(LOG_FOLDER_PATH)) return true;
    if (SD.mkdir(LOG_FOLDER_PATH)) {
        Serial.println("[Logger] Created " LOG_FOLDER_PATH " on SD");
        return true;
    }
    Serial.println("[Logger] Could not create " LOG_FOLDER_PATH " on SD");
    return false;
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

