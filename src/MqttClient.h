#ifndef MY_MQTT_H
#define MY_MQTT_H
#include <Arduino.h>
#include <PubSubClient.h>
#include "./hardware/Logger.h" 
#include <WiFiClientSecure.h>

#define MQTT_USERNAME_SIZE 32
#define MQTT_ID_SIZE 32
#define MQTT_PASSWORD_SIZE 32
#define PREFIX_SIZE 32

#define main_topic            "cfpp4/"

//             subscribe topics    -------------------------------------------------------------------->
#define sub_hours           main_topic "hours"
#define sub_minutes         main_topic "minutes"
#define sub_day             main_topic "day"
#define sub_month           main_topic "month"
#define sub_f1_st1_ontime   main_topic "f1_st1_ontime"
#define sub_f1_st1_offtime  main_topic "f1_st1_offtime"
#define sub_f1_rev_st1_ontime  main_topic "f1_rev_st1_ontime"
#define sub_f1_st2_ontime   main_topic "f1_st2_ontime"
#define sub_f1_st2_offtime  main_topic "f1_st2_offtime"
#define sub_f1_rev_st2_ontime  main_topic "f1_rev_st2_ontime"
#define sub_s1_st2_ontime   main_topic "s1_st2_ontime"
#define sub_s1_st2_offtime  main_topic "s1_st2_offtime"
#define sub_f1_st3_ontime   main_topic "f1_st3_ontime"
#define sub_f1_st3_offtime  main_topic "f1_st3_offtime"
#define sub_f1_rev_st3_ontime  main_topic "f1_rev_st3_ontime"
#define sub_s1_st3_ontime   main_topic "s1_st3_ontime"
#define sub_s1_st3_offtime  main_topic "s1_st3_offtime"
#define sub_A               main_topic "A"
#define sub_B               main_topic "B"
#define sub_ts_set          main_topic "ts_set"
#define sub_tc_set          main_topic "tc_set"
#define sub_start           main_topic "start"
#define sub_d_start         main_topic "d_start"
#define sub_stop            main_topic "stop"
#define sub_TempAcqDelay    main_topic "TempAcqDelay"
#define sub_P               main_topic "P"
#define sub_I               main_topic "I"
#define sub_D               main_topic "D"
#define sub_avgTiming       main_topic "TsAvgTime"      // in ms the sampling rate for Ts calculation
#define sub_tsAvgSpan       main_topic "TsAvgFifoSpan"  // in minutes the span of the fifo for Ts calculation
#define sub_chooseTs        main_topic "chooseTs"
#define sub_coefPID         main_topic "coefPID"
#define sub_coefPIDFwd      main_topic "coefPIDFwd"
#define sub_coefPIDRev      main_topic "coefPIDRev"
#define LORA_TC             main_topic "lora_TC"
#define IS_TC_LORA          main_topic "isTcLora"
#define IS_TS_IR            main_topic "isTsIR"


#define SUB_ARRAY_SIZE 37

//------------ publish index    -------------------------------------------------------------------->
#define m_F1                main_topic "M_F1"
#define m_F1_CCW            main_topic "M_F1_CCW"
#define m_F2                main_topic "M_F2"
#define m_S1                main_topic "M_S1"
#define STAGE               main_topic "stage"
#define AVG_TS_TOPIC        main_topic "AvgTs"
#define AVG_TC_TOPIC        main_topic "AvgTc"
#define AVG_TA_TOPIC        main_topic "AvgTa"
#define TA_TOPIC            main_topic "Ta"
#define TS_TOPIC            main_topic "Ts"
#define TC_TOPIC            main_topic "Tc"
#define TI_TOPIC            main_topic "Ti"
#define TS_PT100_TOPIC      main_topic "Ts_PT100"
#define TS_IR_MLX_TOPIC     main_topic "Ts_IR_MLX"
#define PID_OUTPUT          main_topic "PID_output"
#define SETPOINT            main_topic "setpoint"
#define ACK_F1_ST1_ONTIME   main_topic "ack_f1_st1_ontime"
#define ACK_F1_ST1_OFFTIME  main_topic "ack_f1_st1_offtime"
#define ACK_F1_ST2_ONTIME   main_topic "ack_f1_st2_ontime"
#define ACK_F1_ST2_OFFTIME  main_topic "ack_f1_st2_offtime"
#define ACK_S1_ST2_ONTIME   main_topic "ack_s1_st2_ontime"
#define ACK_S1_ST2_OFFTIME  main_topic "ack_s1_st2_offtime"
#define ACK_F1_ST3_ONTIME   main_topic "ack_f1_st3_ontime"
#define ACK_F1_ST3_OFFTIME  main_topic "ack_f1_st3_offtime"
#define ACK_S1_ST3_ONTIME   main_topic "ack_s1_st3_ontime"
#define ACK_S1_ST3_OFFTIME  main_topic "ack_s1_st3_offtime"
#define ACK_A               main_topic "ack_A"
#define ACK_B               main_topic "ack_B"
#define ACK_TS              main_topic "ack_Ts"
#define ACK_TC              main_topic "ack_Tc"
#define SPOILED_SENSOR      main_topic "spoiled_sensor"
#define IR_TS               main_topic "IR_TS"
#define REMAINING_D_START   main_topic "REMAINING_DELAY_START"

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
    std::function<void ()> callback_connect = NULL;
    void ERROR (ErrorType error);

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
    };

};
#endif
