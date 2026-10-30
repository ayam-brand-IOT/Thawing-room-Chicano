#ifndef CALIBRATION_H
#define CALIBRATION_H

// Calibración de sensores editable desde la web (/calibration). Las cuentas viven en
// CalibrationMath; esto es la parte del equipo: archivo en SPIFFS, muestras del ADC para
// el asistente y el paso de datos entre los handlers web y el loop.
//
// Hilos, igual que /control:
//   - loop() (core 1) es el único que lee el ADC y el único que usa la calibración
//     activa (rawToTemp). Empuja muestras con sample() y aplica cambios con service().
//   - Los handlers web validan, escriben el archivo y dejan la tabla nueva pendiente.
//
// Archivo /calibration.txt:
//   {"ta":{"cur":{"at":<epoch>,"pts":[[raw,°C],[raw,°C],[raw,°C]]},"prev":{...}},"ts":..,"tc":..}
// Sin archivo, o con un canal sin "cur", ese canal usa la rampa de fábrica de config.h.

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "CalibrationMath.h"

#define CAL_FILE          "/calibration.txt"
#define CAL_CHANNELS      3
#define CAL_WINDOW        80      // muestras por canal: 80 x 250 ms = 20 s de historia
#define CAL_SAMPLE_MS     250

enum CalChannel : uint8_t { CAL_TA = 0, CAL_TS = 1, CAL_TC = 2 };

enum CalOp : uint8_t { CAL_OP_SAVE, CAL_OP_REVERT, CAL_OP_RESET };

class SensorCalibration {
public:
    // setup(), con SPIFFS montado y EventLog listo.
    void begin();

    // --- loop() (core 1) ---
    const ChannelCal& active(uint8_t ch) const { return act[ch]; }
    void sample(uint8_t ch, uint16_t raw);
    void setContext(bool lockdown, uint8_t stage, const bool available[CAL_CHANNELS]);
    void service();   // aplica la tabla que haya dejado un handler

    // --- handlers web ---
    // pts solo para CAL_OP_SAVE. false con el motivo en `msg`; true con la confirmación.
    bool request(CalOp op, uint8_t ch, CalPoint pts[CAL_POINTS], char *msg, size_t msg_len);
    bool writeJSON(Print &out);

    static float factorySlope(uint8_t ch);
    static float factoryOffset(uint8_t ch);
    static const char* channelKey(uint8_t ch);    // "ta" / "ts" / "tc"
    static const char* channelName(uint8_t ch);   // "Ta" / "Ts" / "Tc"

private:
    // Modelo persistido (lo que hay en el archivo). Lo tocan los handlers, bajo `model_mutex`.
    ChannelCal cur[CAL_CHANNELS];
    ChannelCal prev[CAL_CHANNELS];
    SemaphoreHandle_t model_mutex = nullptr;

    // Calibración en uso. SOLO loop().
    ChannelCal act[CAL_CHANNELS];

    // Handler -> loop
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    ChannelCal pending[CAL_CHANNELS];
    bool       has_pending[CAL_CHANNELS] = {false, false, false};

    // loop -> handler: muestras y contexto
    uint16_t ring[CAL_CHANNELS][CAL_WINDOW] = {};
    uint8_t  ring_count[CAL_CHANNELS] = {0, 0, 0};
    uint8_t  ring_next[CAL_CHANNELS] = {0, 0, 0};
    uint16_t last_raw[CAL_CHANNELS] = {0, 0, 0};
    bool     ctx_lock = false;
    uint8_t  ctx_stage = 0;
    bool     ctx_avail[CAL_CHANNELS] = {true, true, true};
    bool     ctx_ready = false;

    bool save(char *msg, size_t msg_len);   // con model_mutex tomado
    void post(uint8_t ch, const ChannelCal &cal);
};

extern SensorCalibration calib;

#endif // CALIBRATION_H
