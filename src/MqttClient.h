#ifndef MY_MQTT_H
#define MY_MQTT_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "./hardware/Logger.h"
#include <WiFiClientSecure.h>

#define MQTT_USERNAME_SIZE 32
#define MQTT_ID_SIZE 32
#define MQTT_PASSWORD_SIZE 32
#define PREFIX_SIZE 32
#define TOPIC_SUFFIX_MAX 32   // el sufijo más largo hoy es "REMAINING_DELAY_START" (21)

// Prefijo por defecto si config.txt no trae la clave "TOPIC". Se mantiene "cfpp4/" para que
// un equipo ya desplegado no cambie de tópicos al actualizar el firmware.
#define DEFAULT_TOPIC_PREFIX  "cfpp4/"

// ----------------------------------------------------------------------------------------
// Los defines de abajo son SUFIJOS, no tópicos completos: la planta tiene 9 cuartos y cada
// uno necesita su propio espacio de nombres. El prefijo se lee de config.txt en runtime y
// MqttClient lo antepone al publicar, suscribir y comparar. Antes iban concatenados contra
// un #define, lo que obligaba a compilar un binario distinto por cuarto.
// ----------------------------------------------------------------------------------------

//             subscribe topics    -------------------------------------------------------------------->
#define sub_hours               "hours"
#define sub_minutes             "minutes"
#define sub_day                 "day"
#define sub_month               "month"
#define sub_f1_st1_ontime       "f1_st1_ontime"
#define sub_f1_st1_offtime      "f1_st1_offtime"
#define sub_f1_rev_st1_ontime   "f1_rev_st1_ontime"
#define sub_f1_st2_ontime       "f1_st2_ontime"
#define sub_f1_st2_offtime      "f1_st2_offtime"
#define sub_f1_rev_st2_ontime   "f1_rev_st2_ontime"
#define sub_s1_st2_ontime       "s1_st2_ontime"
#define sub_s1_st2_offtime      "s1_st2_offtime"
#define sub_f1_st3_ontime       "f1_st3_ontime"
#define sub_f1_st3_offtime      "f1_st3_offtime"
#define sub_f1_rev_st3_ontime   "f1_rev_st3_ontime"
#define sub_s1_st3_ontime       "s1_st3_ontime"
#define sub_s1_st3_offtime      "s1_st3_offtime"
#define sub_A                   "A"
#define sub_B                   "B"
#define sub_ts_set              "ts_set"
#define sub_tc_set              "tc_set"
#define sub_start               "start"
#define sub_d_start             "d_start"
#define sub_stop                "stop"
#define sub_TempAcqDelay        "TempAcqDelay"
#define sub_P                   "P"
#define sub_I                   "I"
#define sub_D                   "D"
#define sub_avgTiming           "TsAvgTime"      // in ms the sampling rate for Ts calculation
#define sub_tsAvgSpan           "TsAvgFifoSpan"  // in minutes the span of the fifo for Ts calculation
#define sub_chooseTs            "chooseTs"
#define sub_coefPID             "coefPID"
#define sub_coefPIDFwd          "coefPIDFwd"
#define sub_coefPIDRev          "coefPIDRev"
#define LORA_TC                 "lora_TC"
#define IS_TC_LORA              "isTcLora"
#define IS_TS_IR                "isTsIR"
#define sub_maintenance         "maintenance"   // 1 = activar bloqueo por mantenimiento, 0 = levantarlo


#define SUB_ARRAY_SIZE 38

//------------ publish index    -------------------------------------------------------------------->
#define m_F1                    "M_F1"
#define m_F1_CCW                "M_F1_CCW"
#define m_F2                    "M_F2"
#define m_S1                    "M_S1"
#define STAGE                   "stage"
#define AVG_TS_TOPIC            "AvgTs"
#define AVG_TC_TOPIC            "AvgTc"
#define AVG_TA_TOPIC            "AvgTa"
#define TA_TOPIC                "Ta"
#define TS_TOPIC                "Ts"
#define TC_TOPIC                "Tc"
#define TI_TOPIC                "Ti"
#define SENSOR_FAULT            "sensorFault"   // {"sensor":..,"fault":bool,"temp":°C,"raw":cuentas,..}
// Diagnóstico crudo del ADC, en CUENTAS (no °C). Con la cadena 4-20 mA -> AIA03 -> ADC,
// un lazo sano vive entre 819 (4 mA) y 4095 (20 mA); por debajo de 819 el lazo está abierto.
#define ADC_RAW                 "adc_raw"       // {"ta":{"raw":..,"med":..,"fails":..},..}
#define TS_PT100_TOPIC          "Ts_PT100"
#define TS_IR_MLX_TOPIC         "Ts_IR_MLX"
#define PID_OUTPUT              "PID_output"
#define SETPOINT                "setpoint"
#define ACK_F1_ST1_ONTIME       "ack_f1_st1_ontime"
#define ACK_F1_ST1_OFFTIME      "ack_f1_st1_offtime"
#define ACK_F1_ST2_ONTIME       "ack_f1_st2_ontime"
#define ACK_F1_ST2_OFFTIME      "ack_f1_st2_offtime"
#define ACK_S1_ST2_ONTIME       "ack_s1_st2_ontime"
#define ACK_S1_ST2_OFFTIME      "ack_s1_st2_offtime"
#define ACK_F1_ST3_ONTIME       "ack_f1_st3_ontime"
#define ACK_F1_ST3_OFFTIME      "ack_f1_st3_offtime"
#define ACK_S1_ST3_ONTIME       "ack_s1_st3_ontime"
#define ACK_S1_ST3_OFFTIME      "ack_s1_st3_offtime"
#define ACK_A                   "ack_A"
#define ACK_B                   "ack_B"
#define ACK_TS                  "ack_Ts"
#define ACK_TC                  "ack_Tc"
#define SPOILED_SENSOR          "spoiled_sensor"
#define IR_TS                   "IR_TS"
#define REMAINING_D_START       "REMAINING_DELAY_START"
#define MAINTENANCE_STATE       "maintenance_state"   // 1/0: estado vigente del bloqueo
#define FAN_OVERRIDE_STATE      "fan_override"        // 1/0: fans prendidos a mano desde el selector del tablero

