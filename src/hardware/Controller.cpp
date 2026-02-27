#include "Controller.h"

RTC_DS3231 rtc;
WiFiUDP ntpUDP;
Adafruit_MLX90640 mlx;
TwoWire rtc_i2c = TwoWire(0);
NTPClient timeClient(ntpUDP);

// OneWire oneWire(ONE_WIRE_BUS);         // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
// DallasTemperature temp_sensor_bus(&oneWire);  // PASS our oneWire reference to Dallas Temperature.

const float voltage_per_step = REFERENCE / ADC__RESOLUTION;
const int16_t range = TEMPERATURE_MAX - TEMPERATURE_MIN;
const double temperature_per_step = range / REFERENCE;

Controller::Controller(/* args */) {
  setUpLogger();
  DEBUG("Controller created");
}

Controller::~Controller() {

}

void Controller::init() {
  setUpI2C();
  setUpIOS();
  logger.setupSD();
}

void Controller::setUpLogger() {
  // #ifdef WebSerial
    logger.init(115200);
    DEBUG("Logger set up");
  // #endif
}

void Controller::setUpIOS() {
  setUpAnalogInputs();
  setUpAnalogOutputs();
  setUpDigitalInputs();
  setUpDigitalOutputs();

  // Setting to LOW all pulled up pins
  for (uint8_t i = 0; i < pulled_up_size; i++) {
    pinMode(pulled_up[i], OUTPUT);
    digitalWrite(pulled_up[i], LOW);
  }
}


void Controller::setUpAnalogOutputs() {
  ledcSetup(AIR_PWM, FREQ, RESOLUTION);
  ledcAttachPin(AIR_PIN, AIR_PWM);
}

void Controller::setUpDigitalOutputs() {
  for (uint8_t i = 0; i < outputs_size; i++) {
    pinMode(outputs[i], OUTPUT);
    digitalWrite(outputs[i], LOW);
  }

  pinMode(VALVE_IO, OUTPUT);

}

void Controller::setUpDigitalInputs() {
  //Testing pourpose
  // pinMode(PORT_B0, INPUT_PULLUP);

  for (uint8_t i = 0; i < inputs_size; i++) pinMode(inputs[i], INPUT_PULLUP);
}

void Controller::setUpAnalogInputs() {
  
}

void Controller::setUpI2C() {
  rtc_i2c.begin(I2C_SDA, I2C_SCL);
  delay(10);
}

void Controller::setUpRTC() {
  rtc_last_valid_datetime = DateTime(__DATE__, __TIME__);
  rtc_last_valid_millis = millis();

  if (!tryConnectRTC(true)) {
    DEBUG("RTC not found at startup. Continuing with fallback clock.");
  } else {
    DateTime now = rtc.now();
    if (rtc.lostPower() || !isDateTimeValid(now)) {
      DEBUG("RTC time invalid. Syncing with NTP.");
      syncRTCWithNTP();
      now = rtc.now();
    }

    if (isDateTimeValid(now)) {
      rtc_last_valid_datetime = now;
      rtc_last_valid_millis = millis();
    }
  }

  if(isTsContactLess()) setUpIRTc();
}

void Controller::setUpIRTc() {
  DEBUG("Inicializando MLX90640...");
  ir_sensor_ready = false;
  ir_sensor_attempted = true;

  const uint8_t max_attempts = 5;
  for (uint8_t attempt = 1; attempt <= max_attempts; attempt++) {
    if (mlx.begin(MLX90640_I2CADDR_DEFAULT, &rtc_i2c)) {
      DEBUG("Sensor MLX90640 iniciado correctamente");
      mlx.setRefreshRate(MLX90640_4_HZ);
      ir_sensor_ready = true;
      return;
    }

    char buffer[70];
    snprintf(buffer, sizeof(buffer), "Error al iniciar MLX90640 (intento %u/%u)", attempt, max_attempts);
    DEBUG(buffer);
    delay(500);
  }

  DEBUG("MLX90640 no disponible, se omitirán las lecturas IR");
  ERROR(IR_NOT_FOUND);
}

