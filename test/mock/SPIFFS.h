// Mock de SPIFFS.h: instancia global del FS en RAM (inline var C++17 -> una sola
// definición aunque varias TUs lo incluyan).
#pragma once
#include <FS.h>

inline MockFS SPIFFS;
