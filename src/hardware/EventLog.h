#ifndef EVENT_LOG_H
#define EVENT_LOG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// ============================================================================
// Bitácora de eventos del proceso: quién hizo qué y a qué hora.
//
// No es el Logger. El Logger es diagnóstico (mucho volumen, a la SD si hay, se
// rota y se pierde). Esto es la historia operativa del cuarto: arranques, cambios
// de etapa, paros, parámetros modificados, alarmas. Poco volumen y tiene que
// sobrevivir aunque no haya tarjeta, así que vive en SPIFFS como un anillo de
// EVENT_LOG_CAPACITY registros de tamaño fijo. Si hay SD, además se agrega cada
// evento a /logs/events.csv, que no se recorta.
//
// Concurrencia: mismo esquema que el Logger. emit() se llama desde cualquier task
// (loop de control, handlers web, backgroundTasks), formatea y encola SIN bloquear.
// drain() la llama SOLO backgroundTasks (core 0): asigna el número de secuencia,
// fecha con la instantánea de reloj del Logger y escribe. El archivo del anillo
// lo leen además los handlers web, por eso lleva un mutex propio.
// ============================================================================

enum EventType : uint8_t {
    EV_PROCESS = 0,   // arranque, cambio de etapa, paro, fin, reanudación
    EV_PARAM,         // set-points, tiempos, PID
    EV_ALERT,         // algo anda mal pero el proceso sigue
    EV_ERROR,         // falla seria
    EV_SYSTEM,        // arranque del equipo, firmware, configuración
    EV_TYPE_COUNT
};

enum EventSource : uint8_t {
    SRC_MANUAL = 0,   // botón físico del tablero
    SRC_REMOTE,       // SCADA (Ecava) por MQTT. Todavía sin identidad del operador.
    SRC_WEB,          // interfaz web del equipo
    SRC_AUTO,         // decisión del firmware (hora, temperatura)
    SRC_SYSTEM,       // arranque, recuperación tras reinicio
    SRC_COUNT
};

#define EVENT_LOG_CAPACITY  1000
#define EVENT_MSG_MAX       108
#define EVENT_QUEUE_LEN     24
#define EVENT_RING_FILE     "/events.bin"
#define EVENT_SD_FILE       "/logs/events.csv"

// Bits de EventRecord::flags
#define EVF_TIME_TRUSTED  0x01   // la hora salió de RTC/NTP y no del respaldo
#define EVF_CYCLE_START   0x02
#define EVF_CYCLE_END     0x04
#define EVF_FINISHED      0x08   // fin normal (STOP en etapa 3) en vez de paro
#define EVF_CYCLE_ACTIVE  0x10   // tras este evento hay un ciclo en curso

// Registro en flash: 128 bytes exactos, para que el slot N esté en N*128.
struct EventRecord {
    uint32_t seq;        // 1.. creciente; 0 = slot vacío
    uint32_t epoch;      // hora LOCAL en segundos (ver memoria del RTC); 0 = sin hora
    uint32_t uptime_s;
    uint8_t  type;
    uint8_t  source;
    uint8_t  stage;      // etapa vigente después del evento
    uint8_t  flags;
    uint16_t cycle;      // 0 = fuera de un ciclo
    uint16_t reserved;
    char     msg[EVENT_MSG_MAX];
};
static_assert(sizeof(EventRecord) == 128, "EventRecord debe medir 128 bytes");

class EventLog {
public:
    // Crea cola y mutex y recorre el anillo para retomar la secuencia y el ciclo.
    // Llamar en setup() con SPIFFS ya montado (controller.init()).
    bool begin();

    void emit(EventType type, EventSource source, const char* fmt, ...);
    // Abre un ciclo nuevo y registra el evento como su inicio.
    void cycleStart(EventSource source, const char* fmt, ...);
    // Cierra el ciclo en curso. finished=true para fin normal, false para paro.
    void cycleEnd(EventSource source, bool finished, const char* fmt, ...);
    // Tras un reinicio con proceso en curso: sigue en el mismo ciclo.
    void cycleResume(EventSource source, const char* fmt, ...);
    // Tras un reinicio sin proceso: cierra en silencio un ciclo que quedó colgado.
    void cycleClear() { cycle_active = false; }

    void setStage(uint8_t stage) { current_stage = stage; }
    uint16_t cycle() const      { return current_cycle; }
    bool     cycleActive() const { return cycle_active; }

    // SOLO backgroundTasks (core 0).
    void drain();

    // Escribe a `out` un JSON con los eventos más nuevos primero. before/after son
    // exclusivos (0 = sin límite). Lo usan los handlers web.
    bool writeJSON(Print &out, uint32_t before, uint32_t after, uint16_t limit);

    static const char* typeName(uint8_t type);
    static const char* sourceName(uint8_t source);

private:
    struct Pending {
        uint32_t ms;
        uint8_t  type, source, stage, flags;
        uint16_t cycle;
        char     msg[EVENT_MSG_MAX];
    };

    QueueHandle_t     queue = nullptr;
    SemaphoreHandle_t file_mutex = nullptr;
    uint32_t next_seq = 1;
    uint32_t dropped = 0;

    // Los escribe solo el loop de control (core 1): los cambios de ciclo nacen ahí.
    volatile uint16_t current_cycle = 0;
    volatile bool     cycle_active  = false;
    volatile uint8_t  current_stage = 0;

    void enqueue(EventType type, EventSource source, uint8_t flags, const char* fmt, va_list args);
    void scanRing();
    bool writeRing(const EventRecord &rec);
    void appendSD(const EventRecord *recs, uint8_t count);
    static void formatStamp(const EventRecord &rec, char *buf, size_t len);
    static void writeEscaped(Print &out, const char *s);
};

extern EventLog events;

#endif // EVENT_LOG_H
