#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

// Control del proceso desde la página web (/control).
//
// Los handlers web corren en la task de AsyncTCP, no en el loop. Nada de acá toca el
// proceso directamente:
//   - Comandos: el handler valida y deja UNO pendiente; loop() lo toma y lo ejecuta por
//     el mismo camino que el SCADA (handleInputs), con origen WEB en la bitácora.
//   - Estado: loop() publica una foto cada ~500 ms; los handlers solo copian esa foto.
// Así no hay lecturas del RTC, del ADC ni de MQTT fuera del core 1.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>

enum ControlAction : uint8_t {
    CTRL_NONE   = 0,
    CTRL_START  = 1,   // directo a Etapa 2
    CTRL_DSTART = 2,   // Etapa 1 con hora programada para la Etapa 2
    CTRL_STOP   = 3,
    CTRL_LOCK_ON  = 4,   // bloqueo por mantenimiento
    CTRL_LOCK_OFF = 5
};

struct ControlCommand {
    uint32_t      id;
    ControlAction action;
    uint8_t       hour, minute, day, month;   // solo CTRL_DSTART
};

// Foto del proceso. La arma loop(); los handlers la serializan.
struct ControlStatus {
    uint8_t  stage;
    uint8_t  step;               // paso del ciclo de ventilador (1..4), 0 = sin iniciar
    bool     starting;           // guarda de 5 s al entrar a Etapa 2/3
    uint16_t cycle;
    bool     cycle_active;
    uint32_t stage_since;        // epoch LOCAL de entrada a la etapa (0 = desconocido)
    bool     stage_since_boot;   // la etapa venía de antes del reinicio: el "desde" es el arranque

    float    ta, ts, tc;
    float    avg_ta, avg_ts, avg_tc;
    bool     ta_fault, ts_fault, tc_fault;
    uint16_t ta_counts, ts_counts, tc_counts;   // mediana del ADC; 0xFFFF = no viene del ADC
    float    ts_set, tc_set;     // condición de paso a Etapa 3
    float    pid_setpoint;       // consigna de Ta que calcula el PID (B - A*avgTs)

    bool     fan_on, fan_reverse;
    bool     damper, sprinkler;
    uint8_t  air_duty, air_max;  // último duty escrito al PWM y su tope (2^RESOLUTION - 1)

    uint8_t  s2_hour, s2_minute, s2_day, s2_month;   // programación de la Etapa 2
    bool     s2_set;
    int32_t  s2_remaining_min;

    bool     lock;               // bloqueo por mantenimiento activo
    uint32_t lock_since;         // epoch LOCAL de activación (0 = desconocido)
    uint8_t  lock_source;        // EventSource de quien lo activó

    bool     wifi, mqtt, sd;
    bool     time_trusted;
    uint8_t  time_source;        // TimeSource
};

class RemoteControl {
public:
    // --- loop() (core 1) ---
    void publish(const ControlStatus &s);
    bool take(ControlCommand &cmd);
    void finish(uint32_t id, bool ok, const char *msg);

    // --- handlers web ---
    // Devuelve el id del comando encolado, o 0 con el motivo en `err`.
    uint32_t request(ControlCommand cmd, char *err, size_t err_len);
    void writeJSON(Print &out);

private:
    portMUX_TYPE  mux = portMUX_INITIALIZER_UNLOCKED;
    ControlStatus status = {};
    bool          has_status = false;

    ControlCommand pending = {};
    bool           has_pending = false;
    uint32_t       next_id = 1;

    uint32_t done_id = 0;
    bool     done_ok = false;
    char     done_msg[96] = "";
};

extern RemoteControl remote;

#endif // REMOTE_CONTROL_H