float Controller::getIRTemp() {
  if (!ir_sensor_ready) {
    return -1;
  }

  float pixelTemps[32 * 24]; // Array temporal para almacenar las temperaturas de todos los píxeles
  float bottomTemps[ARRAY_SIZE]; // Inicializa con valores infinitos

  for (int i = 0; i < ARRAY_SIZE; i++) bottomTemps[i] = INFINITY;

  if (!mlx.getFrame(pixelTemps)) {
    for (int i = 0; i < 32 * 24; i++) checkAndInsertBottomTemps(pixelTemps[i], bottomTemps);

    // min, max and avg temps
    const float min = getMinTemp(bottomTemps);
    const float max = getMaxTemp(bottomTemps);
    const float avg = getAvgBottomTemp(bottomTemps);

    StaticJsonDocument<200> doc;

    doc["MIN"] = round(min * 100) / 100.0;
    doc["MAX"] = round(max * 100) / 100.0;
    doc["AVG"] = round(avg * 100) / 100.0;
    for(int i = 0; i < ARRAY_SIZE; i++) doc["values"][i] = round(bottomTemps[i] * 100) / 100.0;

    // Crear una cadena para almacenar el resultado JSON
    String output;
    serializeJson(doc, output);

    DEBUG(("Min: "+String(min)).c_str());
    DEBUG(("Max: "+String(max)).c_str());
    DEBUG(("Avg: "+String(avg)).c_str());

    // Serial.println(output.c_str());
  
    return avg;
  } 
  
  DEBUG("Error al leer el frame del sensor MLX90640");

  return -1;
}

float Controller::getAvgBottomTemp(float *temps){
  float sum = 0;
  for (int i = 0; i < ARRAY_SIZE; i++) sum += temps[i];
  return sum / ARRAY_SIZE;
}

float Controller::getMinTemp(float *temps) {
  float minTemp = temps[0];
  for (int i = 1; i < ARRAY_SIZE; i++) {
    if (temps[i] < minTemp) minTemp = temps[i];
  }
  return minTemp;
}

float Controller::getMaxTemp(float *temps) {
  float maxTemp = temps[0];
  for (int i = 1; i < ARRAY_SIZE; i++) {
    if (temps[i] > maxTemp) maxTemp = temps[i];
  }
  return maxTemp;
}

float roundToDecimalPlaces(float number, int decimalPlaces) {
    float multiplier = pow(10.0, decimalPlaces);
    return round(number * multiplier) / multiplier;
}

void Controller::checkAndInsertBottomTemps(float temp, float *temps) {
  if (temp < temps[ARRAY_SIZE - 1]) {
    temps[ARRAY_SIZE - 1] =temp; // Reemplaza el valor más alto con la nueva temperatura
    for (int i = ARRAY_SIZE - 1; i > 0; i--) {
      if (temps[i] < temps[i - 1]) {
        float tmp = temps[i];
        temps[i] = temps[i - 1];
        temps[i - 1] = tmp;
      }
    }
  }
}




bool Controller::isTsContactLess() {
  return ir_ts;
}

bool Controller::hasIRSensor() {
  if (!ir_sensor_ready && !ir_sensor_attempted) {
    setUpIRTc();
  }
  return ir_sensor_ready;
}

void Controller::setTsContactLess(bool value) {
  updateConfigJson("IR_TS", value);
  ir_ts = value;
  if (ir_ts && !ir_sensor_ready) setUpIRTc();
}

bool Controller::isLoraTc() {
  return lora_tc;
}

void Controller::setLoraTc(bool value) {
  updateConfigJson("LoRa_Tc", value);
  lora_tc = value;
}

bool Controller::isRTCConnected() {
  return tryConnectRTC();
}

DateTime Controller::getDateTime() {
  if (tryConnectRTC()) {
    // Re-sincronizar con NTP si el RTC acaba de reconectar
    if (rtc_needs_ntp_sync) syncRTCWithNTP();

    DateTime now = rtc.now();
    if (isDateTimeValid(now)) {
      rtc_last_valid_datetime = now;
      rtc_last_valid_millis = millis();
      return now;
    }

    // El RTC respondió pero con fecha inválida: puede ser batería muerta
    rtc_connected = false;
    rtc_needs_ntp_sync = true;
    DEBUG("RTC returned invalid datetime. Using fallback clock.");
  }

  return buildFallbackDateTime();
}

uint64_t Controller::readAnalogInput(uint8_t input) {
  return analogRead(input);
}

bool Controller::readDigitalInput(uint8_t input) {
  return digitalRead(input);
}

