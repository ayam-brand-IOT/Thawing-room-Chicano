#include "EventLog.h"
#include "Logger.h"
#include <SPIFFS.h>
#include <stdarg.h>

EventLog events;

#define EVENT_DRAIN_BATCH 8

static const char* const TYPE_NAMES[EV_TYPE_COUNT] = {"PROCESS", "PARAMETER", "ALERT", "ERROR", "SYSTEM"};
static const char* const SOURCE_NAMES[SRC_COUNT]   = {"MANUAL", "REMOTE", "WEB", "AUTO", "SYSTEM"};

const char* EventLog::typeName(uint8_t type)     { return type < EV_TYPE_COUNT ? TYPE_NAMES[type] : "?"; }
const char* EventLog::sourceName(uint8_t source) { return source < SRC_COUNT ? SOURCE_NAMES[source] : "?"; }

bool EventLog::begin() {
    if (queue != nullptr) return true;
    queue      = xQueueCreate(EVENT_QUEUE_LEN, sizeof(Pending));
    file_mutex = xSemaphoreCreateMutex();
    if (queue == nullptr || file_mutex == nullptr) {
        LOG_E(TAG_LOG, "EventLog: sin memoria para cola/mutex, no se registraran eventos");
        return false;
    }
    scanRing();
    return true;
}

// Recorre el anillo una vez al arrancar: la secuencia sigue donde quedó y el número
// de ciclo no se repite aunque el equipo se haya reiniciado mil veces. Son 128 KB de
// lectura secuencial, ~0.2 s, y pasa antes de armar el watchdog.
void EventLog::scanRing() {
    File f = SPIFFS.open(EVENT_RING_FILE, FILE_READ);
    if (!f) return;

    EventRecord rec;
    uint32_t max_seq = 0;
    uint16_t max_cycle = 0;
    uint8_t  last_flags = 0;
    while (f.read((uint8_t*)&rec, sizeof(rec)) == sizeof(rec)) {
        if (rec.seq == 0) continue;
        if (rec.cycle > max_cycle) max_cycle = rec.cycle;
        if (rec.seq > max_seq) { max_seq = rec.seq; last_flags = rec.flags; }
    }
    f.close();

    next_seq      = max_seq + 1;
    current_cycle = max_cycle;
    cycle_active  = (last_flags & EVF_CYCLE_ACTIVE) != 0;
    LOG_I(TAG_LOG, "EventLog: %lu eventos previos, ultimo ciclo #%u",
          (unsigned long)max_seq, (unsigned)max_cycle);
}

void EventLog::enqueue(EventType type, EventSource source, uint8_t flags, const char* fmt, va_list args) {
    Pending p;
    p.ms     = millis();
    p.type   = type;
    p.source = source;
    p.stage  = current_stage;
    p.flags  = flags | (cycle_active ? EVF_CYCLE_ACTIVE : 0);
    p.cycle  = (cycle_active || (flags & EVF_CYCLE_END)) ? current_cycle : 0;
    vsnprintf(p.msg, sizeof(p.msg), fmt, args);

    // Espejo en el log de diagnóstico: así el evento también queda en el archivo del
    // proceso en la SD, en contexto con el resto de las líneas.
    const LogLevel lvl = type == EV_ERROR ? LOG_LVL_ERROR : type == EV_ALERT ? LOG_LVL_WARN : LOG_LVL_INFO;
    logger.log(lvl, "EVENT", "%s/%s %s", typeName(type), sourceName(source), p.msg);

    // Igual que el Logger: nunca bloquear al que registra.
    if (queue == nullptr || xQueueSend(queue, &p, 0) != pdTRUE) dropped++;
}

void EventLog::emit(EventType type, EventSource source, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    enqueue(type, source, 0, fmt, args);
    va_end(args);
}

