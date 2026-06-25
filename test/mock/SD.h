// Mock de SD.h: ConfigStore solo usa SD en migrateFromSD(). Reusamos el mismo
// MockFS; los tests de la lógica atómica no tocan SD, pero debe compilar/enlazar.
#pragma once
#include <FS.h>

inline MockFS SD;
