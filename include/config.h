#ifndef CONFIG_H
#define CONFIG_H
#include <WS_V2.h>
#include <Arduino.h>
#include <DallasTemperature.h>

// temperature acquisition filter 
#define HIGH_TEMP_LIMIT 60
#define LOW_TEMP_LIMIT -40

// setting PWM properties
#define AIR_PWM     7   
#define FREQ        5000 
#define AIR_PIN     PORT_A3  
#define RESOLUTION  6    

//------------ IO's    -------------------------------------------------------------------->
#define STAGE_1_IO  PORT_C0   
#define STAGE_2_IO  PORT_C1
#define STAGE_3_IO  PORT_C2
#define FLUSH_IO    PORT_C3


#define VALVE_IO        PORT_D3 // changed to have more logic on the board
#define AIR_DAMPER_IO   PORT_D2
#define FAN_CCW_IO      PORT_D1 
#define FAN_CW_IO       PORT_D0  
// #define STOP_IO     DI_0    
// #define DLY_S_IO    DI_1    
// #define START_IO    DI_2    

#define START_IO    PORT_B0  
#define STOP_IO     PORT_B2
#define DLY_S_IO    PORT_B1   

// #define A0    13 //ONE_WIRE_BUS  

#define TA_AI       AI_0
#define TS_AI       AI_1
#define TC_AI       AI_2

#define BUFFER_SIZE 60 

// Temperature Sensors settings          -------------------------------------------------------------------->
#define ONE_WIRE_BUS 13// Data wire is plugged into port 2 on the Arduin
#define TEMPERATURE_PRECISION 12

#define TIME_ACQ_DELAY 1000 //in ms the delay between temperature value refresh
#define AVG_RESOLUTION 1000   //in ms the sampling for the Ts measure

// MIN OFFTIMES
#define MIN_OFFTIME_STAGE1 0.5
#define MIN_OFFTIME_STAGE2 0.5
#define MIN_OFFTIME_STAGE3 0.5

// #define TIME_ZONE_OFFSET_HRS            (-7)  /* Ensenada, México */
#define TIME_ZONE_OFFSET_HRS            (+8)   /* Taiping, Malaysia */

// DeviceAddress ADDRESS_TA = { 0x28, 0x8C, 0x4B, 0xAD, 0x27, 0x19, 0x01, 0xCA }; // Ta
// // DeviceAddress ADDRESS_TC1 = { 0x28, 0xA7, 0x93, 0x8B, 0x0B, 0x00, 0x00, 0xB2 }; // Ta
// DeviceAddress ADDRESS_TS = { 0x28, 0x78, 0x98, 0x8B, 0x0B, 0x00, 0x00, 0x22 }; // Ts
// DeviceAddress ADDRESS_TC = { 0x28, 0xDA, 0xB6, 0xF7, 0x3A, 0x19, 0x01, 0x85 }; // Tc 
// DeviceAddress ADDRESS_TI = { 0x28, 0x5A, 0xD3, 0x2A, 0x0D, 0x00, 0x00, 0x94 }; // Ti

#define IR_SENSOR_ADDRESS 0x5A
#define READ_TEMPERATURE 0x07

#define CONFIG_FILE "/config.txt"
#define DEFAULT_PARAMS_FILE "/defaultParameters.txt"

// ------------ Access Point / Config Portal -------------------------------->
// Si no hay config.txt, o si el WiFi falla 3 veces al arrancar, el equipo
// levanta su propio Access Point para poder reconfigurarlo desde un navegador.
#define AP_SSID_PREFIX  "ThawingRoom-"   // se le concatena el hostname
#define AP_PASSWORD     "thawing1234"     // WPA2 (>= 8 chars). "" = AP abierto
#define AP_MAX_BOOT_WIFI_TRIES 3          // intentos de WiFi en boot antes de caer a AP
#define DNS_PORT 53

// ------------ Embedded fallback parameters -------------------------------->
// Último recurso si no hay defaultParameters.txt ni en SPIFFS ni en SD ni en .bak.
// Mantener en sintonía con data/defaultParameters.txt.
#define EMBEDDED_DEFAULT_PARAMS "{\"stage1\":{\"f1Ontime\":\"1\",\"f1Offtime\":\"1\",\"s1Ontime\":\"0.5\",\"s1Offtime\":\"1\"},\"stage2\":{\"f1Ontime\":\"1\",\"f1Offtime\":\"1\",\"s1Ontime\":\"1\",\"s1Offtime\":\"1\"},\"stage3\":{\"f1Ontime\":\"10\",\"f1Offtime\":\"30\",\"s1Ontime\":\"1\",\"s1Offtime\":\"15\"},\"setPoint\":{\"A\":\"0.5\",\"B\":\"20\"},\"tset\":{\"tsSet\":\"21\",\"tcSet\":\"21\"}}"


#endif