void EventLog::cycleStart(EventSource source, const char* fmt, ...) {
    current_cycle = current_cycle + 1;
    cycle_active  = true;
    va_list args;
    va_start(args, fmt);
    enqueue(EV_PROCESS, source, EVF_CYCLE_START, fmt, args);
    va_end(args);
}

void EventLog::cycleEnd(EventSource source, bool finished, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    cycle_active = false;   // antes de encolar: el registro no debe salir como "activo"
    enqueue(EV_PROCESS, source, EVF_CYCLE_END | (finished ? EVF_FINISHED : 0), fmt, args);
    va_end(args);
}

void EventLog::cycleResume(EventSource source, const char* fmt, ...) {
    // Si el anillo se perdió (SPIFFS formateado) no hay ciclo que retomar: abrir uno.
    const bool fresh = current_cycle == 0;
    if (fresh) current_cycle = 1;
    cycle_active = true;
    va_list args;
    va_start(args, fmt);
    enqueue(EV_PROCESS, source, fresh ? EVF_CYCLE_START : 0, fmt, args);
    va_end(args);
}

void EventLog::drain() {
    if (queue == nullptr || uxQueueMessagesWaiting(queue) == 0) return;

    EventRecord batch[EVENT_DRAIN_BATCH];
    uint8_t count = 0;
    Pending p;
    while (count < EVENT_DRAIN_BATCH && xQueueReceive(queue, &p, 0) == pdTRUE) {
        EventRecord &rec = batch[count++];
        memset(&rec, 0, sizeof(rec));
        bool trusted = false;
        rec.seq      = next_seq++;
        rec.epoch    = logger.epochAt(p.ms, &trusted);
        rec.uptime_s = p.ms / 1000;
        rec.type     = p.type;
        rec.source   = p.source;
        rec.stage    = p.stage;
        rec.flags    = p.flags | (trusted ? EVF_TIME_TRUSTED : 0);
        rec.cycle    = p.cycle;
        strlcpy(rec.msg, p.msg, sizeof(rec.msg));
    }

    if (xSemaphoreTake(file_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        for (uint8_t i = 0; i < count; i++) {
            if (!writeRing(batch[i])) {
                LOG_E(TAG_LOG, "EventLog: no se pudo escribir %s", EVENT_RING_FILE);
                break;
            }
        }
        xSemaphoreGive(file_mutex);
    }

    appendSD(batch, count);

    if (dropped) {
        LOG_W(TAG_LOG, "EventLog: %lu eventos descartados (cola llena)", (unsigned long)dropped);
        dropped = 0;
    }
}

// Slot fijo por secuencia: (seq-1) % capacidad. Al dar la vuelta se pisa el más viejo.
bool EventLog::writeRing(const EventRecord &rec) {
    if (!SPIFFS.exists(EVENT_RING_FILE)) {
        File created = SPIFFS.open(EVENT_RING_FILE, FILE_WRITE);
        if (!created) return false;
        created.close();
    }
    File f = SPIFFS.open(EVENT_RING_FILE, "r+");
    if (!f) return false;

    const size_t offset = (size_t)((rec.seq - 1) % EVENT_LOG_CAPACITY) * sizeof(EventRecord);
    // Mientras el anillo se llena, el slot nuevo cae justo al final. Si el archivo quedó
    // más corto (se borró a mano), rellenar con slots vacíos para no escribir en un hueco.
    const size_t size = f.size() - f.size() % sizeof(EventRecord);
    if (size < offset) {
        static const EventRecord blank = {};
        f.seek(size);
        for (size_t pad = (offset - size) / sizeof(EventRecord); pad > 0; pad--) {
            f.write((const uint8_t*)&blank, sizeof(blank));
        }
    }
    f.seek(offset);
    const bool ok = f.write((const uint8_t*)&rec, sizeof(rec)) == sizeof(rec);
    f.close();
    return ok;
}

void EventLog::formatStamp(const EventRecord &rec, char *buf, size_t len) {
    if (rec.epoch == 0) {
        snprintf(buf, len, "boot+%lus", (unsigned long)rec.uptime_s);
        return;
    }
    const DateTime t(rec.epoch);
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d",
             t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second());
}

