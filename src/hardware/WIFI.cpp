// #include "WiFiType.h"
#include "WIFI.h"
#include "ConfigStore.h"

AsyncWebServer server(80);

// Bandera a nivel de archivo para que los handlers (lambdas sin captura) sepan
// si estamos en modo Access Point y deban redirigir al portal cautivo.
static bool s_ap_mode = false;

static volatile uint32_t s_restart_at = 0;   // millis() objetivo; 0 = sin reinicio pedido

void scheduleRestart(uint32_t delay_ms) {
  const uint32_t at = millis() + delay_ms;
  s_restart_at = at ? at : 1;
}

void serviceRestart() {
  if (s_restart_at == 0 || (int32_t)(millis() - s_restart_at) < 0) return;
  events.drain();
  logger.drain();
  ESP.restart();
}

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

// Resultado de la última subida a /replace-config. El handler de la petición corre
// DESPUÉS del último trozo del archivo, así que lo lee de acá para contestar la verdad
// (antes contestaba "updated successfully" aunque el archivo se hubiera rechazado).
static bool   s_upload_done = false;
static bool   s_upload_ok   = false;

// Texto -> string JSON (comillas, barras y controles escapados).
static String jsonString(const String &in) {
  String out = "\"";
  for (size_t i = 0; i < in.length(); i++) {
    const char c = in[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if ((uint8_t)c < 0x20) out += ' ';
    else out += c;
  }
  return out + "\"";
}

static void sendResult(AsyncWebServerRequest *request, int code, bool ok, const String &msg, const String &extra = "") {
  String body = "{\"ok\":";
  body += ok ? "true" : "false";
  body += ",\"msg\":" + jsonString(msg);
  body += extra;
  body += "}";
  request->send(code, "application/json", body);
}

void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // Acumulamos el archivo en memoria y lo persistimos atómicamente en SPIFFS al
  // terminar. Así una subida cortada a la mitad nunca corrompe el config bueno.
  static String upload_buffer;

  if (!index) {
    s_upload_done = false;
    Serial.printf("Subiendo archivo: %s\n", filename.c_str());
    upload_buffer = "";
    upload_buffer.reserve(2048);
  }

  for (size_t i = 0; i < len; i++) upload_buffer += (char)data[i];

  if (final) {
    s_upload_done = true;
    s_upload_ok = ConfigStore::write(CONFIG_FILE, upload_buffer);
    if (s_upload_ok) {
      Serial.printf("Archivo subido con éxito: %s\n", filename.c_str());
      events.emit(EV_SYSTEM, SRC_WEB, "config.txt replaced by upload (%s), applies on restart", filename.c_str());
    } else {
      Serial.println("Error: config subido no es JSON válido, descartado");
      events.emit(EV_ALERT, SRC_WEB, "Uploaded config rejected: not valid JSON (%s)", filename.c_str());
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


void WIFI::updateJsonFromForm(AsyncWebServerRequest *request, JsonVariant json, std::vector<String> *changes) {
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

        if (changes) {
          const bool is_index = tokens[j][0] == '[';
          const String prev = is_index
              ? cur[tokens[j].substring(1, tokens[j].length() - 1).toInt()].as<String>()
              : cur[tokens[j]].as<String>();
          if (prev != val) {
            String key = keyPath;
            key.replace("|", ".");
            // Contraseñas: se registra que cambiaron, nunca el valor.
            String upper = key;
            upper.toUpperCase();
            if (upper.indexOf("PASS") >= 0) changes->push_back(key + " changed");
            else                            changes->push_back(key + ": " + prev + " -> " + val);
          }
        }
        
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
    // Sin SD o con la carpeta ilegible se muestra la página igual, con el motivo, en vez
    // de una hoja en blanco con un texto plano.
    String log_list = "";
    const char *state = "ok";
    if (!logger.hasSD()) {
      state = "nosd";
    } else {
      logger.ensureLogFolder();   // SD nueva: lista vacía en vez de error
      File root = SD.open(LOG_FOLDER_PATH);
      if (!root) {
        state = "error";
      } else {
        File file = root.openNextFile();
        while (file) {
          String fileName = file.name();
          log_list += "\""+fileName + "\", ";
          file = root.openNextFile();
        }
        log_list = log_list.substring(0, log_list.length() - 2);  // Remove trailing comma and space
      }
    }

    String html = setLayOutInfo(LOGS_HTML, "//{{LOGS}}", log_list);
    html.replace("{{SD_STATE}}", state);
    request->send(200, "text/html", html);
  });

  // Bitácora de eventos. Sin `checkAuth` capturado: ese lambda es local de esta función
  // y estos handlers corren cuando ya retornó. Se autentica directo contra los miembros.
  server.on("/events", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    request->send(200, "text/html", setLayOutInfo(EVENTS_HTML));
  });

  // GET /api/events?before=<seq>&after=<seq>&limit=<n>  -> más nuevos primero
  server.on("/api/events", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    auto param = [request](const char* name) -> uint32_t {
      return request->hasParam(name) ? (uint32_t)request->getParam(name)->value().toInt() : 0;
    };
    uint32_t limit = param("limit");
    if (limit == 0 || limit > 250) limit = 250;

    AsyncResponseStream *res = request->beginResponseStream("application/json");
    res->addHeader("Cache-Control", "no-store");
    if (!events.writeJSON(*res, param("before"), param("after"), (uint16_t)limit)) {
      delete res;
      request->send(503, "text/plain", "Event log busy, try again");
      return;
    }
    request->send(res);
  });

  // Control remoto del proceso. Mismo esquema de auth que /events.
  server.on("/control", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    request->send(200, "text/html", setLayOutInfo(CONTROL_HTML));
  });

  server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    AsyncResponseStream *res = request->beginResponseStream("application/json");
    res->addHeader("Cache-Control", "no-store");
    remote.writeJSON(*res);
    request->send(res);
  });

  // POST /api/control  action=start|dstart|stop|lock|unlock  [hour, minute, day, month]
  // Exige el header X-Control: un formulario de otro sitio no puede mandarlo, y un fetch
  // de otro origen con ese header necesita un preflight CORS que acá nunca se aprueba.
  // Sin esto, cualquier página abierta en un navegador ya autenticado podría parar el cuarto.
  server.on("/api/control", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    if (!request->hasHeader("X-Control")) {
      request->send(403, "application/json", "{\"ok\":false,\"msg\":\"Missing X-Control header\"}");
      return;
    }
    auto param = [request](const char* name) -> String {
      return request->hasParam(name, true) ? request->getParam(name, true)->value() : String();
    };
    const String action = param("action");
    ControlCommand cmd = {};
    if      (action == "start")  cmd.action = CTRL_START;
    else if (action == "dstart") cmd.action = CTRL_DSTART;
    else if (action == "stop")   cmd.action = CTRL_STOP;
    else if (action == "lock")   cmd.action = CTRL_LOCK_ON;
    else if (action == "unlock") cmd.action = CTRL_LOCK_OFF;
    if (cmd.action == CTRL_DSTART) {
      // toInt() de un campo vacío da 0; un 0 en día o mes lo rechaza la validación.
      cmd.hour   = (uint8_t)constrain(param("hour").toInt(),   0, 255);
      cmd.minute = (uint8_t)constrain(param("minute").toInt(), 0, 255);
      cmd.day    = (uint8_t)constrain(param("day").toInt(),    0, 255);
      cmd.month  = (uint8_t)constrain(param("month").toInt(),  0, 255);
    }

    char err[160];
    const uint32_t id = remote.request(cmd, err, sizeof(err));
    if (!id) {
      String body = "{\"ok\":false,\"msg\":\"";
      body += err;   // mensajes propios, sin comillas
      body += "\"}";
      request->send(409, "application/json", body);
      return;
    }
    request->send(202, "application/json", "{\"ok\":true,\"id\":" + String(id) + "}");
  });

  // Asistente de calibración de sondas. Lecturas siempre; cambios solo con el bloqueo
  // por mantenimiento activo y el cuarto en reposo (lo valida SensorCalibration).
  server.on("/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    request->send(200, "text/html", setLayOutInfo(CALIBRATION_HTML));
  });

  server.on("/api/calibration", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    AsyncResponseStream *res = request->beginResponseStream("application/json");
    res->addHeader("Cache-Control", "no-store");
    if (!calib.writeJSON(*res)) {
      delete res;
      request->send(503, "text/plain", "Calibration busy, try again");
      return;
    }
    request->send(res);
  });

  // POST /api/calibration  action=save|revert|reset  ch=ta|ts|tc  [r1,t1,r2,t2,r3,t3]
  // Mismo header X-Control que /api/control, por la misma razón.
  server.on("/api/calibration", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    if (!request->hasHeader("X-Control")) {
      request->send(403, "application/json", "{\"ok\":false,\"msg\":\"Missing X-Control header\"}");
      return;
    }
    auto param = [request](const String &name) -> String {
      return request->hasParam(name, true) ? request->getParam(name, true)->value() : String();
    };
    const String action = param("action"), key = param("ch");
    uint8_t ch = 255;
    for (uint8_t i = 0; i < CAL_CHANNELS; i++) if (key == SensorCalibration::channelKey(i)) ch = i;

    CalOp op;
    if      (action == "save")   op = CAL_OP_SAVE;
    else if (action == "revert") op = CAL_OP_REVERT;
    else if (action == "reset")  op = CAL_OP_RESET;
    else { request->send(400, "application/json", "{\"ok\":false,\"msg\":\"Unknown action\"}"); return; }

    CalPoint pts[CAL_POINTS];
    for (uint8_t i = 0; i < CAL_POINTS; i++) {
      const String r = param("r" + String(i + 1)), t = param("t" + String(i + 1));
      pts[i].raw  = (uint16_t)constrain(r.toInt(), 0, 65535);
      pts[i].temp = t.length() ? t.toFloat() : NAN;
    }

    char msg[160];
    const bool ok = calib.request(op, ch, pts, msg, sizeof(msg));
    String body = ok ? "{\"ok\":true,\"msg\":\"" : "{\"ok\":false,\"msg\":\"";
    body += msg;   // mensajes propios, sin comillas
    body += "\"}";
    request->send(ok ? 200 : 409, "application/json", body);
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

  server.on("/replace-config", HTTP_POST, [this](AsyncWebServerRequest *request){
    if (!request->authenticate(www_username, www_password)) return request->requestAuthentication();
    if (!s_upload_done) {
      sendResult(request, 400, false, "No file was received. Choose a config file and try again.");
    } else if (!s_upload_ok) {
      sendResult(request, 422, false, "The file is not valid JSON, so it was rejected. The current config.txt was kept.");
    } else {
      sendResult(request, 200, true, "config.txt replaced. It takes effect after a restart.", ",\"restart\":true");
    }
    s_upload_done = false;
  }, handleFileUpload);
  
  /*handling uploading firmware file */
  server.on("/reset", HTTP_POST, [&checkAuth](AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    sendResult(request, 200, true, "Restarting");
    events.emit(EV_SYSTEM, SRC_WEB, "Controller restart requested from the web interface");
    scheduleRestart();
  });

  server.on("/update", HTTP_POST, [&checkAuth]( AsyncWebServerRequest *request) {
    if(!checkAuth(request)) return;
    const bool failed = Update.hasError();
    request->send(200, "text/plain", failed ? "FAIL" : "OK");
    if (failed) events.emit(EV_ERROR, SRC_WEB, "Firmware update failed, restarting on the current firmware");
    else        events.emit(EV_SYSTEM, SRC_WEB, "Firmware update installed, restarting");
    scheduleRestart();
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
      const AsyncWebParameter* p = request->getParam(i);
      String keyPath = p->name();
      // logger.println(keyPath + ": " + p->value());
      ssidExists = ssidExists || keyPath == "SSID";
    }

    // Seleccionar el archivo a actualizar dependiendo de si existe el "SSID"
    const char* fileToUpdate = ssidExists ? CONFIG_FILE : DEFAULT_PARAMS_FILE;

    const String current = ConfigStore::read(fileToUpdate);
    if (current.length() == 0) {
      sendResult(request, 500, false, "Could not read the file on the controller. Nothing was saved.");
      return;
    }

    DynamicJsonDocument doc(4096);  // Ajusta el tamaño según tu archivo JSON
    DeserializationError error = deserializeJson(doc, current);
    if (error) {
      sendResult(request, 500, false, "The file on the controller is not valid JSON. Nothing was saved.");
      return;
    }

    // Actualizar el JSON con los nuevos valores del cuerpo
    std::vector<String> changes;
    updateJsonFromForm(request, doc, &changes);

    // Imprimir el documento actualizado en el Serial
    Serial.println("Printing updated doc:");
    serializeJson(doc, Serial);
    Serial.println();

    // Sin cambios no se toca la flash ni se ensucia la bitácora.
    if (changes.empty()) {
      sendResult(request, 200, true, "Nothing changed.", ",\"changes\":[],\"restart\":false");
      return;
    }

    // Persistir atómicamente en SPIFFS
    String out;
    serializeJson(doc, out);
    if (ConfigStore::write(fileToUpdate, out)) {
      String list = ",\"changes\":[";
      for (size_t i = 0; i < changes.size(); i++) {
        if (i) list += ',';
        list += jsonString(changes[i]);
      }
      list += "],\"restart\":true";
      sendResult(request, 200, true, String(changes.size()) + (changes.size() == 1 ? " value saved." : " values saved."), list);
      // Estos archivos se leen al arrancar: el cambio rige recién tras reiniciar.
      const bool is_config = strcmp(fileToUpdate, CONFIG_FILE) == 0;
      const char* file = is_config ? "config.txt" : "defaultParameters.txt";
      const size_t shown = changes.size() > 12 ? 12 : changes.size();
      for (size_t i = 0; i < shown; i++) {
        events.emit(is_config ? EV_SYSTEM : EV_PARAM, SRC_WEB, "%s %s (on restart)", file, changes[i].c_str());
      }
      if (changes.size() > shown) {
        events.emit(is_config ? EV_SYSTEM : EV_PARAM, SRC_WEB, "%s: %u more values changed",
                    file, (unsigned)(changes.size() - shown));
      }
    } else {
      sendResult(request, 500, false, "The controller could not write the file. Nothing was changed.");
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

  
  // WebSerial eliminado en la migración a arduino-esp32 3.x.
  (void)brigeSerial;
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
  // Contador de reinicios por WiFi fallido persistido en NVS vía Preferences
  // (antes EEPROM). Es un valor transitorio: si se pierde, vuelve a 0, que es
  // el default seguro. Namespace "wifi", clave "bootTries".
  Preferences prefs;
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    DEBUG("Wifi connecting...");

    notConnectedCounter++;
    if(notConnectedCounter > 7) { // ~16s sin conectar en este arranque
      prefs.begin("wifi", false);
      const uint32_t num_of_tries = prefs.getUInt("bootTries", 0);
      // Tras AP_MAX_BOOT_WIFI_TRIES reinicios fallidos, rendirse y caer a modo AP.
      if (num_of_tries >= AP_MAX_BOOT_WIFI_TRIES) {
        DEBUG("WiFi failed after max boot tries - falling back to AP mode");
        prefs.putUInt("bootTries", 0);   // resetear el contador para el próximo arranque manual
        prefs.end();
        return false;
      }
      DEBUG("Resetting due to Wifi not connecting...");
      prefs.putUInt("bootTries", num_of_tries + 1);
      prefs.end();
      ESP.restart();
    }
  }

  prefs.begin("wifi", false);
  prefs.putUInt("bootTries", 0);
  prefs.end();

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
  LOG_D(TAG_WIFI, "%s", message);
}

void WIFI::ERROR(ErrorType error){
  LOG_E(TAG_WIFI, "%s", errorMessages[error].c_str());
}



