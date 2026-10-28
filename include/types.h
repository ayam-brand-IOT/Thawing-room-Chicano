#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// ==================== ENUMS ====================

typedef enum {
  IDLE = 0,
  STAGE1 = 1,
  STAGE2 = 2,
  STAGE3 = 3,
  ERROR = 4,
  NUM_STATES = 5
} SystemState;

typedef enum {
  NONE = 0,
  D_START = 1,
  START = 2,
  STOP = 3
} button_type;

typedef enum {
  TA_TYPE = 0,
  TS_TYPE = 1,
  TC_TYPE = 2
} SensorProbes;

// ==================== FUENTE DE LA HORA ====================
// De dónde salió el último getDateTime(). Sin esto el reloj nunca puede decir "no sé
// qué hora es": cuando no hay ninguna fuente devuelve la fecha de COMPILACIÓN, que pasa
// la validación de rango y se ve perfectamente plausible. Quien programa un arranque
// diferido necesita poder distinguir eso.
typedef enum {
  TIME_SRC_NONE = 0,   // nunca hubo hora válida -> semilla de compilación. NO confiable.
  TIME_SRC_EXTERNAL,   // DS3231 (TCXO, ~2 ppm)
  TIME_SRC_INTERNAL,   // RTC interno del ESP32 (oscilador RC, deriva con temperatura)
  TIME_SRC_MILLIS,     // última hora válida + delta de millis() (cristal de 40 MHz)
  TIME_SRC_UNKNOWN = 255
} TimeSource;

// ==================== ERROR TYPES ====================

typedef enum {
  I2C_ERROR = 0,
  RTC_NOT_FOND,
  IR_NOT_FOUND,
  TC_OUT_OF_RANGE,
  TS_OUT_OF_RANGE,
  TA_OUT_OF_RANGE,
  NO_CONFIG_FILE,
  NO_DEFAULT_PARAMS,
  NUM_ERRORS
} ErrorType;

// ==================== STAGE STATE ====================

typedef struct {
  SystemState stage;
  uint8_t step;
} StageState;

// ==================== STAGE PARAMETERS ====================

typedef struct {
  float fanOnTime;
  float fanOffTime;
  float fanRevONTime;
  float sprinklerOnTime;
  float sprinklerOffTime;
} stage_parameters;

// ==================== ROOM PARAMETERS ====================

typedef struct {
  float A;
  float B;
  uint8_t coef_pid_fwd;
  uint8_t coef_pid_rev;

} room_parameters;

// ==================== TEMPERATURE DATA ====================

typedef struct {
  float ta;
  float ts;
  float tc;
  float ti;
  float avg_ta;
  float avg_ts;
  float avg_tc;
} data_s;

typedef struct {
  float ts;
  float tc;
} data_tset;

// ==================== TIMER STRUCTURE ====================

typedef struct {
  struct {
    uint32_t fan;
    uint32_t sprinkler;
  } stage1;
  
  struct {
    uint32_t fan;
    uint32_t sprinkler;
    uint32_t pid_computing;
    uint32_t pid_turn_on;
    uint32_t pid_turn_off;
    uint32_t pid_publish;
    uint32_t init_delay;   // non-blocking 5s init guard
  } stage2;
  
  struct {
    uint32_t fan;
    uint32_t sprinkler;
    uint32_t init_delay;   // non-blocking 5s init guard
  } stage3;
  
  uint32_t temp_acquisition;
  uint32_t A_B_publish;
} SystemTimers;

#endif // TYPES_H
