// ###################### NOTICE ######################
// Mapa de pines del board CHICA-LORA REV 0.0.4 (Heltec WiFi LoRa 32 V3 base).
// Vendorizado en el proyecto: antes venía del lib externo Wireless-Stick-Lite-V2.
// Es header-only (solo #defines + arrays const), no depende de ninguna lib de Heltec.
// ####################################################

#include <Arduino.h>

#ifndef WS_V2_h
#define WS_V2_h


// ###################### PORT A ######################
// Reserved for analog inputs

#define PORT_A0 15
#define PORT_A1 6
#define PORT_A2 7
#define PORT_A3 16

// ###################### PORT B ######################
// Reserved for digital inputs

#define PORT_B0 46
#define PORT_B1 1
#define PORT_B2 26
#define PORT_B3 39

// ###################### PORT C ######################
// Reserved for digital outputs

#define PORT_C0 33
#define PORT_C1 12
#define PORT_C2 34
#define PORT_C3 2

// ###################### PORT D ######################
// FREE for use as digital inputs or outputs

#define PORT_D0 3
#define PORT_D1 4
#define PORT_D2 14
#define PORT_D3 5

// ###################### Analog OUTPUTS #######################
// Reserved for digital outputs

#define AI_0         PORT_A0 //
#define AI_1         PORT_A1 //
#define AI_2         PORT_A2 //
#define AI_3         PORT_A3 //

const uint8_t analog_inputs[] = {/* AI_0, */ AI_1 /*, AI_2, AI_3 */};
//PORT_A //NEVR USED

const uint8_t analog_inputs_size =
sizeof(analog_inputs)/sizeof(analog_inputs[0]);

// ######################## RS-485 PINS ########################

#define RS_485_TX   20
#define RS_485_RX   19

// ########################## I2C PINS #########################

#define I2C_SCL     41
#define I2C_SDA     42

// ########################## IO's #########################

const uint8_t inputs[] = {PORT_B0, PORT_B1, PORT_B2, PORT_B3}; //PORT_B
const uint8_t outputs[] = {PORT_D0, PORT_D1, PORT_D2, PORT_D3}; //PORT_D

const uint8_t inputs_size = sizeof(inputs)/sizeof(inputs[0]);
const uint8_t outputs_size = sizeof(outputs)/sizeof(outputs[0]);


// #################### PINS PULLED UP ON START ####################
// This pins need to be set as LOW on start.

const uint8_t pulled_up[] = {33,12}; //PORT_B

const uint8_t pulled_up_size = sizeof(pulled_up)/sizeof(pulled_up[0]);

#endif
