// #include "WiFiType.h"
#include "WIFI.h"
#include "ConfigStore.h"

AsyncWebServer server(80);

// Bandera a nivel de archivo para que los handlers (lambdas sin captura) sepan
// si estamos en modo Access Point y deban redirigir al portal cautivo.
static bool s_ap_mode = false;

static void handle_update_progress_cb(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index){
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    // Update.runAsync(true);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }

  if (final) {
    if (!Update.end(true)){
      Update.printError(Serial);
    }
  }
}

  String WIFI::setLayOutInfo(const char* site, String extra_prop, String value){ 
    String html = site;

    if (extra_prop != "" && value != "") html.replace(extra_prop, value);
    

    html.replace("{{LOCATION}}", String(hostname));
    html.replace("{{VERSION}}", String(VERSION));

    return html;
  };


/* Message callback of WebSerial */
static void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
}

String WIFI::generateHTMLForJson(JsonVariant json, String path) {
    String html = "";
    if (json.is<JsonObject>()) {
        for (auto kvp : json.as<JsonObject>()) {
            String new_path = path + (path == "" ? "" : "|") + String(kvp.key().c_str());
            html += generateHTMLForJson(kvp.value(), new_path);
        }
    } else if (json.is<JsonArray>()) {
        int index = 0;
        for (JsonVariant v : json.as<JsonArray>()) {
            String new_path = path + String("[") + index + String("]");
            html += generateHTMLForJson(v, new_path);
            index++;
        }
    } else {
        // Generate HTML form elements based on the type of the JSON value
        html += "<label for='" + path + "'>" + path + ": </label>";
        // if (json.is<bool>()) {
        //     html += "<input type='text' name='" + path + "' value='" + String(json.as<bool>()) + "'" + (json.as<bool>() ? " checked" : "") + "><br>";
        // } else {
            html += "<input type='text' name='" + path + "' value='" + String(json.as<String>()) + "'><br>";
        // }
    }
    return html;
}

bool isBoolValue(String value){
  if (value == "true" || value == "false") return true;
  return false;
}

bool valToBool(String value){
  return value == "true";
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // Acumulamos el archivo en memoria y lo persistimos atómicamente en SPIFFS al
  // terminar. Así una subida cortada a la mitad nunca corrompe el config bueno.
  static String upload_buffer;

  if (!index) {
    Serial.printf("Subiendo archivo: %s\n", filename.c_str());
    upload_buffer = "";
    upload_buffer.reserve(2048);
  }

  for (size_t i = 0; i < len; i++) upload_buffer += (char)data[i];

  if (final) {
    if (ConfigStore::write(CONFIG_FILE, upload_buffer)) {
      Serial.printf("Archivo subido con éxito: %s\n", filename.c_str());
    } else {
      Serial.println("Error: config subido no es JSON válido, descartado");
    }
    upload_buffer = "";
  }
}

bool WIFI::validateJSON(const String& jsonString) {
  const size_t capacity = JSON_OBJECT_SIZE(2) + 30;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, jsonString);
  return !error; // Retorna true si no hay error, false si hay error
}


void WIFI::updateJsonFromForm(AsyncWebServerRequest *request, JsonVariant json) {
  int params = request->params();
  for (int i = 0; i < params; i++) {
    const AsyncWebParameter* p = request->getParam(i);
    String keyPath = p->name();
    std::vector<String> tokens;
    int last = 0, next = 0;
    while ((next = keyPath.indexOf("|", last)) != -1) {
      tokens.push_back(keyPath.substring(last, next));
      last = next + 1;
    }
    tokens.push_back(keyPath.substring(last));

    JsonVariant cur = json;
    for (size_t j = 0; j < tokens.size(); j++) {
      if (j == tokens.size() - 1) {
        // Handle the array index if it exists
        String val = p->value();
        const bool valIsBool = isBoolValue(val);
        
        if (tokens[j][0] == '[') {
          int index = tokens[j].substring(1, tokens[j].length() - 1).toInt();
          cur[index] = valIsBool ? valToBool(val) : val;
        } 
        else {
          if (valIsBool){
          bool bool_value = valToBool(val);
          cur[tokens[j]] = bool_value;
          } else {
            cur[tokens[j]] = val;
          }
            
        }
      } 
      else {
        // Navigate through the JSON
        if (tokens[j][0] == '[') {
          int index = tokens[j].substring(1, tokens[j].length() - 1).toInt();
          cur = cur[index];
        } else {
          cur = cur[tokens[j]];
        }
      }
    }
  }
}


