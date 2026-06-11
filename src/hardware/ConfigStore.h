#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <FS.h>
#include <SD.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * ConfigStore — persistencia de configuración en SPIFFS (memoria interna del ESP32).
 *
 * Motivación: las SD industriales fallan. La configuración crítica (config.txt y
 * defaultParameters.txt) vive ahora en SPIFFS; la SD queda solo para logs.
 *
 * Garantías de robustez:
 *  - Escritura atómica: se escribe a "<path>.tmp", se valida que sea JSON parseable,
 *    el archivo actual se respalda a "<path>.bak" y recién entonces se promueve el .tmp.
 *    Un corte de energía a mitad de escritura nunca deja el archivo bueno corrupto.
 *  - Lectura con fallback: si "<path>" no existe o está corrupto, se recupera "<path>.bak".
 *  - Migración única SD->SPIFFS: en equipos ya desplegados, copia los archivos desde
 *    la SD a SPIFFS la primera vez que arrancan con este firmware.
 */
class ConfigStore {
public:
    // Monta SPIFFS (formatea en el primer arranque si hace falta). Devuelve true si quedó montado.
    static bool begin();

    // ¿Existe el archivo (principal o respaldo) en SPIFFS?
    static bool exists(const char* path);

    // Lee el contenido. Si el principal falla la validación JSON, intenta el .bak
    // y, si éste es válido, lo restaura como principal. Devuelve "" si no hay nada usable.
    static String read(const char* path);

    // Escritura atómica con validación JSON y respaldo .bak. Devuelve true si quedó persistido.
    static bool write(const char* path, const String& data);

    // Copia 'path' desde la SD a SPIFFS solo si SPIFFS aún no lo tiene y la SD sí.
    // Devuelve true si se migró un archivo en esta llamada.
    static bool migrateFromSD(const char* path);

private:
    static bool isValidJson(const String& data);
    static String readRaw(const char* path);
};

#endif // CONFIG_STORE_H