void Controller::writeAnalogOutput(uint8_t output, uint8_t value) {
  ledcWrite(AIR_PWM, value);
}

void Controller::writeDigitalOutput(uint8_t output, uint8_t value) {
  digitalWrite(output, value);
}

float Controller::readTempFrom(uint8_t channel) {
  const uint16_t raw_voltage_ch = analogRead(channel); 
  // const float voltage_ch = (raw_voltage_ch * voltage_per_step);
  // Serial.println(voltage_ch);
  // const float temp = (voltage_ch * temperature_per_step) + TEMPERATURE_MIN;
  const float temp = raw_voltage_ch*0.0263 -64.5; // ramp calculated with excel trhough manual calibration
  return temp;
}

// WIFI CLASS

void Controller::connectToWiFi(bool web_server, bool web_serial, bool OTA) {
  wifi.connectToWiFi();
  if(OTA) wifi.setUpOTA();
  if(web_server) wifi.setUpWebServer(web_serial);
}

void Controller::reconnectWiFi() {
  wifi.reconnect();
}

bool Controller::isWiFiConnected() {
  return wifi.isConnected();
}

bool Controller::refreshWiFiStatus() {
  return wifi.refreshWiFiStatus();
}

bool Controller::getConnectionStatus() {
  return wifi.getConnectionStatus();
}

String Controller::jsonBuilder(String keys[], float values[], int length) {
  // Crear un buffer estático para almacenar el JSON
  StaticJsonDocument<200> doc;

  // Añadir los datos al documento JSON
  for (int i = 0; i < length; i++) {
    doc[keys[i]] = values[i];
  }

  // Crear una cadena para almacenar el resultado JSON
  String output;
  serializeJson(doc, output);

  // Devolver la cadena JSON
  return output;
}

void Controller::loopOTA() {
  wifi.loopOTA();
}

void Controller::setUpWiFi(const char* ssid, const char* password, const char* hostname) {
  wifi.init(ssid, password, hostname);
}

void Controller::updateDefaultParameters(stage_parameters &stage1_params, stage_parameters &stage2_params, stage_parameters &stage3_params, room_parameters &room, data_tset &N_tset ){
  // Abre el archivo de configuración existente
  // File configFile = SPIFFS.open("/defaultParameters.txt", FILE_READ);
  File configFile = SD.open("/defaultParameters.txt", FILE_READ);
  if (!configFile) {
        DEBUG("Error al abrir el archivo de configuración para lectura");
    return;
  }

  // Lee el contenido en una cadena
  String content = configFile.readString();
  configFile.close();

  // Parsea el objeto JSON del archivo
  StaticJsonDocument<1024> doc; // Cambiado a StaticJsonDocument
  auto error = deserializeJson(doc, content);
  if (error) {
    Serial.println("Error al parsear el archivo de configuración");
    return;
  }

  // Update the values
  doc["stage1"]["f1Ontime"] = stage1_params.fanOnTime;
  doc["stage1"]["f1RevONTime"] = stage1_params.fanRevONTime;
  doc["stage1"]["f1Offtime"] = stage1_params.fanOffTime;

  doc["stage2"]["f1Ontime"] = stage2_params.fanOnTime;
  doc["stage2"]["f1RevONTime"] = stage2_params.fanRevONTime;
  doc["stage2"]["f1Offtime"] = stage2_params.fanOffTime;

  doc["stage2"]["s1Ontime"] = stage2_params.sprinklerOnTime;
  doc["stage2"]["s1Offtime"] = stage2_params.sprinklerOffTime;

  doc["stage3"]["f1Ontime"] = stage3_params.fanOnTime;
  doc["stage3"]["f1RevONTime"] = stage3_params.fanRevONTime;
  doc["stage3"]["f1Offtime"] = stage3_params.fanOffTime;
  doc["stage3"]["s1Ontime"] = stage3_params.sprinklerOnTime;
  doc["stage3"]["s1Offtime"] = stage3_params.sprinklerOffTime;

  doc["setPoint"]["A"] = room.A;
  doc["setPoint"]["B"] = room.B;
  doc["setPoint"]["coef_pid_fwd"] = room.coef_pid_fwd;
  doc["setPoint"]["coef_pid_rev"] = room.coef_pid_rev;
  
  doc["tset"]["tsSet"] = N_tset.ts;
  doc["tset"]["tcSet"] = N_tset.tc;

  // Open file for writing
  configFile = SD.open("/defaultParameters.txt", FILE_WRITE);
  if (!configFile) {
    DEBUG("Error al abrir el archivo de configuración para escritura");
    return;
  }

  // Serializa el JSON al archivo
  if (serializeJson(doc, configFile) == 0) {
    DEBUG("Error al escribir en el archivo de configuración");
  }

  configFile.close();
}

