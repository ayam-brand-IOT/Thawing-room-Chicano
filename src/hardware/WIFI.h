#ifndef MY_WIFI_H
#define MY_WIFI_H
#include "EEPROM.h"
#include <Update.h>
#include <SPIFFS.h>
#include "config.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <WiFiMulti.h>
#include "../secrets.h"
#include <WiFiClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "./hardware/Logger.h"
#include "resources/WebFiles.h"

#define SSID_SIZE 32
#define PASSWORD_SIZE 64
#define HOSTNAME_SIZE 32
#define IP_ADDRESS_SIZE 16
#define DEFAULT_SUBNET "255.255.255.0"
#define DEFAULT_DNS1 "8.8.8.8"
#define DEFAULT_DNS2 "1.1.1.1"

// ERROR MESSAGES
#define ERR_WRONG_CREDENTIALS "Wrong credentials"
#define ERR_LOST_CONNECTION "Lost connection"


#ifdef WebSerial
  // No incluir WebSerialLite.h
#else
  #include "WebSerialLite.h"
#endif
#include "ESPAsyncWebServer.h"


class WIFI {
  public:
    void init(const char* ssid, const char* password, const char* hostname, const char* static_ip = NULL);
    void loopOTA();
    String getIP();
    void setUpOTA();
    void reconnect();
    bool isConnected();

    void setStaticIP(const char* ip, const char* gateway);


    // Devuelve true si logró conectarse, false si agotó los intentos de boot.
    bool connectToWiFi();
    bool refreshWiFiStatus();
    bool getConnectionStatus();
    void setUpWebServer(bool brigeSerial = false);

    // Modo Access Point + portal cautivo de reconfiguración.
    void startAccessPoint();
    bool isAPMode();
    void loopAP();   // debe llamarse periódicamente para atender el DNS cautivo
  private:
    enum ErrorType { 
      WRONG_CREDENTIALS, 
      LOST_CONNECTION,
      NUM_ERRORS 
    };

    const String errorMessages[NUM_ERRORS] = {ERR_WRONG_CREDENTIALS, ERR_LOST_CONNECTION};
    
    char ssid[SSID_SIZE];
    char password[PASSWORD_SIZE];
    char hostname[HOSTNAME_SIZE];
    bool ap_mode = false;
    bool web_server_started = false;
    DNSServer dnsServer;
    bool use_static_ip = false;
    IPAddress static_ip;
    IPAddress static_gateway;
    IPAddress static_subnet;
    IPAddress static_primary_dns;
    IPAddress static_secondary_dns;
    bool last_connection_state = false;
    void DEBUG(const char *message);
    void ERROR(ErrorType error);
    bool validateJSON(const String& jsonString);
    String generateHTMLForJson(JsonVariant json, String path = "");
    void updateJsonFromForm(AsyncWebServerRequest *request, JsonVariant json);
    String setLayOutInfo(const char* site, String extra_prop = "", String value = "");

};
#endif
