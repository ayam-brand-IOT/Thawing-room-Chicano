#include "Controller.h"
#include "config_defaults.h"

RTC_DS3231 rtc;
WiFiUDP ntpUDP;
Adafruit_MLX90640 mlx;
TwoWire rtc_i2c = TwoWire(0);
NTPClient timeClient(ntpUDP);

// RTC memory: persiste entre reinicios de software (no power-off)
RTC_DATA_ATTR static uint32_t g_stage2_epoch = 0;
RTC_DATA_ATTR static bool     g_stage2_set   = false;

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
  digitalWrite(VALVE_IO, LOW);

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
      syncInternalRTC(now);  // Mantener ESP32 RTC interno sincronizado
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

void Controller::forceNTPSync() {
  if (!wifi.isConnected()) {
    DEBUG("NTP sync skipped: no WiFi");
    return;
  }
  if (rtc_connected) {
    syncRTCWithNTP();                // NTP → RTC externo
    DateTime now_utc = rtc.now();
    syncInternalRTC(now_utc);        // propaga al interno
    DEBUG("NTP sync: external + internal RTC updated");
  } else {
    if (syncNTPToInternalRTC()) {    // solo interno si no hay externo
      DEBUG("NTP sync: internal RTC only (no ext RTC)");
    }
  }
}