void Controller::runConfigFile(char* ssid, char* password, char* hostname, char* ip_address, uint16_t* port, char* mqtt_id, char* username, char* mqtt_password, char* prefix_topic, char* static_ip) {
  // Iniciar SPIFFS
  // if (!SPIFFS.begin(true)) {
  //   DEBUG("An error has occurred while mounting SPIFFS");
  //   return;
  // }

  // Leer archivo de configuración
  File file = SD.open(CONFIG_FILE);
  if (!file) {
    while (true){
      DEBUG("Failed to open config file");
      delay(1000);
    }
    // Pending What to do if the file is not found
    return;
  }

  // Tamaño para el documento JSON
  size_t size = file.size();
  std::unique_ptr<char[]> buf(new char[size]);
  file.readBytes(buf.get(), size);
  file.close();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
      DEBUG("Failed to parse config file");
    return;
  }

  // Asignar valores y verificar si están presentes en el JSON
  if (doc.containsKey("SSID")) strlcpy(ssid, doc["SSID"], SSID_SIZE);
  if (doc.containsKey("WIFI_PASSWORD")) strlcpy(password, doc["WIFI_PASSWORD"], PASSWORD_SIZE);
  if (doc.containsKey("HOST_NAME")) strlcpy(hostname, doc["HOST_NAME"], HOSTNAME_SIZE);
  if (doc.containsKey("STATIC_IP")) strlcpy(static_ip, doc["STATIC_IP"], HOSTNAME_SIZE);
  if (doc.containsKey("IP_ADDRESS")) strlcpy(ip_address, doc["IP_ADDRESS"], IP_ADDRESS_SIZE);
  if (doc.containsKey("STATIC_IP") && doc.containsKey("GATEWAY")) {
    const char* ip = doc["STATIC_IP"];
    const char* gateway = doc["GATEWAY"];
    wifi.setStaticIP(ip, gateway);
  } 
  if (doc.containsKey("PORT")) *port = doc["PORT"];
  if (doc.containsKey("USERNAME")) strlcpy(username, doc["USERNAME"], HOSTNAME_SIZE);
  // if (doc.containsKey("TOPIC")) strlcpy(prefix_topic, doc["TOPIC"], HOSTNAME_SIZE);
  if (doc.containsKey("MQTT_ID")) strlcpy(mqtt_id, doc["MQTT_ID"], MQTT_ID_SIZE);
  if (doc.containsKey("MQTT_PASSWORD")) strlcpy(mqtt_password, doc["MQTT_PASSWORD"], MQTT_PASSWORD_SIZE);
  if(doc.containsKey("IR_TS")) ir_ts = doc["IR_TS"];
  #ifndef TIME_ZONE_OFFSET_HRS
    if(doc.containsKey("TIME_ZONE_OFFSET_HRS")) TIME_ZONE_OFFSET_HRS = doc["TIME_ZONE_OFFSET_HRS"];
  #endif
  DEBUG(("TIME_ZONE_OFFSET_HRS: " + String(TIME_ZONE_OFFSET_HRS)).c_str());
  if(doc.containsKey("LoRa_Tc")) setLoraTc(doc["LoRa_Tc"]);
  if(doc.containsKey("WEB_SERIAL")) logger.setOutput(doc["WEB_SERIAL"]);
  // logging all values
  DEBUG(("SSID: " + String(ssid)).c_str());
  DEBUG(("WIFI_PASSWORD: " + String(password)).c_str());
  DEBUG(("HOST_NAME: " + String(hostname)).c_str());
  DEBUG(("IP_ADDRESS: " + String(ip_address)).c_str());
  DEBUG(("PORT: " + String(*port)).c_str());
  DEBUG(("USERNAME: " + String(username)).c_str());
  DEBUG(("TOPIC: " + String(prefix_topic)).c_str());
  DEBUG(("MQTT_ID: " + String(mqtt_id)).c_str());
  DEBUG(("MQTT_PASSWORD: " + String(mqtt_password)).c_str());

}