void WIFI::init(const char* ssid, const char* password, const char* hostname, const char* static_ip) {
  strncpy(this->ssid, ssid, sizeof(this->ssid) - 1);
  this->ssid[sizeof(this->ssid) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  strncpy(this->password, password, sizeof(this->password) - 1);
  this->password[sizeof(this->password) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  strncpy(this->hostname, hostname, sizeof(this->hostname) - 1);
  this->hostname[sizeof(this->hostname) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  // strncpy(this->static_ip, static_ip, sizeof(this->static_ip) - 1);
  // this->static_ip[sizeof(this->static_ip) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'
}

void WIFI::setStaticIP(const char* ip, const char* gateway){
  if (static_ip.fromString(ip) && static_gateway.fromString(gateway)) {
    static_subnet.fromString(DEFAULT_SUBNET);
    static_primary_dns.fromString(DEFAULT_DNS1);
    static_secondary_dns.fromString(DEFAULT_DNS2);
    use_static_ip = true;
  } else {
    use_static_ip = false;
  }
}

void WIFI::setUpWebServer(bool brigeSerial){
  // El servidor web se registra una sola vez; en caída a modo AP reusamos las rutas.
  if (web_server_started) return;

  /*use mdns for host name resolution — reintento ACOTADO, no bloquea el arranque*/
  bool mdns_ok = false;
  for (uint8_t i = 0; i < 5 && !mdns_ok; i++) {
    mdns_ok = MDNS.begin(hostname);
    if (!mdns_ok) {
      DEBUG("Error setting up MDNS responder!");
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
  }
  if (mdns_ok) DEBUG("mDNS responder started");
  else         DEBUG("mDNS unavailable - continuing without it");

  // Función para autenticación básica en todas las rutas
  auto checkAuth = [&](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) {
      request->requestAuthentication();
      return false;
    }
    return true;
  };


  // ======================== Static Files ========================

  server.on("/style.css", HTTP_GET, [&checkAuth](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    request->send(200, "text/css", STYLE_CSS);
  });

  server.on("/config.txt", HTTP_GET, [&checkAuth](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    const String content = ConfigStore::read(CONFIG_FILE);
    if (content.length()) {
        request->send(200, "application/json", content);
    } else {
        request->send(404, "text/plain", "Configuration file not found");
    }
  });

  server.on("/defaultParameters.txt", HTTP_GET, [&checkAuth](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    const String content = ConfigStore::read(DEFAULT_PARAMS_FILE);
    if (content.length()) {
        request->send(200, "application/json", content);
    } else {
        request->send(404, "text/plain", "Default Parameters file not found");
    }
  });

// ======================== Routes ========================

    /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    const String doc = setLayOutInfo(SERVER_INDEX_HTML);
    request->send(200, "text/html", doc);
  });

  server.on("/edit-config", HTTP_GET, [&](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    return request->send(200, "text/html", setLayOutInfo(EDIT_CONFIG_HTML));
  });

  server.on("/edit-settings", HTTP_GET, [&](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    request->send(200, "text/html", setLayOutInfo(EDIT_SETTINGS_HTML));
  });

  server.on("/logs", HTTP_GET, [&](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    if (!logger.hasSD()) {
      request->send(503, "text/plain", "No SD card present - logs unavailable");
      return;
    }
    String log_list = "";
    File root = SD.open(LOG_FOLDER_PATH);
    if (!root) {
      request->send(500, "text/plain", "Failed to open logs directory");
      return;
    }
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      log_list += "\""+fileName + "\", ";
      file = root.openNextFile();
    }
    log_list = log_list.substring(0, log_list.length() - 2);  // Remove trailing comma and space
    

    request->send(200, "text/html", setLayOutInfo(LOGS_HTML, "//{{LOGS}}", log_list));
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
      // En modo AP, cualquier URL desconocida redirige al portal (captive portal):
      // así el navegador/teléfono abre solo la página de configuración.
      if (s_ap_mode) {
        request->redirect("/edit-config");
        return;
      }
      request->send(404, "text/html", "Not found: <u>'"+ request->url() + "'</u>");
  });

  // ======================== SERVER PROCESES ========================

  server.on("/replace-config", HTTP_POST, [&checkAuth](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    request->send(200, "text/plain", "Configuration updated successfully");

  }, handleFileUpload);
  
  /*handling uploading firmware file */
  server.on("/reset", HTTP_POST, [&checkAuth](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    request->send(200, "text/plain", "Resetting...");
    ESP.restart(); 
  });

  server.on("/update", HTTP_POST, [&checkAuth]( AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); 
  }, handle_update_progress_cb);

    // Manejar la descarga de archivos
  server.on("/download_log", HTTP_GET, [&checkAuth](AsyncWebServerRequest *request){
    if(!checkAuth(request)) return;
    if (!logger.hasSD()) {
      request->send(503, "text/plain", "No SD card present - logs unavailable");
      return;
    }
    if (request->hasParam("file")) {
      String fileName = request->getParam("file")->value();
      String full_path = LOG_FOLDER_PATH + String("/") + fileName;
      logger.println(("Downloading file: " + full_path).c_str());
      File file = SD.open(full_path);
      if (file) {
        request->send(file, file.name(), "application/octet-stream", false);
        // Do NOT call file.close() here — ESPAsyncWebServer closes it automatically
        // after the async transfer completes.
      } else {
        request->send(404, "text/plain", "File not found");
      }
    } else {
      request->send(400, "text/plain", "File parameter missing");
    }
  });

  // server.on("/update-config", HTTP_POST, [&](AsyncWebServerRequest *request) {
  //   if(!checkAuth(request)) return;

  //   String body = "";
  //   if (request->hasParam("body", true)) {
  //     body = request->getParam("body", true)->value();
  //   }

  //   File file = SD.open(CONFIG_FILE, "r");
  //   if (!file) {
  //     request->send(500, "text/plain", "Failed to open config file for writing");
  //     return;
  //   }
  //   DynamicJsonDocument doc(4096);  // Ajusta el tamaño según tu archivo JSON

  //   DeserializationError error = deserializeJson(doc, file);
  //   if (error) {
  //     file.close();
  //     request->send(500, "text/plain", "Failed to parse config file");
  //     return;
  //   }

  //   file.close();  // Close the file to reset the file pointer

  //   updateJsonFromForm(request, doc);

  //   // printing doc
  //   Serial.println("Printing doc");
  //   serializeJson(doc, Serial);
  //   Serial.println();

  //   // Re-open the file for writing
  //   file = SD.open(CONFIG_FILE, "w");
    
  //   if (serializeJson(doc, file) == 0) {
  //     file.close();
  //     request->send(500, "text/plain", "Failed to write to file");
  //   } else {
  //     file.close();
  //     request->send(200, "text/plain", "Configuration updated successfully");
  //   }
  // });

  server.on("/update-config", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;

    bool ssidExists = false;

    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      String keyPath = p->name();
      // logger.println(keyPath + ": " + p->value());
      ssidExists = ssidExists || keyPath == "SSID";
    }

    // Seleccionar el archivo a actualizar dependiendo de si existe el "SSID"
    const char* fileToUpdate = ssidExists ? CONFIG_FILE : DEFAULT_PARAMS_FILE;

    const String current = ConfigStore::read(fileToUpdate);
    if (current.length() == 0) {
      request->send(500, "text/plain", "Failed to open config file for writing");
      return;
    }

    DynamicJsonDocument doc(4096);  // Ajusta el tamaño según tu archivo JSON
    DeserializationError error = deserializeJson(doc, current);
    if (error) {
      request->send(500, "text/plain", "Failed to parse config file");
      return;
    }

    // Actualizar el JSON con los nuevos valores del cuerpo
    updateJsonFromForm(request, doc);

    // Imprimir el documento actualizado en el Serial
    Serial.println("Printing updated doc:");
    serializeJson(doc, Serial);
    Serial.println();

    // Persistir atómicamente en SPIFFS
    String out;
    serializeJson(doc, out);
    if (ConfigStore::write(fileToUpdate, out)) {
      request->send(200, "text/plain", "Configuration updated successfully");
    } else {
      request->send(500, "text/plain", "Failed to write to file");
    }
  });


  server.on("/toggle-output", HTTP_GET, [&](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    if (logger.currentOutput == Logger::HW_SERIAL) {
      logger.setOutput(Logger::WEBSERIAL);
    } else {
      logger.setOutput(Logger::HW_SERIAL);
    }
    request->send(200, "text/plain", "Output toggled");
  });

  
  if (brigeSerial) {
    WebSerial.begin(&server);
    WebSerial.onMessage(recvMsg);
  }
  server.begin();
  web_server_started = true;
}