DateTime Controller::getDateTime() {
  if (tryConnectRTC()) {
    // Re-sincronizar con NTP si el RTC acaba de reconectar
    if (rtc_needs_ntp_sync) syncRTCWithNTP();

    DateTime now = rtc.now();
    if (isDateTimeValid(now)) {
      rtc_last_valid_datetime = now;
      rtc_last_valid_millis = millis();
      syncInternalRTC(now);           // mantener interno en sync con externo
      ntp_synced_to_internal = true;
      return now;
    }

    // El RTC respondió pero con fecha inválida: puede ser batería muerta
    rtc_connected = false;
    rtc_needs_ntp_sync = true;
    DEBUG("RTC returned invalid datetime.");
  }

  // Sin RTC externo: primera sync NTP al interno si WiFi disponible
  if (!ntp_synced_to_internal && wifi.isConnected()) {
    syncNTPToInternalRTC();
  }

  // Fallback 1: ESP32 RTC interno
  DateTime internal = getDateTimeFromInternalRTC();
  if (isDateTimeValid(internal)) {
    static bool internal_rtc_warned = false;
    if (!internal_rtc_warned) {
      DEBUG("Ext RTC unavailable - using ESP32 internal RTC");
      internal_rtc_warned = true;
    }
    return internal;
  }

  // Fallback 2: última fecha válida + delta millis
  static bool millis_warned = false;
  if (!millis_warned) {
    DEBUG("All RTC sources failed - millis fallback");
    millis_warned = true;
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

bool Controller::canUseInternet() {
  return wifi.canUseInternet();
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

bool Controller::updateDefaultParameters(stage_parameters &stage1_params, stage_parameters &stage2_params, stage_parameters &stage3_params, room_parameters &room, data_tset &N_tset ){
  // Abre el archivo de configuración existente
  // File configFile = SPIFFS.open("/defaultParameters.txt", FILE_READ);
  File configFile = SD.open("/defaultParameters.txt", FILE_READ);
  if (!configFile) {
        DEBUG("Error al abrir el archivo de configuración para lectura");
    return false;
  }

  // Lee el contenido en una cadena
  String content = configFile.readString();
  configFile.close();

  // Parsea el objeto JSON del archivo
  StaticJsonDocument<1024> doc; // Cambiado a StaticJsonDocument
  auto error = deserializeJson(doc, content);
  if (error) {
    Serial.println("Error al parsear el archivo de configuración");
    return false;
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
    return false;
  }

  // Serializa el JSON al archivo
  if (serializeJson(doc, configFile) == 0) {
    DEBUG("Error al escribir en el archivo de configuración");
    configFile.close();
    return false;
  }

  configFile.close();
  return true;
}

int Controller::runConfigFile(char* ssid, char* password, char* hostname, char* ip_address, uint16_t* port, char* mqtt_id, char* username, char* mqtt_password, char* prefix_topic, char* static_ip) {
  const char* default_ssid = DEFAULT_SSID;
  const char* default_password = DEFAULT_WIFI_PASSWORD;
  const char* default_hostname = DEFAULT_HOSTNAME;
  const char* default_ip_address = DEFAULT_IP_ADDRESS;
  const char* default_static_ip = DEFAULT_STATIC_IP;
  const char* default_gateway = DEFAULT_GATEWAY;
  const uint16_t default_port = DEFAULT_PORT;
  const char* default_mqtt_id = DEFAULT_MQTT_ID;
  const char* default_username = DEFAULT_MQTT_USERNAME;
  const char* default_mqtt_password = DEFAULT_MQTT_PASSWORD;
  const int8_t default_tz_offset = DEFAULT_TIME_ZONE_OFFSET_HRS;
  const bool default_ir_ts = DEFAULT_IR_TS;
  const bool default_lora_tc = DEFAULT_LORA_TC;
  const bool default_web_serial = DEFAULT_WEB_SERIAL;

  auto applyDefaults = [&]() {
    strlcpy(ssid, default_ssid, SSID_SIZE);
    strlcpy(password, default_password, PASSWORD_SIZE);
    strlcpy(hostname, default_hostname, HOSTNAME_SIZE);
    strlcpy(ip_address, default_ip_address, IP_ADDRESS_SIZE);
    strlcpy(static_ip, default_static_ip, IP_ADDRESS_SIZE);
    *port = default_port;
    strlcpy(mqtt_id, default_mqtt_id, MQTT_ID_SIZE);
    strlcpy(username, default_username, MQTT_USERNAME_SIZE);
    strlcpy(mqtt_password, default_mqtt_password, MQTT_PASSWORD_SIZE);
    ir_ts = default_ir_ts;
    setLoraTc(default_lora_tc);
    logger.setOutput(default_web_serial ? Logger::WEBSERIAL : Logger::HW_SERIAL);
    #ifndef TIME_ZONE_OFFSET_HRS
      TIME_ZONE_OFFSET_HRS = default_tz_offset;
    #endif
    wifi.setStaticIP(default_static_ip, default_gateway);
  };

  applyDefaults();

  if (!logger.isSdAvailable()) {
    DEBUG("No SD available: using built-in defaults");
    return 2;
  }

  File file = SD.open(CONFIG_FILE, FILE_READ);
  if (!file) {
    DEBUG("Config file missing or unreadable: creating default config file");
    writeDefaultConfigFile();
    return 1;
  }

  size_t size = file.size();
  if (size == 0) {
    file.close();
    DEBUG("Config file is empty: recreating default config file");
    writeDefaultConfigFile();
    return 1;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  file.readBytes(buf.get(), size);
  buf[size] = '\0';
  file.close();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    DEBUG("Failed to parse config file: recreating default config file");
    writeDefaultConfigFile();
    return 1;
  }

  if (doc.containsKey("SSID")) strlcpy(ssid, doc["SSID"], SSID_SIZE);
  if (doc.containsKey("WIFI_PASSWORD")) strlcpy(password, doc["WIFI_PASSWORD"], PASSWORD_SIZE);
  if (doc.containsKey("HOST_NAME")) strlcpy(hostname, doc["HOST_NAME"], HOSTNAME_SIZE);
  if (doc.containsKey("STATIC_IP")) strlcpy(static_ip, doc["STATIC_IP"], IP_ADDRESS_SIZE);
  if (doc.containsKey("IP_ADDRESS")) strlcpy(ip_address, doc["IP_ADDRESS"], IP_ADDRESS_SIZE);
  if (doc.containsKey("STATIC_IP") && doc.containsKey("GATEWAY")) {
    const char* ip = doc["STATIC_IP"];
    const char* gateway = doc["GATEWAY"];
    wifi.setStaticIP(ip, gateway);
  }
  if (doc.containsKey("PORT")) *port = doc["PORT"];
  if (doc.containsKey("USERNAME")) strlcpy(username, doc["USERNAME"], MQTT_USERNAME_SIZE);
  if (doc.containsKey("MQTT_ID")) strlcpy(mqtt_id, doc["MQTT_ID"], MQTT_ID_SIZE);
  if (doc.containsKey("MQTT_PASSWORD")) strlcpy(mqtt_password, doc["MQTT_PASSWORD"], MQTT_PASSWORD_SIZE);
  if(doc.containsKey("IR_TS")) ir_ts = doc["IR_TS"];
  #ifndef TIME_ZONE_OFFSET_HRS
    if(doc.containsKey("TIME_ZONE_OFFSET_HRS")) TIME_ZONE_OFFSET_HRS = doc["TIME_ZONE_OFFSET_HRS"];
  #endif
  DEBUG(("TIME_ZONE_OFFSET_HRS: " + String(TIME_ZONE_OFFSET_HRS)).c_str());
  if(doc.containsKey("LoRa_Tc")) setLoraTc(doc["LoRa_Tc"]);
  if(doc.containsKey("WEB_SERIAL")) logger.setOutput(doc["WEB_SERIAL"]);

  DEBUG(("SSID: " + String(ssid)).c_str());
  DEBUG(("WIFI_PASSWORD: " + String(password)).c_str());
  DEBUG(("HOST_NAME: " + String(hostname)).c_str());
  DEBUG(("IP_ADDRESS: " + String(ip_address)).c_str());
  DEBUG(("PORT: " + String(*port)).c_str());
  DEBUG(("USERNAME: " + String(username)).c_str());
  DEBUG(("MQTT_ID: " + String(mqtt_id)).c_str());
  DEBUG(("MQTT_PASSWORD: " + String(mqtt_password)).c_str());
  return 0;
}

bool Controller::writeDefaultConfigFile() {
  StaticJsonDocument<512> doc;
  doc["SSID"] = DEFAULT_SSID;
  doc["WIFI_PASSWORD"] = DEFAULT_WIFI_PASSWORD;
  doc["HOST_NAME"] = DEFAULT_HOSTNAME;
  doc["IP_ADDRESS"] = DEFAULT_IP_ADDRESS;
  doc["STATIC_IP"] = DEFAULT_STATIC_IP;
  doc["GATEWAY"] = DEFAULT_GATEWAY;
  doc["PORT"] = DEFAULT_PORT;
  doc["MQTT_ID"] = DEFAULT_MQTT_ID;
  doc["USERNAME"] = DEFAULT_MQTT_USERNAME;
  doc["MQTT_PASSWORD"] = DEFAULT_MQTT_PASSWORD;
  doc["TIME_ZONE_OFFSET_HRS"] = DEFAULT_TIME_ZONE_OFFSET_HRS;
  doc["IR_TS"] = DEFAULT_IR_TS;
  doc["LoRa_Tc"] = DEFAULT_LORA_TC;
  doc["WEB_SERIAL"] = DEFAULT_WEB_SERIAL;

  File file = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!file) {
    DEBUG("Failed to create default config file on SD");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    DEBUG("Failed to write default config file to SD");
    file.close();
    return false;
  }

  file.close();
  DEBUG("Default config file created on SD");
  return true;
}

void Controller::setUpDefaultParameters(stage_parameters &stage1_params, stage_parameters &stage2_params, stage_parameters &stage3_params, room_parameters &room, data_tset &N_tset){
  if (!SD.exists("/defaultParameters.txt")) {
    DEBUG("Default parameters file missing: using built-in defaults");
    stage1_params.fanOnTime = 1;
    stage1_params.fanRevONTime = 0;
    stage1_params.fanOffTime = 1;
    stage1_params.sprinklerOnTime = 0;
    stage1_params.sprinklerOffTime = MIN_OFFTIME_STAGE1;

    stage2_params.fanOnTime = 1;
    stage2_params.fanRevONTime = 0;
    stage2_params.fanOffTime = 1;
    stage2_params.sprinklerOnTime = 0;
    stage2_params.sprinklerOffTime = MIN_OFFTIME_STAGE2;

    stage3_params.fanOnTime = 1;
    stage3_params.fanRevONTime = 0;
    stage3_params.fanOffTime = 1;
    stage3_params.sprinklerOnTime = 0;
    stage3_params.sprinklerOffTime = MIN_OFFTIME_STAGE3;

    room.A = 0;
    room.B = 0;
    room.coef_pid_fwd = 100;
    room.coef_pid_rev = 100;

    N_tset.ts = 0;
    N_tset.tc = 0;
    return;
  }

  File file = SD.open("/defaultParameters.txt", "r");
  if (!file) {
    DEBUG("Default parameters file missing: using built-in defaults");
    stage1_params.fanOnTime = 1;
    stage1_params.fanRevONTime = 0;
    stage1_params.fanOffTime = 1;
    stage1_params.sprinklerOnTime = 0;
    stage1_params.sprinklerOffTime = MIN_OFFTIME_STAGE1;

    stage2_params.fanOnTime = 1;
    stage2_params.fanRevONTime = 0;
    stage2_params.fanOffTime = 1;
    stage2_params.sprinklerOnTime = 0;
    stage2_params.sprinklerOffTime = MIN_OFFTIME_STAGE2;

    stage3_params.fanOnTime = 1;
    stage3_params.fanRevONTime = 0;
    stage3_params.fanOffTime = 1;
    stage3_params.sprinklerOnTime = 0;
    stage3_params.sprinklerOffTime = MIN_OFFTIME_STAGE3;

    room.A = 0;
    room.B = 0;
    room.coef_pid_fwd = 100;
    room.coef_pid_rev = 100;

    N_tset.ts = 0;
    N_tset.tc = 0;
    return;
  }

  String jsonText = file.readString();
  file.close();

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonText);
  if (error) {
    DEBUG("Error al parsear el JSON: using built-in defaults");
    stage1_params.fanOnTime = 1;
    stage1_params.fanRevONTime = 0;
    stage1_params.fanOffTime = 1;
    stage1_params.sprinklerOnTime = 0;
    stage1_params.sprinklerOffTime = MIN_OFFTIME_STAGE1;

    stage2_params.fanOnTime = 1;
    stage2_params.fanRevONTime = 0;
    stage2_params.fanOffTime = 1;
    stage2_params.sprinklerOnTime = 0;
    stage2_params.sprinklerOffTime = MIN_OFFTIME_STAGE2;

    stage3_params.fanOnTime = 1;
    stage3_params.fanRevONTime = 0;
    stage3_params.fanOffTime = 1;
    stage3_params.sprinklerOnTime = 0;
    stage3_params.sprinklerOffTime = MIN_OFFTIME_STAGE3;

    room.A = 0;
    room.B = 0;
    room.coef_pid_fwd = 100;
    room.coef_pid_rev = 100;

    N_tset.ts = 0;
    N_tset.tc = 0;
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
}

void Controller::WiFiLoop() {
  reconnectWiFi();
  vTaskDelay(500 / portTICK_PERIOD_MS);
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

void Controller::syncInternalRTC(const DateTime& dt) {
  struct timeval tv;
  tv.tv_sec  = (time_t)dt.unixtime();
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
}

bool Controller::syncNTPToInternalRTC() {
  WiFiUDP localUDP;
  NTPClient localClient(localUDP, "pool.ntp.org", 0, 60000);  // UTC puro
  localClient.begin();

  bool ok = false;
  const uint8_t max_attempts = 5;
  for (uint8_t i = 0; i < max_attempts && !ok; i++) {
    if (localClient.update()) {
      const unsigned long epochTime = localClient.getEpochTime();
      if (epochTime > 1000000000UL) {
        struct timeval tv;
        tv.tv_sec  = (time_t)epochTime;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr);
        ntp_synced_to_internal = true;
        rtc_last_valid_datetime = DateTime((uint32_t)epochTime);
        rtc_last_valid_millis   = millis();
        char buf[50];
        snprintf(buf, sizeof(buf), "NTP->internal RTC synced (epoch: %lu)", epochTime);
        DEBUG(buf);
        ok = true;
      }
    }
    if (!ok) delay(200);
  }

  if (!ok) DEBUG("NTP->internal RTC sync failed");
  localClient.end();
  return ok;
}

DateTime Controller::getDateTimeFromInternalRTC() const {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return DateTime((uint32_t)tv.tv_sec);
}

void Controller::saveStage2StartTime(uint8_t hour, uint8_t minute, uint8_t day, uint8_t month) {
  if (day == 0 || month == 0) {
    DEBUG("Stage2 time invalid: day/month are 0");
    return;
  }

  DateTime now = getDateTime();
  int16_t year = now.year();

  DateTime target(year, month, day, hour, minute, 0);

  // Si la fecha objetivo ya pasó, usar el año siguiente
  if (target.unixtime() < now.unixtime()) {
    target = DateTime(year + 1, month, day, hour, minute, 0);
  }

  g_stage2_epoch = target.unixtime();
  g_stage2_set   = true;

  // Persistir en flash: sobrevive power-off (RTC_DATA_ATTR no sobrevive)
  preferences.begin("stage2", false);
  preferences.putUInt("epoch", g_stage2_epoch);
  preferences.putBool("set", true);
  preferences.end();

  const long remaining = getRemainingMinutesToStage2(now);
  char buf[80];
  snprintf(buf, sizeof(buf), "Stage2 target: %04d/%02d/%02d %02d:%02d -> %ld min remaining",
           target.year(), target.month(), target.day(),
           target.hour(), target.minute(), remaining);
  DEBUG(buf);
}

long Controller::getRemainingMinutesToStage2(const DateTime& now) const {
  if (!g_stage2_set) return -1;
  const long diff = (long)g_stage2_epoch - (long)now.unixtime();
  return diff / 60L;
}

bool Controller::hasStage2TimeElapsed(const DateTime& now) const {
  if (!g_stage2_set) return false;
  return now.unixtime() >= g_stage2_epoch;
}

bool Controller::isStage2TimeSet() const {
  return g_stage2_set;
}

void Controller::loadStage2StartTime() {
  // RTC_DATA_ATTR se pierde en power-off: recuperar desde Preferences flash
  if (g_stage2_set) return;  // ya en RAM, no hace falta

  preferences.begin("stage2", true);
  const bool saved = preferences.getBool("set", false);
  if (saved) {
    g_stage2_epoch = preferences.getUInt("epoch", 0);
    g_stage2_set   = (g_stage2_epoch > 0);
  }
  preferences.end();

  if (g_stage2_set) {
    DEBUG("Stage2 epoch restored from flash");
  } else {
    DEBUG("Stage2 epoch not found in flash");
  }
}