void Controller::setUpDefaultParameters(stage_parameters &stage1_params, stage_parameters &stage2_params, stage_parameters &stage3_params, room_parameters &room, data_tset &N_tset){
  File file = SD.open("/defaultParameters.txt", "r");
  if (!file) {
    while (true){
      DEBUG("Failed to open default parameters file");
      delay(1000);
    }
    
    return;
  }

  String jsonText = file.readString();
  file.close();

  // Parsea el JSON
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonText);
  if (error) {
    DEBUG("Error al parsear el JSON");
    return;
  }

  stage1_params.fanOnTime = doc["stage1"]["f1Ontime"];
  stage1_params.fanRevONTime = doc["stage1"]["f1RevONtime"];
  stage1_params.fanOffTime = doc["stage1"]["f1Offtime"];
  stage1_params.sprinklerOnTime = doc["stage1"]["s1Ontime"];
  stage1_params.sprinklerOffTime = doc["stage1"]["s1Offtime"];

  stage2_params.fanOnTime = doc["stage2"]["f1Ontime"];
  stage2_params.fanRevONTime = doc["stage2"]["f1RevONtime"];
  stage2_params.fanOffTime = doc["stage2"]["f1Offtime"];
  stage2_params.sprinklerOnTime = doc["stage2"]["s1Ontime"];
  stage2_params.sprinklerOffTime = doc["stage2"]["s1Offtime"];

  stage3_params.fanOnTime = doc["stage3"]["f1Ontime"];
  stage3_params.fanRevONTime = doc["stage3"]["f1RevONtime"];
  stage3_params.fanOffTime = doc["stage3"]["f1Offtime"];
  stage3_params.sprinklerOnTime = doc["stage3"]["s1Ontime"];
  stage3_params.sprinklerOffTime = doc["stage3"]["s1Offtime"];

  if(stage1_params.sprinklerOffTime < MIN_OFFTIME_STAGE1 ) stage1_params.sprinklerOffTime = MIN_OFFTIME_STAGE1;
  if(stage2_params.sprinklerOffTime < MIN_OFFTIME_STAGE2 ) stage2_params.sprinklerOffTime = MIN_OFFTIME_STAGE2;
  if(stage3_params.sprinklerOffTime < MIN_OFFTIME_STAGE3 ) stage3_params.sprinklerOffTime  = MIN_OFFTIME_STAGE3;
  

  room.A = doc["setPoint"]["A"];
  room.B = doc["setPoint"]["B"];
  room.coef_pid_fwd = doc["setPoint"]["coef_pid_fwd"] | 100;
  room.coef_pid_rev = doc["setPoint"]["coef_pid_rev"] | 100;

  N_tset.ts = doc["tset"]["tsSet"];
  N_tset.tc = doc["tset"]["tcSet"];

  // // log all data


}

void Controller::WiFiLoop() {
  if (!isWiFiConnected()) {
    reconnectWiFi();
    vTaskDelay(500 / portTICK_PERIOD_MS);
    return;
  }
}

void Controller::DEBUG(const char *message){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), "[Controller]: %s", message);
  logger.println(buffer);
}

void Controller::ERROR(ErrorType error){
  char buffer[100];
  snprintf(buffer, sizeof(buffer), " -> Controller]: %s", ERROR_MESSAGES[error]);
  logger.printError(buffer);
}

void Controller::turnOnFan(bool value, bool CCW) {
  if (value) {
    fan_state = true;
    digitalWrite(FAN_CW_IO, CCW ? LOW : HIGH);
    digitalWrite(FAN_CCW_IO, CCW ? HIGH : LOW);
  } else {
    fan_state = false;
    digitalWrite(FAN_CW_IO, LOW);
    digitalWrite(FAN_CCW_IO, LOW);
  }
}

bool Controller::getFanState() {
  return fan_state;
}

StageState Controller::getLastState() {
  StageState last_state;
  preferences.begin("recovery", false);
  last_state.stage = (SystemState)preferences.getUInt("stage", IDLE);
  last_state.step = preferences.getUInt("step", 0);
  preferences.end();

  return last_state;
}

void Controller::saveLastState(StageState current_state) {
  preferences.begin("recovery", false);
  preferences.putUInt("stage", current_state.stage);
  preferences.putUInt("step", current_state.step);
  preferences.end();
}