// ERROR MESSAGES
#define ON_CONNECTION_ERR_TXT "Error on connection"
#define ON_RECONNECT_ERR_TXT "Error on reconnect"
#define ON_SUBSCRIBE_ERR_TXT "Error on subscribe"

struct mqtt_event {
  const char* topic;
  std::function<void (char *, uint8_t *, unsigned int)> callback;
};

class MqttClient {
  public:
    void loop();
    // Prefijo de tópicos del cuarto (clave "TOPIC" de config.txt). Vacío o nulo => se usa
    // DEFAULT_TOPIC_PREFIX. Se le agrega '/' final si no lo trae, así "cfpp9" y "cfpp9/"
    // dan lo mismo y un dedazo en el portal no manda los mensajes a un espacio equivocado.
    // Debe llamarse ANTES de connect(), porque connect() ya suscribe.
    void setTopicPrefix(const char* prefix);
    const char* getTopicPrefix() const { return topic_prefix; }
    // Habilita TLS (server-auth). Si ca_override viene vacío usa el CA embebido
    // en mqtt_certs.h; si trae PEM, ese tiene prioridad. Debe llamarse ANTES de connect().
    void setTLS(bool enabled, const String& ca_override = String());
    bool isTLS();
    void connect(const char *domain, uint16_t port, const char *id, const char *username, const char *password);
    void reconnect();
    bool isConnected();
    void subscribeRoutine();
    bool refreshMQTTStatus();
    bool isServiceAvailable();
    bool getConnectionStatus();
    int responseToInt(byte *value, size_t len);
    void publishData(String topic, double value);
    void publishData(String topic, String value);
    float responseToFloat(byte *value, size_t len);
    bool isTopicEqual(const char* a, const char* b);
    void onConnect(std::function<void ()> callback);
    mqtt_event searchEventByTopic(const char* topic);
    void setCallback(std::function<void (char *, uint8_t *, unsigned int)> callback);
    void createMqttEvent(const char* topic, std::function<void (char *, uint8_t *, unsigned int)> callback);
    // void publishEcava(const String* topics, const String* values, int arraySize, const char* mqttTopic);
    // String getIsoTimestamp();
    // void exampleCall();
    void DEBUG(const char *message);
  private:
    enum ErrorType { 
      ERROR_ON_CONNECTION,
      ERROR_ON_RECONNECT,
      ERROR_ON_SUBSCRIBE,
      NUM_ERRORS 
    };
    
    const String errorMessages[NUM_ERRORS] = {ON_CONNECTION_ERR_TXT, ON_RECONNECT_ERR_TXT, ON_SUBSCRIBE_ERR_TXT};

    uint16_t mqtt_port;
    char mqtt_id[MQTT_USERNAME_SIZE];
    char mqtt_username[MQTT_USERNAME_SIZE];  
    char mqtt_password[MQTT_USERNAME_SIZE];
    char mqtt_domain[MQTT_USERNAME_SIZE];
    bool no_service_available = true;
    bool last_connection_state = false;
    bool use_tls = false;
    String ca_cert;  // se mantiene vivo: WiFiClientSecure::setCACert guarda el puntero
    std::function<void ()> callback_connect = NULL;
    void ERROR (ErrorType error);

    char topic_prefix[PREFIX_SIZE] = DEFAULT_TOPIC_PREFIX;
    // Buffer de armado para prefijo+sufijo. Se reusa en cada llamada para no tocar el heap
    // (se publica cada 3 s desde el loop de control). No es reentrante, y no hace falta:
    // publicar, suscribir y comparar tópicos ocurre siempre desde la task del loop (core 1),
    // nunca desde backgroundTasks ni desde los handlers del servidor web.
    char topic_buf[PREFIX_SIZE + TOPIC_SUFFIX_MAX];
    const char* fullTopic(const char* suffix);

    // map list of suscribed topics
    const char* topics[SUB_ARRAY_SIZE] = {
      sub_hours,
      sub_minutes,
      sub_day,
      sub_month,
      sub_f1_st1_ontime,
      sub_f1_st1_offtime,
      sub_f1_rev_st1_ontime,
      sub_f1_st2_ontime,
      sub_f1_st2_offtime,
      sub_f1_rev_st2_ontime,
      sub_s1_st2_ontime,
      sub_s1_st2_offtime,
      sub_f1_st3_ontime,
      sub_f1_st3_offtime,
      sub_f1_rev_st3_ontime,
      sub_s1_st3_ontime,
      sub_s1_st3_offtime,
      sub_A,
      sub_B,
      sub_ts_set,
      sub_tc_set,
      sub_start,
      sub_d_start,
      sub_stop,
      sub_TempAcqDelay,
      sub_P,
      sub_I,
      sub_D,
      sub_avgTiming,
      sub_tsAvgSpan,
      sub_chooseTs,
      sub_coefPID,
      sub_coefPIDFwd,
      sub_coefPIDRev,
      LORA_TC,
      IS_TC_LORA,
      IS_TS_IR,
      sub_maintenance,
    };

};
#endif
