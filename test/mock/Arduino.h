// Mock de Arduino.h para tests NATIVOS (corren en la PC, no en el ESP32).
// Solo cubre lo que ConfigStore / Logger necesitan para compilar y testear.
#pragma once
#ifdef __cplusplus   // inerte en C (Unity se compila como C y se le inyecta por force-include)

#include <cstdint>
#include <cstddef>
#include <string>

// La clave del truco: Arduino String == std::string. Así ArduinoJson lo soporta
// nativamente (ARDUINOJSON_ENABLE_STD_STRING) y todas las operaciones que usa
// ConfigStore (length(), c_str(), +, ==) funcionan sin una clase String propia.
using String = std::string;

// ---- Shims de FreeRTOS (los tests nativos son single-thread: lock = no-op) ----
typedef void* SemaphoreHandle_t;

#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFFUL
#endif
#ifndef pdTRUE
#define pdTRUE 1
#endif
#ifndef pdFALSE
#define pdFALSE 0
#endif

inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() {
    static int dummy = 0;
    return &dummy;  // handle no-nulo: ConfigStore solo comprueba != nullptr
}
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t, uint32_t) { return pdTRUE; }
inline int xSemaphoreGiveRecursive(SemaphoreHandle_t) { return pdTRUE; }

#endif // __cplusplus
