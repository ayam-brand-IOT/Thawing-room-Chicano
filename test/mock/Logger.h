// Mock mínimo de Logger.h para tests nativos. ConfigStore solo usa logger.println().
// Evitamos arrastrar el Logger real (Serial, SPI, SD, RTClib...).
//
// Este header se inyecta con force-include (-include) ANTES que cualquier código.
// Definimos el include-guard del Logger.h real (LOGGER_H) para que, cuando
// ConfigStore.cpp haga #include "Logger.h" (que resuelve al real por estar en el
// mismo directorio), su cuerpo quede vacío y gane este mock.
#pragma once
#define LOGGER_H
#ifdef __cplusplus   // inerte en C (Unity); el #define LOGGER_H de arriba sí aplica siempre
#include <Arduino.h>
#include <cstdio>

struct MockLogger {
    void println(const char* msg) { (void)msg; /* silencioso en tests; descomenta para depurar */
        // std::printf("[log] %s\n", msg);
    }
    void println(const String& msg) { println(msg.c_str()); }
};

inline MockLogger logger;
#endif // __cplusplus