bool Controller::thresLastState() {
    return preferences.getBool("thres", false);
}

void Controller::saveLogToSD(const String &message) {
  const DateTime now = getDateTime();
  if (logger.getFileName() == DEFAULT_LOG_FILE) logger.setFileName(now);
  logger.writeSD(message, now);
}

bool Controller::isDateTimeValid(const DateTime& dt) const {
  return dt.year() >= 2024 && dt.year() <= 2099;
}

bool Controller::tryConnectRTC(bool force) {
  const uint32_t nowMs = millis();
  const uint32_t reconnectIntervalMs = 5000;

  if (!force && rtc_connected) return true;
  if (!force && (nowMs - rtc_last_reconnect_attempt) < reconnectIntervalMs) return false;

  rtc_last_reconnect_attempt = nowMs;

  // Primer intento normal
  rtc_connected = rtc.begin(&rtc_i2c);

  if (!rtc_connected) {
    // El bus I2C puede estar colgado: reset completo y reintento
    resetI2CBus();
    rtc_connected = rtc.begin(&rtc_i2c);
  }

  const bool just_reconnected = rtc_connected && !rtc_last_reported_connected;

  if (rtc_connected != rtc_last_reported_connected) {
    if (rtc_connected) {
      DateTime now = rtc.now();
      char buf[60];
      snprintf(buf, sizeof(buf), "RTC connected - time: %04d/%02d/%02d %02d:%02d:%02d",
               now.year(), now.month(), now.day(),
               now.hour(), now.minute(), now.second());
      DEBUG(buf);
    } else {
      DEBUG("RTC disconnected");
    }
    rtc_last_reported_connected = rtc_connected;
  }

  // Si acaba de reconectar, programar re-sync NTP para validar el tiempo interno
  if (just_reconnected) rtc_needs_ntp_sync = true;

  return rtc_connected;
}

void Controller::resetI2CBus() {
  rtc_i2c.end();
  delay(20);
  // Enviar 9 pulsos de reloj para desbloquear dispositivos I2C colgados
  pinMode(I2C_SDA, OUTPUT);
  pinMode(I2C_SCL, OUTPUT);
  digitalWrite(I2C_SDA, HIGH);
  for (uint8_t i = 0; i < 9; i++) {
    digitalWrite(I2C_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(I2C_SCL, LOW);
    delayMicroseconds(5);
  }
  digitalWrite(I2C_SDA, LOW);  // START condition
  delayMicroseconds(5);
  digitalWrite(I2C_SDA, HIGH); // STOP condition
  delayMicroseconds(5);
  delay(10);
  rtc_i2c.begin(I2C_SDA, I2C_SCL);
  delay(10);
}

void Controller::syncRTCWithNTP() {
  if (!wifi.isConnected()) {
    DEBUG("NTP sync skipped: WiFi not connected");
    return;
  }

  timeClient.begin();
  timeClient.setTimeOffset(SECS_IN_HR * TIME_ZONE_OFFSET_HRS);
  timeClient.setUpdateInterval(SECS_IN_HR);

  bool updated = timeClient.update();
  if (!updated) updated = timeClient.forceUpdate();

  if (!updated) {
    DEBUG("NTP sync failed: no response");
    return;
  }

  const unsigned long epochTime = timeClient.getEpochTime();
  if (epochTime < 1700000000UL) {
    DEBUG("NTP sync failed: invalid epoch");
    return;
  }

  const DateTime ntpTime(epochTime);
  rtc.adjust(ntpTime);

  rtc_last_valid_datetime = ntpTime;
  rtc_last_valid_millis = millis();
  rtc_needs_ntp_sync = false;

  char buf[60];
  snprintf(buf, sizeof(buf), "RTC synced via NTP: %04d/%02d/%02d %02d:%02d:%02d",
           ntpTime.year(), ntpTime.month(), ntpTime.day(),
           ntpTime.hour(), ntpTime.minute(), ntpTime.second());
  DEBUG(buf);
}

DateTime Controller::buildFallbackDateTime() const {
  if (rtc_last_valid_millis == 0) {
    return DateTime(__DATE__, __TIME__);
  }

  const uint32_t elapsedSeconds = (millis() - rtc_last_valid_millis) / 1000;
  return rtc_last_valid_datetime + TimeSpan(elapsedSeconds);
}
