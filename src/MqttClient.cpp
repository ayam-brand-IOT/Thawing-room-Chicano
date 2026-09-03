#include "MqttClient.h"
#include "mqtt_certs.h"
#include <SPIFFS.h>

// Ruta opcional en SPIFFS para sobrescribir el CA embebido sin reflashear.
#define MQTT_CA_SPIFFS_PATH "/mqtt_ca.pem"

// Timeouts que ACOTAN el bloqueo del reconnect dentro del loop de control. Sin esto,
// un broker inalcanzable con TLS cuelga el loop ~30s en el handshake y dispara el
// watchdog. Con el bloqueo acotado a ~socket+handshake, el WDT (15s) es seguro.
#define MQTT_SOCKET_TIMEOUT_S         5   // socket PubSubClient (default 15)
#define MQTT_TLS_HANDSHAKE_TIMEOUT_S  8   // handshake TLS (el que se colgaba ~30s)

WiFiClient esp32Client;            // transporte en claro (TLS deshabilitado)
WiFiClientSecure esp32SecureClient; // transporte TLS (server-auth con CA)
PubSubClient mqttClient(esp32Client);

// void subscribeReceive(char* topic, byte* payload, unsigned int length);

void MqttClient::setTopicPrefix(const char* prefix) {
  if (prefix == nullptr || prefix[0] == '\0') {
    strlcpy(topic_prefix, DEFAULT_TOPIC_PREFIX, sizeof(topic_prefix));
  } else {
    strlcpy(topic_prefix, prefix, sizeof(topic_prefix));

    // Normalizar el '/' final: sin esto "cfpp9" produciría "cfpp9Ta" en vez de "cfpp9/Ta".
    const size_t len = strlen(topic_prefix);
    if (len > 0 && topic_prefix[len - 1] != '/' && len + 1 < sizeof(topic_prefix)) {
      topic_prefix[len]     = '/';
      topic_prefix[len + 1] = '\0';
    }
  }

  DEBUG(("Topic prefix: " + String(topic_prefix)).c_str());
}

// prefijo + sufijo en un buffer reusable. snprintf trunca en vez de desbordar.
const char* MqttClient::fullTopic(const char* suffix) {
  snprintf(topic_buf, sizeof(topic_buf), "%s%s", topic_prefix, suffix ? suffix : "");
  return topic_buf;
}

void MqttClient::setTLS(bool enabled, const String& ca_override) {
  use_tls = enabled;
  if (!enabled) {
    mqttClient.setClient(esp32Client);
    DEBUG("TLS deshabilitado: MQTT en claro");
    return;
  }

  // Prioridad: PEM pasado por parámetro > /mqtt_ca.pem en SPIFFS > CA embebido.
  if (ca_override.length() > 0) {
    ca_cert = ca_override;
    DEBUG("TLS: usando CA pasado por parámetro");
  } else {
    String fromFs;
    if (SPIFFS.exists(MQTT_CA_SPIFFS_PATH)) {
      File f = SPIFFS.open(MQTT_CA_SPIFFS_PATH, "r");
      if (f) { fromFs = f.readString(); f.close(); }
    }
    if (fromFs.indexOf("BEGIN CERTIFICATE") >= 0) {
      ca_cert = fromFs;
      DEBUG("TLS: usando CA desde SPIFFS (" MQTT_CA_SPIFFS_PATH ")");
    } else {
      ca_cert = String(MQTT_CA_CERT);
      DEBUG("TLS: usando CA embebido en firmware");
    }
  }

  esp32SecureClient.setCACert(ca_cert.c_str());
  esp32SecureClient.setHandshakeTimeout(MQTT_TLS_HANDSHAKE_TIMEOUT_S);  // no colgar el loop en un handshake inalcanzable
  mqttClient.setClient(esp32SecureClient);
  DEBUG("TLS habilitado: el broker se verifica contra el CA");
}

bool MqttClient::isTLS() {
  return use_tls;
}

