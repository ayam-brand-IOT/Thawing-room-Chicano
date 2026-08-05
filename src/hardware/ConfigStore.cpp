#include "ConfigStore.h"
#include "Logger.h"

SemaphoreHandle_t ConfigStore::mutex = nullptr;

namespace {
    // RAII: toma el mutex recursivo en el ctor, lo libera en el dtor. Null-safe: si
    // el mutex aún no existe (no se llamó begin()), degrada a sin-lock en vez de crashear.
    struct ScopedLock {
        SemaphoreHandle_t m;
        bool taken;
        explicit ScopedLock(SemaphoreHandle_t mtx) : m(mtx), taken(false) {
            if (m) taken = (xSemaphoreTakeRecursive(m, portMAX_DELAY) == pdTRUE);
        }
        ~ScopedLock() { if (m && taken) xSemaphoreGiveRecursive(m); }
        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
    };
}

bool ConfigStore::begin() {
    // Crear el mutex recursivo antes de cualquier acceso concurrente (aquí el arranque
    // es single-thread; las tasks que compiten arrancan después).
    if (mutex == nullptr) mutex = xSemaphoreCreateRecursiveMutex();

    // format_on_fail = true: en el primer arranque (SPIFFS vacío) lo formatea automáticamente.
    if (!SPIFFS.begin(true)) {
        logger.println("[ConfigStore] SPIFFS mount failed");
        return false;
    }
    logger.println("[ConfigStore] SPIFFS mounted");
    return true;
}

bool ConfigStore::isValidJson(const String& data) {
    if (data.length() == 0) return false;
    StaticJsonDocument<2048> doc;
    return deserializeJson(doc, data) == DeserializationError::Ok;
}

String ConfigStore::readRaw(const char* path) {
    if (!SPIFFS.exists(path)) return String();
    File f = SPIFFS.open(path, FILE_READ);
    if (!f) return String();
    String content = f.readString();
    f.close();
    return content;
}

bool ConfigStore::exists(const char* path) {
    if (SPIFFS.exists(path)) return true;
    String bak = String(path) + ".bak";
    return SPIFFS.exists(bak);
}

String ConfigStore::read(const char* path) {
    ScopedLock lock(mutex);  // serializa con write() de los handlers web en otra task

    // 1. Intento normal sobre el archivo principal
    String content = readRaw(path);
    if (isValidJson(content)) return content;

    // 2. El principal falta o está corrupto: intentar el respaldo
    String bakPath = String(path) + ".bak";
    String bak = readRaw(bakPath.c_str());
    if (isValidJson(bak)) {
        logger.println(("[ConfigStore] Primary corrupt, restoring backup: " + String(path)).c_str());
        write(path, bak);  // re-promueve el respaldo a principal
        return bak;
    }

    logger.println(("[ConfigStore] No usable data for: " + String(path)).c_str());
    return String();
}

bool ConfigStore::write(const char* path, const String& data) {
    ScopedLock lock(mutex);  // serializa con read()/write() concurrentes (recursivo: read() puede llamar aquí)

    // Validar ANTES de tocar el disco: nunca persistir basura.
    if (!isValidJson(data)) {
        logger.println(("[ConfigStore] Refusing to write invalid JSON to: " + String(path)).c_str());
        return false;
    }

    String tmpPath = String(path) + ".tmp";
    String bakPath = String(path) + ".bak";

    // 1. Escribir a archivo temporal
    File tmp = SPIFFS.open(tmpPath, FILE_WRITE);
    if (!tmp) {
        logger.println(("[ConfigStore] Failed to open tmp: " + tmpPath).c_str());
        return false;
    }
    const size_t written = tmp.print(data);
    tmp.close();

    if (written != data.length()) {
        logger.println("[ConfigStore] Short write to tmp, aborting");
        SPIFFS.remove(tmpPath);
        return false;
    }

    // 2. Verificar que el temporal se lea de vuelta como JSON válido
    if (!isValidJson(readRaw(tmpPath.c_str()))) {
        logger.println("[ConfigStore] tmp failed read-back validation, aborting");
        SPIFFS.remove(tmpPath);
        return false;
    }

    // 3. Respaldar el archivo bueno actual (si existe)
    if (SPIFFS.exists(path)) {
        SPIFFS.remove(bakPath);              // rename falla si el destino existe
        SPIFFS.rename(path, bakPath);
    }

    // 4. Promover el temporal a principal
    if (!SPIFFS.rename(tmpPath, path)) {
        logger.println("[ConfigStore] Promote failed, restoring backup");
        SPIFFS.remove(tmpPath);
        if (SPIFFS.exists(bakPath)) SPIFFS.rename(bakPath, path);  // restaurar
        return false;
    }

    return true;
}

bool ConfigStore::migrateFromSD(const char* path) {
    if (SPIFFS.exists(path)) return false;   // ya está en SPIFFS: nada que migrar
    if (!SD.exists(path))    return false;   // la SD no lo tiene (o no hay SD)

    File src = SD.open(path, FILE_READ);
    if (!src) return false;
    String content = src.readString();
    src.close();

    if (content.length() == 0) return false;

    if (write(path, content)) {
        logger.println(("[ConfigStore] Migrated SD -> SPIFFS: " + String(path)).c_str());
        return true;
    }
    return false;
}
