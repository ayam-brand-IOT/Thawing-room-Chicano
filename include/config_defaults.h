#ifndef CONFIG_DEFAULTS_H
#define CONFIG_DEFAULTS_H

// Default configuration parameters (used when SD card is unavailable)

// WiFi Configuration
#define DEFAULT_SSID "CFPP-Iot"
#define DEFAULT_WIFI_PASSWORD ""
#define DEFAULT_HOSTNAME "cfpp1"

// Network Configuration
#define DEFAULT_IP_ADDRESS "192.168.100.1"//"18.142.87.81"
#define DEFAULT_STATIC_IP "192.168.100.41"
#define DEFAULT_GATEWAY "192.168.100.1"
#define DEFAULT_PORT 1883

// MQTT Configuration
#define DEFAULT_MQTT_ID "Chicano_cfpp1"
#define DEFAULT_MQTT_USERNAME "chicano"//"cfpp"
#define DEFAULT_MQTT_PASSWORD "nomameshugo"//"Cfpp2021"

// System Configuration
#define DEFAULT_TIME_ZONE_OFFSET_HRS 8
#define DEFAULT_IR_TS false
#define DEFAULT_LORA_TC true
#define DEFAULT_WEB_SERIAL false

#endif
