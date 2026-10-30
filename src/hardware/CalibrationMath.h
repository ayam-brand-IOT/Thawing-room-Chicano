#ifndef CALIBRATION_MATH_H
#define CALIBRATION_MATH_H

// Calibración de 3 puntos por tramos rectos: cuentas del ADC -> °C.
//
// Se calibra la cadena COMPLETA (PT-100 + transmisor + cable + AIA03 + ADC) contra un
// termómetro de referencia. El AIA03 es no lineal por diseño (optoacopladores), así que
// una sola recta se desvía en los extremos; con tres puntos la curva pasa EXACTO por
// cada uno y entre ellos interpola recto. Fuera del rango calibrado extrapola con el
// tramo de la orilla: nunca se dobla ni se invierte, a diferencia de una parábola.
//
// Sin dependencias del core ESP32: lo usan el firmware y los tests nativos.

#include <stdint.h>
#include <stddef.h>

#define CAL_POINTS        3
#define CAL_RAW_MIN       819     // 4 mA: un lazo 4-20 mA vivo nunca lee menos
#define CAL_RAW_MAX       4095    // 20 mA / tope del ADC
#define CAL_MIN_TEMP_GAP  5.0f    // °C entre puntos: más cerca, la pendiente es puro ruido
#define CAL_MIN_RAW_GAP   40      // cuentas entre puntos, por la misma razón
#define CAL_TEMP_LO       (-50.0f)
#define CAL_TEMP_HI       60.0f
// Pendiente de cada tramo, °C por cuenta. La rampa histórica es 0.0263 y la Raja 0.0571:
// esto deja margen para otros transmisores y atrapa errores de dedo (un signo, un cero).
#define CAL_SLOPE_MIN     0.008f
#define CAL_SLOPE_MAX     0.12f

struct CalPoint {
    uint16_t raw;    // mediana de cuentas del ADC durante la captura
    float    temp;   // lo que marcaba el termómetro de referencia, °C
};

struct ChannelCal {
    bool     valid = false;       // false = usar la rampa de fábrica
    CalPoint pts[CAL_POINTS] = {};   // ordenados por `raw` ascendente
    uint32_t at = 0;              // epoch LOCAL de la calibración (0 = desconocido)
};

// Ordena los puntos por cuentas. Devuelve nullptr si la calibración sirve, o el motivo
// (en inglés, va directo a la interfaz) si no.
const char* calValidate(CalPoint pts[CAL_POINTS]);

// Aplica la calibración. Con `cal.valid == false` usa la recta de fábrica.
float calApply(const ChannelCal &cal, uint16_t raw, float factory_slope, float factory_offset);

#endif // CALIBRATION_MATH_H