String WIFI::getIP(){
  String ip =  MDNS.queryHost("beer-control").toString();
  Serial.println(ip);
  return ip;
}

bool WIFI::connectToWiFi(){
  if (use_static_ip) {
    if(!WiFi.config(static_ip, static_gateway, static_subnet, static_primary_dns, static_secondary_dns)) {
      DEBUG("Failed to configure static IP");
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  uint32_t notConnectedCounter = 0;
  EEPROM.begin(32);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    DEBUG("Wifi connecting...");

    notConnectedCounter++;
    if(notConnectedCounter > 7) { // ~16s sin conectar en este arranque
      const uint8_t num_of_tries = EEPROM.readInt(1);
      // Tras AP_MAX_BOOT_WIFI_TRIES reinicios fallidos, rendirse y caer a modo AP.
      if (num_of_tries >= AP_MAX_BOOT_WIFI_TRIES) {
        DEBUG("WiFi failed after max boot tries - falling back to AP mode");
        EEPROM.writeInt(1, 0);   // resetear el contador para el próximo arranque manual
        EEPROM.commit();
        EEPROM.end();
        return false;
      }
      DEBUG("Resetting due to Wifi not connecting...");
      EEPROM.writeInt(1, num_of_tries + 1);
      EEPROM.commit();
      EEPROM.end();
      ESP.restart();
    }
  }

  EEPROM.writeInt(1, 0);
  EEPROM.commit();
  EEPROM.end();

  DEBUG(("IP address: " + WiFi.localIP().toString()).c_str());
  return true;
}

void WIFI::startAccessPoint(){
  ap_mode = true;
  s_ap_mode = true;

  // Levantar el AP. El control de la sala sigue corriendo en loop() en paralelo.
  WiFi.mode(WIFI_AP);
  String ap_ssid = String(AP_SSID_PREFIX) + String(hostname);

  bool ok;
  if (strlen(AP_PASSWORD) >= 8) ok = WiFi.softAP(ap_ssid.c_str(), AP_PASSWORD);  // WPA2
  else                          ok = WiFi.softAP(ap_ssid.c_str());                // abierto

  if (!ok) {
    DEBUG("softAP failed to start");
    return;
  }

  const IPAddress ap_ip = WiFi.softAPIP();

  // DNS cautivo: responde toda consulta con nuestra IP -> el portal abre solo.
  dnsServer.start(DNS_PORT, "*", ap_ip);

  // Reusar las rutas web ya registradas para editar la config.
  setUpWebServer(false);

  char buf[80];
  snprintf(buf, sizeof(buf), "AP up: '%s' @ %s (pwd: %s)",
           ap_ssid.c_str(), ap_ip.toString().c_str(),
           strlen(AP_PASSWORD) >= 8 ? AP_PASSWORD : "<open>");
  DEBUG(buf);
}

bool WIFI::isAPMode(){
  return ap_mode;
}

void WIFI::loopAP(){
  if (ap_mode) dnsServer.processNextRequest();
}

void WIFI::setUpOTA(){
  if(isConnected()){
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.onStart([]() {
    String type;
    type = ArduinoOTA.getCommand() == U_FLASH ? "sketch" : "filesystem";
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    logger.println("Start updating " + type);
    }).onEnd([]() {
      logger.println("\nEnd");
    }).onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    }).onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) logger.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) logger.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) logger.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) logger.println("Receive Failed");
      else if (error == OTA_END_ERROR) logger.println("End Failed");
    });
    ArduinoOTA.begin();
  }
}

void WIFI::loopOTA(){
  ArduinoOTA.handle();
}

bool WIFI::refreshWiFiStatus(){
  const bool connection = isConnected();
  if (connection != last_connection_state){
    last_connection_state = connection;
    return true;
  }
  return false;
}

bool WIFI::getConnectionStatus(){
  return last_connection_state;
}

bool WIFI::isConnected(){
  return WiFi.status() == WL_CONNECTED;
}

void WIFI::reconnect(){
  WiFi.begin(ssid, password);
  uint8_t timeout = 0;
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  while ( WiFi.status() != WL_CONNECTED ){
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    log_i(" waiting on wifi connection" );
    timeout++;
    if (timeout == 2) return;
  }
}

void WIFI::DEBUG(const char *message){
  // concat prefix to the message with the classname
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[WIFI]: %s", message);
  logger.println(buffer);
}\

void WIFI::ERROR(ErrorType error){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), " -> WIFI]: %s", errorMessages[error].c_str());
  logger.printError(buffer);
}