// Copia completa en la SD: el anillo de SPIFFS se recorta en 1000, esto no.
void EventLog::appendSD(const EventRecord *recs, uint8_t count) {
    if (count == 0 || !logger.ensureLogFolder()) return;

    File f = SD.open(EVENT_SD_FILE, FILE_APPEND);
    if (!f) return;
    if (f.size() == 0) f.println("seq,time,time_trusted,type,origin,cycle,stage,message");

    for (uint8_t i = 0; i < count; i++) {
        const EventRecord &r = recs[i];
        char stamp[24];
        formatStamp(r, stamp, sizeof(stamp));
        f.printf("%lu,%s,%d,%s,%s,%u,%u,\"", (unsigned long)r.seq, stamp,
                 (r.flags & EVF_TIME_TRUSTED) ? 1 : 0, typeName(r.type), sourceName(r.source),
                 (unsigned)r.cycle, (unsigned)r.stage);
        for (const char *c = r.msg; *c; c++) {
            if (*c == '"') f.print('"');   // CSV: comillas dobladas
            f.print(*c);
        }
        f.println('"');
    }
    f.close();
}

void EventLog::writeEscaped(Print &out, const char *s) {
    for (; *s; s++) {
        const unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') { out.print('\\'); out.print((char)c); }
        else if (c < 0x20)         { out.printf("\\u%04x", c); }
        else                       { out.print((char)c); }
    }
}

bool EventLog::writeJSON(Print &out, uint32_t before, uint32_t after, uint16_t limit) {
    if (file_mutex == nullptr || xSemaphoreTake(file_mutex, pdMS_TO_TICKS(500)) != pdTRUE) return false;

    const uint32_t head = next_seq - 1;
    // "now" = hora del equipo, para que la página calcule la duración de un ciclo en curso
    // con el reloj del cuarto y no con el de la PC que mira.
    out.printf("{\"head\":%lu,\"now\":%lu,\"cap\":%u,\"stage\":%u,\"cycle\":%u,\"active\":%s,\"events\":[",
               (unsigned long)head, (unsigned long)logger.epochAt(millis()),
               (unsigned)EVENT_LOG_CAPACITY, (unsigned)current_stage,
               (unsigned)current_cycle, cycle_active ? "true" : "false");

    uint32_t from = head;
    if (before != 0 && before - 1 < from) from = before - 1;
    uint32_t oldest = head > EVENT_LOG_CAPACITY ? head - EVENT_LOG_CAPACITY + 1 : 1;
    if (after + 1 > oldest) oldest = after + 1;

    File f = SPIFFS.open(EVENT_RING_FILE, FILE_READ);
    bool first = true;
    uint16_t sent = 0;
    if (f) {
        EventRecord rec;
        for (uint32_t seq = from; seq >= oldest && seq > 0 && sent < limit; seq--) {
            f.seek((size_t)((seq - 1) % EVENT_LOG_CAPACITY) * sizeof(EventRecord));
            if (f.read((uint8_t*)&rec, sizeof(rec)) != sizeof(rec) || rec.seq != seq) continue;
            out.printf("%s{\"s\":%lu,\"e\":%lu,\"u\":%lu,\"k\":%u,\"o\":%u,\"g\":%u,\"f\":%u,\"c\":%u,\"m\":\"",
                       first ? "" : ",", (unsigned long)rec.seq, (unsigned long)rec.epoch,
                       (unsigned long)rec.uptime_s, rec.type, rec.source, rec.stage, rec.flags,
                       (unsigned)rec.cycle);
            writeEscaped(out, rec.msg);
            out.print("\"}");
            first = false;
            sent++;
        }
        f.close();
    }
    out.print("]}");
    xSemaphoreGive(file_mutex);
    return true;
}