void MqttClient::connect(const char *domain, uint16_t port, const char *id, const char *username, const char *password) {
  strncpy(mqtt_domain, domain, sizeof(mqtt_domain) - 1);
  mqtt_domain[sizeof(mqtt_domain) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  mqtt_port = port;

  strncpy(mqtt_id, id, sizeof(mqtt_id) - 1);
  mqtt_id[sizeof(mqtt_id) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  strncpy(mqtt_username, username, sizeof(mqtt_username) - 1);
  mqtt_username[sizeof(mqtt_username) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  strncpy(mqtt_password, password, sizeof(mqtt_password) - 1);
  mqtt_password[sizeof(mqtt_password) - 1] = '\0';  // Asegurarse de que esté terminado con '\0'

  mqttClient.setServer(domain, port);
  mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);  // acota el bloqueo del socket en connect/reconnect
  if (mqttClient.connect(mqtt_id, mqtt_username, mqtt_password)) {
    DEBUG("Connection has been established, well done");
    if(callback_connect != NULL) callback_connect();
    subscribeRoutine();
    no_service_available = false;
  } else {
    DEBUG("Looks like the server connection failed...");
  }
}

bool MqttClient::isServiceAvailable() {
  return !no_service_available;
}

void MqttClient::reconnect() {
  static unsigned long lastReconnectAttempt = 0;
  unsigned long now = millis();
  static int reconnectAttempts = 0;

  if (!mqttClient.connected()) {
    if (now - lastReconnectAttempt > 120000 || reconnectAttempts == 0) { // 120000ms = 2 minutos
      lastReconnectAttempt = now;

      if (reconnectAttempts < 5) {
        mqttClient.flush();
        mqttClient.disconnect();
        mqttClient.setServer(mqtt_domain, mqtt_port);
        
        DEBUG("Attempting MQTT connection...");

        if (mqttClient.connect(mqtt_id, mqtt_username, mqtt_password)) {
          DEBUG("connected");
          subscribeRoutine();
          reconnectAttempts = 0; // Resetear los intentos si la conexión es exitosa
        } else {
          DEBUG(("failed, rc= " + String(mqttClient.state()) + ", try again in 2 minutes").c_str());
          reconnectAttempts++;
        }
      } else {
        DEBUG("Max reconnect attempts reached, try again in 2 minutes");
        reconnectAttempts = 0; // Resetear los intentos después de alcanzar el máximo
      }
    }
  } else {
    reconnectAttempts = 0; // Resetear los intentos si ya está conectado
  }
}

void MqttClient::onConnect(std::function<void()> callback) {
  callback_connect = callback;
}

// `a` es el tópico COMPLETO que llegó del broker; `b` es uno de los sufijos #define.
// Se le antepone el prefijo del cuarto a `b` antes de comparar.
bool MqttClient::isTopicEqual(const char* a, const char* b){
  if (a == nullptr || b == nullptr) return false;
  return strcmp(a, fullTopic(b)) == 0;
}

bool MqttClient::isConnected() {
  return mqttClient.connected();
}

void MqttClient::loop() {
  if (!isServiceAvailable()) return;
  if (!mqttClient.connected()) reconnect();
  
  delay(100);
  // vTaskDelay(100 / portTICK_PERIOD_MS);
  mqttClient.loop();
}

void MqttClient::setCallback(std::function<void(char *, uint8_t *, unsigned int)> callback) {
  mqttClient.setCallback(callback);
}

void MqttClient::subscribeRoutine() {
  if (mqttClient.connect(mqtt_username)) {
    DEBUG("connected, subscribing");
    
    for (int i = 0; i < SUB_ARRAY_SIZE; i++) mqttClient.subscribe(fullTopic(topics[i]));

    DEBUG("subscribing done");
  } else DEBUG("not connected, subscribing aborted");
}

// `topic` es un SUFIJO (los #define de MqttClient.h): el prefijo del cuarto se antepone acá.
void MqttClient::publishData(String topic, double value) {
  if (WiFi.status() != WL_CONNECTED) return;
  char value_buffer[32];
  snprintf(value_buffer, sizeof(value_buffer), "%.2f", value);
  mqttClient.publish(fullTopic(topic.c_str()), value_buffer);
}

void MqttClient::publishData(String topic, String value) {
  if (WiFi.status() != WL_CONNECTED) return;
  // fullTopic() usa el buffer compartido, así que se resuelve antes de tocar `value`.
  const char* full = fullTopic(topic.c_str());
  mqttClient.publish(full, value.c_str());
}

bool MqttClient::refreshMQTTStatus() {
  const bool connection = isConnected();
  if (connection != last_connection_state) {
    last_connection_state = connection;
    return true;
  }
  return false;
}

bool MqttClient::getConnectionStatus() {
  return last_connection_state;
}

float MqttClient::responseToFloat(byte *value, size_t len) {
  String string_builder;
  for (int i = 0; i < len; i++) string_builder += (char)value[i];
  return string_builder.toFloat();
}

int MqttClient::responseToInt(byte *value, size_t len) {
  String string_builder;
  for (int i = 0; i < len; i++) string_builder += (char)value[i];
  return string_builder.toInt();
}

void MqttClient::DEBUG(const char *message){
  LOG_D(TAG_MQTT, "%s", message);
}

void MqttClient::ERROR(ErrorType error){
  LOG_E(TAG_MQTT, "%s", errorMessages[error].c_str());
}