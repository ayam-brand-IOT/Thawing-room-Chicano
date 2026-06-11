#include "Thawing-room-chicano.h"

// Stage parameters
stage_parameters stage1_params;
stage_parameters stage2_params;  // fan (F1) and sprinklers (S1) STAGE 2 on and off time
stage_parameters stage3_params;

// Stages
Stage stage_1(2, initStage1, destroyStage1);
Stage stage_2(2, initStage2, destroyStage2);
Stage stage_3(2, initStage3, destroyStage3);

Button start_btn(START_IO); // Connect your button between pin 2 and GND
Button stop_btn(STOP_IO); // Connect your button between pin 3 and GND
Button d_start_button(DLY_S_IO); // Connect your button between pin 4 and GND

// A & B variables
room_parameters room;

// Temperatures measures of Ta, Ts, Tc, Ti & AvgTs
data_s temp_data;

// Ts & Tc target value
data_tset temp_set;

// PID parameters
float pid_output, pid_setpoint;           // value of the PID output
double Kp = 0, Ki = 10, Kd = 0;
double Output, pid_input, Setpoint;

bool kp_has_changed = true;
bool ki_has_changed = true;
bool kd_has_changed = true;

double coef_output = 0;  // Output for the infeed (New Analog Output that will be sent to S1
uint8_t Converted_Output = 0;

bool is_rts_ir = 0;

// Parameters of Stage 2
uint8_t stage2_hour = 0;
uint8_t stage2_minute = 0;
uint8_t stage2_day = 0;
uint8_t stage2_month = 0;

// ########################### Timers ##########################
SystemTimers timers = {0};  // Consolidated timer structure

// ######################## Temperature ########################
float TA = 0, TA_F = 0;  //Ta
float TS = 0, TS_F = 0;  //Ts
float TC = 0, TC_F = 0;  //Tc
float TI = 0, TI_F = 0;  //Ti optional

// ########################### Buffer ##########################
SensorBuffer sensorTs(BUFFER_SIZE);  // Crear una instancia para el sensor Ts
SensorBuffer sensorTc(10);  // Crear otra instancia para el sensor Tc
SensorBuffer sensorTa(10);  // Crear otra instancia para el sensor Ta

SensorBuffer sensorTsPT100(10);  // Crear otra instancia para el sensor TsPT100

// SystemState currentState = IDLE;
StageState currentState = {IDLE, 0};

// ########################## TASKS ##########################
Task low_priority_msgs(10000, TASK_FOREVER, &aknowledgementRoutine);
Task high_priority_msgs(3000, TASK_FOREVER, &publishTemperatures);
Task turn_off_flush(20, TASK_ONCE, &turn_off_flush_routine);
Task turn_on_flush(20, TASK_ONCE, &turn_on_flush_routine);
Task ntp_sync_task(24UL * 60UL * 60UL * 1000UL, TASK_FOREVER, &ntp_sync_callback);

Scheduler runner;

MqttClient mqtt;
Controller controller;

void ntp_sync_callback() { controller.forceNTPSync(); }
TaskHandle_t communicationTask;
PID air_in_feed_PID(&pid_input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);  // DIRECT or REVERSE

void printStackUsage() {
  UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);
  char buffer[50];
  sprintf(buffer, "Stack high water mark: %lu", (unsigned long)highWaterMark);
  logger.println(buffer);
}

// ---- Log the cause of the previous reset (called after logger is ready) ----
void logResetReason() {
  const esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:  logger.println(F("[BOOT] Power-on reset"));            break;
    case ESP_RST_SW:       logger.println(F("[BOOT] Software restart"));           break;
    case ESP_RST_PANIC:    logger.println(F("[CRITICAL] Reset by panic / crash")); break;
    case ESP_RST_INT_WDT:  logger.println(F("[CRITICAL] Reset by interrupt WDT")); break;
    case ESP_RST_TASK_WDT: logger.println(F("[CRITICAL] Reset by task WDT"));     break;
    case ESP_RST_WDT:      logger.println(F("[CRITICAL] Reset by WDT (other)"));   break;
    case ESP_RST_BROWNOUT: logger.println(F("[CRITICAL] Reset by brownout"));      break;
    default:               logger.println(F("[BOOT] Reset reason: unknown"));      break;
  }
}

void backgroundTasks(void* pvParameters) {
  for (;;) {
    if (controller.isAPMode()) {
      // En modo portal: atender el DNS cautivo. No intentar reconectar STA.
      controller.loopAP();
    } else {
      controller.WiFiLoop();
      if(controller.isWiFiConnected()) {
        controller.loopOTA();
      }
    }
    // printStackUsage(); // Monitorea el uso de la pila
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void setup() {
  controller.init();

  // Defaults por si no hay config.txt: el equipo igual arranca y levanta su portal AP.
  char SSID[SSID_SIZE] = "";
  char PASS[PASSWORD_SIZE] = "";
  char HOST_NAME[HOSTNAME_SIZE] = "thawingroom";
  char IP_ADDRESS[IP_ADDRESS_SIZE] = "";
  char STATIC_IP[IP_ADDRESS_SIZE] = "";
  uint16_t PORT = 1883;
  char MQTT_ID[MQTT_ID_SIZE] = "";
  char USERNAME[MQTT_USERNAME_SIZE] = "";
  char MQTT_PASSWORD[MQTT_PASSWORD_SIZE] = "";
  char PREFIX_TOPIC[PREFIX_SIZE] = "";

  const bool config_ok = controller.runConfigFile(SSID, PASS, HOST_NAME, IP_ADDRESS, &PORT, MQTT_ID, USERNAME, MQTT_PASSWORD, PREFIX_TOPIC, STATIC_IP);
  controller.setUpDefaultParameters(stage1_params, stage2_params, stage3_params, room, temp_set);

  start_btn.begin();
  stop_btn.begin();
  d_start_button.begin();

  controller.setUpWiFi(SSID, PASS, HOST_NAME);

  if (!config_ok) {
    // Sin config.txt no hay credenciales que probar: portal de configuración directo.
    // startConfigPortal levanta el AP y registra el servidor web.
    logger.println(F("No config - starting AP config portal"));
    controller.startConfigPortal();
  } else {
    const bool connected = controller.connectToWiFi(/* web_server */ true, /* web_serial */ true, /* OTA */ true);
    if (!connected) {
      // 3 reinicios sin lograr WiFi -> portal AP. El control de la sala sigue operando.
      logger.println(F("WiFi failed on boot - starting AP config portal"));
      controller.startConfigPortal();
    }
  }
  controller.setUpRTC();

  runner.init();
  runner.addTask(low_priority_msgs);
  runner.addTask(high_priority_msgs);
  runner.addTask(turn_on_flush);
  runner.addTask(turn_off_flush);
  runner.addTask(ntp_sync_task);
  low_priority_msgs.enable();
  high_priority_msgs.enable();
  ntp_sync_task.enable();

  if (false /* flush active */) {
    turn_on_flush.enable();
  }

  DateTime current_date = controller.getDateTime();
  char dateBuffer[60];
  sprintf(dateBuffer, "Current date: %d:%d %d/%d", current_date.hour(), current_date.minute(), current_date.day(), current_date.month());
  logger.println(dateBuffer);

  StageState last_state = controller.getLastState();
  char stateBuffer[50];
  sprintf(stateBuffer, "Last state: %d %d", last_state.stage, last_state.step);
  logger.println(stateBuffer);
  if (last_state.stage != IDLE) currentState = last_state;

  mqtt.setCallback(callback);
  mqtt.onConnect(onMQTTConnect);
  mqtt.connect(IP_ADDRESS, PORT, MQTT_ID, USERNAME, MQTT_PASSWORD);

  xTaskCreatePinnedToCore(backgroundTasks, "communicationTask", 20000, NULL, 1, &communicationTask, 0);

  logResetReason();  // log why we (re)started — written after logger is ready
  logger.println(F("===========> Reboted!! <==========="));

  //Turn the PID on
  air_in_feed_PID.SetMode(AUTOMATIC);
  air_in_feed_PID.SetSampleTime(3000);
  air_in_feed_PID.SetTunings(Kp, Ki, Kd);

  delay(750);
}

void loop() {
  runner.execute();
  // if is for testing porpuse comment this "if" and replace DateTime "now" for: DateTime now(__DATE__, __TIME__); 
  DateTime current_date = controller.getDateTime();

  // if (!controller.isRTCConnected()) {  
  //   logger.println("RTC not connected"); 
  //   while (true) delay(1000);
  // }

  if(controller.isWiFiConnected()) {
    mqtt.loop();
  }

  updateTemperature();

  // if ((TA) > (LOW_TEMP_LIMIT) && (TA) < (HIGH_TEMP_LIMIT))
   TA_F = TA;  // if the temperature over the limit it will not be considered
  
  // if ((TS) > (LOW_TEMP_LIMIT) && (TS) < (HIGH_TEMP_LIMIT))
   TS_F = TS;
  
  // if ((TC) > (LOW_TEMP_LIMIT) && (TC) < (HIGH_TEMP_LIMIT))
   TC_F = TC;
  
  // if ((TI) > (LOW_TEMP_LIMIT) && (TI) < (HIGH_TEMP_LIMIT))
   TI_F = TI;

  handleInputs();

  handleStage();

  // if (hasIntervalPassed(get_temp_timer, TIME_ACQ_DELAY)) publishTemperatures();  
  
}


bool handleInputs(button_type override) {
  button_type button = NONE;
  bool pressed_btn = false;

  if (override) pressed_btn = button = override;

  if      (d_start_button.released()) button = D_START;
  else if (start_btn.released())      button = START;
  else if (stop_btn.released())       button = STOP;

  if      (button == D_START)         setStage(STAGE1);
  else if (button == START)           setStage(STAGE2);
  else if (button == STOP)            stopRoutine(); 

  return !pressed_btn;
}

void handleStage(){
  // logger.print("Current State: "+ String(currentState) + " ");
  if      (currentState.stage == STAGE1)  handleStage1();
  else if (currentState.stage == STAGE2)  handleStage2();
  else if (currentState.stage == STAGE3)  handleStage3();

  else if (currentState.stage == IDLE)    idle();
  else if (currentState.stage == ERROR)   stopRoutine();
}

void handleStage1(){
  // Tras power-off: RTC_DATA_ATTR se borró → intentar recuperar desde flash,
  // y solo si tampoco está en flash, recalcular con los parámetros actuales.
  if (stage_1.getCurrentStep() > 0 && !controller.isStage2TimeSet()) {
    controller.loadStage2StartTime();
    if (!controller.isStage2TimeSet()) {
      logger.println("Stage2 epoch lost - recalculating from current params");
      controller.saveStage2StartTime(stage2_hour, stage2_minute, stage2_day, stage2_month);
    }
  }

  asyncLoopSprinkler(timers.stage1.sprinkler, stage1_params.sprinklerOffTime, stage1_params.sprinklerOnTime);

  // Init stage 1
  if (stage_1.getCurrentStep() == 0) {
    stage_1.init();
    logger.print("STAGE #1, current step: ");
    // Calcular y persistir el epoch de transición Stage1->Stage2
    controller.saveStage2StartTime(stage2_hour, stage2_minute, stage2_day, stage2_month);
    stage_1.nextStep();
  }
  
  // Step #1
  else if (stage_1.getCurrentStep() == 1){
    if (!controller.getFanState()){
      hasIntervalPassed(timers.stage1.fan, stage1_params.fanOnTime, true); 

      controller.turnOnFan(true);                                                                                     // Turn ON F1
      logger.println(F("Stage 1 F1 On"));

      publishStateChange(m_F1, true, "Stage 1 init M_F1 ON published ");
    }

    else if (hasIntervalPassed(timers.stage1.fan, stage1_params.fanOnTime , true)) {
      logger.print("STAGE #1, current step: ");
      stage_1.nextStep();
    }
     
  }

  // Step #2
  else if (stage_1.getCurrentStep() == 2 ){
    // Turn OFF F1 when the time set in the configuration is over
    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage1.fan, stage1_params.fanOffTime , true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(false);
      logger.println("Stage 1 F1 Off");

      publishStateChange(m_F1, false, "Stage 1 init M_F1 OFF published ");
    }
    else if (hasIntervalPassed(timers.stage1.fan, stage1_params.fanOffTime, true)) {
      logger.print("STAGE #1, current step: ");
      stage_1.nextStep();
    }
  }

  // Step #3
  else if (stage_1.getCurrentStep() == 3){
    if (!controller.getFanState()){
      hasIntervalPassed(timers.stage1.fan, stage1_params.fanRevONTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(true, true); 
      logger.println("Stage 1 F1 On");

      publishStateChange(m_F1, true, "Stage 1 init M_F1 ON published ");
    }

    else if (hasIntervalPassed(timers.stage1.fan, stage1_params.fanRevONTime , true)) {
      logger.print("STAGE #1, current step: ");
      stage_1.nextStep();
    }
  }

  // Step #4
  else if (stage_1.getCurrentStep() == 4){
    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage1.fan, stage1_params.fanOffTime , true); // In case that the time is over or that the stage and step are not updated
      
      controller.turnOnFan(false);
      logger.println("Stage 1 F1 Off");

      publishStateChange(m_F1, false, "Stage 1 init M_F1 OFF published ");
    }

    else if (hasIntervalPassed(timers.stage1.fan, stage1_params.fanOffTime, true)) stage_1.setStep(1);
  }

  DateTime current_date = controller.getDateTime();
  
  if (controller.hasStage2TimeElapsed(current_date)) {
    const long overdue = -controller.getRemainingMinutesToStage2(current_date);
    char buf[60];
    snprintf(buf, sizeof(buf), "Stage2 time reached (overdue: %ld min)", overdue);
    logger.println(buf);
    stage_1.destroy();
    setStage(STAGE2);
  }
  //  Stage 1 Process 
}

void handleStage2(){
  // Loop of async process
  asyncLoopSprinkler(timers.stage2.sprinkler, stage2_params.sprinklerOffTime, stage2_params.sprinklerOnTime);

  // Init stage 2
  if (stage_2.getCurrentStep() == 0) {
    stage_2.init(); 
    logger.print("STAGE #2, current step: ");
    stage_2.nextStep();
    timers.stage2.init_delay = millis();  // start non-blocking 5s guard
  }

  // Non-blocking init guard: hold 5s before starting actuators
  // During this window the loop keeps running: STOP button, MQTT, temps all work
  else if (stage_2.getCurrentStep() == 1 && (millis() - timers.stage2.init_delay) < 5000) {
    return;
  }

  // Step #1
  else if (stage_2.getCurrentStep() == 1){
    if (!controller.getFanState()) {
      hasIntervalPassed(timers.stage2.fan, stage2_params.fanOnTime, true);
      controller.turnOnFan(true);// Output of F1
      controller.writeDigitalOutput(AIR_DAMPER_IO, HIGH);  // Air damper
      logger.println("Stage 2 F1 On");

      publishStateChange(m_F1, true, "Stage 2 F1 Start published ");
    }


    // Calculate the Setpoint every 3 seconds in Function of Ta with the formula : Setpoint = B - A*Ts
    if (hasIntervalPassed(timers.stage2.pid_computing, 3000)) {
      Setpoint = (-(room.A * (temp_data.avg_ts)) + room.B);  //use the average of the temperature over the x last minuites
      pid_setpoint = float(Setpoint);

      char buffer[40];
      sprintf(buffer, "New Setpoint: %.2f", pid_setpoint);
      logger.println(buffer);


      publishStateChange(SETPOINT, pid_setpoint, "Setpoint published ");
    }

    // Activate the PID when F1 ON
    if (controller.getFanState() && hasIntervalPassed(timers.stage2.pid_turn_on, 3000)) {
      pid_input = TA_F;
      // coef_output = Output;  // Transform the Output of the PID to the desired max value
      coef_output = (room.coef_pid_fwd * Output) / 100;  // Transform the Output of the PID to the desired max value

      air_in_feed_PID.Compute();
      char buffer[40];
      sprintf(buffer, "Computing PID: %.2f", coef_output);
      logger.println(buffer);
      
      controller.writeAnalogOutput(AIR_PWM, coef_output);
      publishPID();
    }

    // Turn OFF F1 when time is over
    if (controller.getFanState() && hasIntervalPassed(timers.stage2.fan, stage2_params.fanOnTime, true)) stage_2.nextStep();
  }

  // Step #2
  else if (stage_2.getCurrentStep() == 2 ){
    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage2.fan, stage2_params.fanOffTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(false);
      controller.writeDigitalOutput(AIR_DAMPER_IO, LOW);  // Air damper

      logger.println("Stage 2 F1 Off");
      publishStateChange(m_F1, false, "Stage 2 F1 Stop published ");
    }

    // Put the PID at 0 when F1 OFF
    if (!controller.getFanState() && hasIntervalPassed(timers.stage2.pid_turn_off, 3000)) {
      //Setpoint = 0;
      pid_input = 0;
      Output = 0;
      coef_output = 0;
      
      controller.writeAnalogOutput(AIR_PWM, Output);
      publishPID();
    }

    if (!controller.getFanState() && hasIntervalPassed(timers.stage2.fan, stage2_params.fanOffTime, true)) stage_2.nextStep();
  }

  //Step #3
  else if (stage_2.getCurrentStep() == 3){
     if (!controller.getFanState()){
      hasIntervalPassed(timers.stage2.fan, stage2_params.fanRevONTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(true, true);
      logger.println("Stage 2 F1 On");

      publishStateChange(m_F1, true, "Stage 2 F1 Start published ");
    }

    // Calculate the Setpoint every 3 seconds in Function of Ta with the formula : Setpoint = B - A*Ts
    if (hasIntervalPassed(timers.stage2.pid_computing, 3000)) {
      Setpoint = (-(room.A * (temp_data.avg_ts)) + room.B);  //use the average of the temperature over the x last minuites
      pid_setpoint = float(Setpoint);

      char buffer[40];
      sprintf(buffer, "New Setpoint: %.2f", pid_setpoint);
      logger.println(buffer);


      publishStateChange(SETPOINT, pid_setpoint, "Setpoint published ");
    }

    // Activate the PID when F1 ON (reverse)
    if (controller.getFanState() && hasIntervalPassed(timers.stage2.pid_turn_on, 3000)) {
      pid_input = TA_F;
      coef_output = (room.coef_pid_rev * Output) / 100;  // Transform the Output of the PID to the desired max value

      air_in_feed_PID.Compute();
      char buffer[50];
      sprintf(buffer, "Computing PID (reverse): %.2f", coef_output);
      logger.println(buffer);
      
      controller.writeAnalogOutput(AIR_PWM, coef_output);
      publishPID();
    }

    if (controller.getFanState() && hasIntervalPassed(timers.stage2.fan, stage2_params.fanRevONTime, true)) stage_2.nextStep();
  }

  //Step #4
  else if (stage_2.getCurrentStep() == 4){
    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage2.fan, stage2_params.fanOffTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(false);
      // controller.writeDigitalOutput(AIR_DAMPER_IO, HIGH);  // Air damper
      logger.println("Stage 2 F1 Off");

      publishStateChange(m_F1, false, "Stage 2 F1 Stop published ");
    }

    // Ensure PID output is cleared when the fan stops in step 4
    if (!controller.getFanState() && hasIntervalPassed(timers.stage2.pid_turn_off, 3000)) {
      pid_input = 0;
      Output = 0;
      coef_output = 0;

      controller.writeAnalogOutput(AIR_PWM, 0);
      publishPID();
    }

    if (!controller.getFanState() && hasIntervalPassed(timers.stage2.fan, stage2_params.fanOffTime, true)) stage_2.setStep(1);
  }

  bool isReadyForStage3 = TS_F >= temp_set.ts && TC_F >= temp_set.tc;

  if (isReadyForStage3 ){
    stage_2.destroy();
    setStage(STAGE3);
  } 
  //  Stage 2 Process
}

void handleStage3(){
  // Loop of async process
  asyncLoopSprinkler(timers.stage3.sprinkler, stage3_params.sprinklerOffTime, stage3_params.sprinklerOnTime);

  // Init stage 3
  if (stage_3.getCurrentStep() == 0) {
    stage_3.init();
    logger.print("STAGE #3, current step: ");
    stage_3.nextStep();
    timers.stage3.init_delay = millis();  // start non-blocking 5s guard
  }

  // Non-blocking init guard: hold 5s before starting actuators
  else if (stage_3.getCurrentStep() == 1 && (millis() - timers.stage3.init_delay) < 5000) {
    return;
  }

  // Step #1
  else if (stage_3.getCurrentStep() == 1){

    if (!controller.getFanState()) {
      hasIntervalPassed(timers.stage3.fan, stage3_params.fanOnTime, true);
      controller.turnOnFan(true);// Output of F1
      logger.println("Stage 3 F1 On");

      publishStateChange(m_F1, true, "Stage 3 F1 Start published ");
    }

    if(controller.getFanState() && hasIntervalPassed(timers.stage3.fan, stage3_params.fanOnTime, true)) stage_3.nextStep();
  }

  // Step #2
  else if (stage_3.getCurrentStep() == 2){
    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage3.fan, stage3_params.fanOffTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(false);
      controller.writeDigitalOutput(AIR_DAMPER_IO, LOW);  // Air damper
      logger.println("Stage 3 F1 Off");

      publishStateChange(m_F1, false, "Stage 3 F1 Stop published ");
    }

    if (!controller.getFanState() && hasIntervalPassed(timers.stage3.fan, stage3_params.fanOffTime, true)) stage_3.nextStep();
  }

  // Step #3
  else if (stage_3.getCurrentStep() == 3){

    if (!controller.getFanState()){
      hasIntervalPassed(timers.stage3.fan, stage3_params.fanRevONTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(true, true);// Output of F1
      logger.println("Stage 3 F1 On");

      publishStateChange(m_F1, true, "Stage 3 F1 Start published ");
    }

    if (controller.getFanState() && hasIntervalPassed(timers.stage3.fan, stage3_params.fanRevONTime, true)) stage_3.nextStep();
  }

  // Step #4
  else if (stage_3.getCurrentStep() == 4){

    if (controller.getFanState()) {
      hasIntervalPassed(timers.stage3.fan, stage3_params.fanOffTime, true); // In case that the time is over or that the stage and step are not updated

      controller.turnOnFan(false);
      // controller.writeDigitalOutput(AIR_DAMPER_IO, HIGH);  // Air damper
      logger.println("Stage 3 F1 Off");

      publishStateChange(m_F1, false, "Stage 3 F1 Stop published ");
    }

    if (!controller.getFanState() && hasIntervalPassed(timers.stage3.fan, stage3_params.fanOffTime, true)) stage_3.setStep(1);
  }

  bool finishedStage3 = false;

  if (finishedStage3){
    stage_3.destroy();
    stopRoutine();
  }
}

void idle(){
  // delay(1000);
}

//// fct Callback ==> RECEIVE MQTT MESSAGES ////////////////////////////////////////////////////////////////////
void callback(char *topic, byte *payload, unsigned int len) {
  char buffer[80];
  sprintf(buffer, "Message arrived [%s]", topic);
  logger.println(buffer);

   // STOP
  if (mqtt.isTopicEqual(topic, sub_stop) ) {
    const bool value = mqtt.responseToInt(payload, len);
    if (!value) return;

    handleInputs(STOP);
    logger.println(F("stop BUTTON PRESSED ON NODE RED"));
  }

  // Choose TS
  if (mqtt.isTopicEqual(topic, sub_chooseTs)) {
    is_rts_ir = mqtt.responseToInt(payload, len);
    char buf[30];
    sprintf(buf, "Ts is now IR: %d", is_rts_ir);
    logger.println(buf);
  }

  // LoRaTc
  if (mqtt.isTopicEqual(topic, LORA_TC)) {
    if (!controller.isLoraTc()) return;

    const float tc_raw = mqtt.responseToFloat(payload, len);
    TC = isValidTemperature(tc_raw, TC_MIN, TC_MAX, "TC") ? tc_raw : TC_DEF;

    char buf[20];
    sprintf(buf, "%.2f", TC);
    logger.println(buf);
  }

  if (mqtt.isTopicEqual(topic, IS_TC_LORA)) {
    controller.setLoraTc(mqtt.responseToInt(payload, len));
    
    char buf[30];
    sprintf(buf, "Lora TC is now: %d", controller.isLoraTc());
    logger.println(buf);
  }

  // IR_TS
  if (mqtt.isTopicEqual(topic, IS_TS_IR)) {
    controller.setTsContactLess(mqtt.responseToInt(payload, len));

    char buf[30];
    sprintf(buf, "Ts is now: %d", controller.isTsContactLess());
    logger.println(buf);
  }
  

  if (currentState.stage != IDLE) return;

      // Delayed start timing
  if (mqtt.isTopicEqual(topic, sub_hours)) {
    stage2_hour = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "Stage 2 Hours set to: %d", stage2_hour);
    logger.println(buffer);
  }

  if (mqtt.isTopicEqual(topic, sub_minutes)) {
    stage2_minute = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "Stage 2 Minutes set to: %d", stage2_minute);
    logger.println(buffer);
  }

  if (mqtt.isTopicEqual(topic, sub_day)) {
    stage2_day = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "Stage 2 Day set to: %d", stage2_day);
    logger.println(buffer);
  }

  if (mqtt.isTopicEqual(topic, sub_month)) {
    stage2_month = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "Stage 2 Month set to: %d", stage2_month);
    logger.println(buffer);
  }

  bool update_default_parameters = false;

  //F1 stg1 on/off time
  if (mqtt.isTopicEqual(topic, sub_f1_st1_ontime)) {
    stage1_params.fanOnTime  = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 1 on time set to: %.1f MINS", stage1_params.fanOnTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_st1_offtime)) {
    stage1_params.fanOffTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 1 off time set to: %.1f MINS", stage1_params.fanOffTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_rev_st1_ontime)) {
    stage1_params.fanRevONTime = mqtt.responseToFloat(payload, len);
    char buffer[60];
    sprintf(buffer, "F1 Stage 1 reverse on time set to: %.1f MINS", stage1_params.fanRevONTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  // F1 and S1 STAGE 2 on/off time
  if (mqtt.isTopicEqual(topic, sub_f1_st2_ontime)) {
    stage2_params.fanOnTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 2 on time set to: %.1f MINS", stage2_params.fanOnTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_st2_offtime)) {
    stage2_params.fanOffTime  = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 2 off time set to: %.1f MINS", stage2_params.fanOffTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_rev_st2_ontime)) {
    stage2_params.fanRevONTime = mqtt.responseToFloat(payload, len);
    char buffer[60];
    sprintf(buffer, "F1 Stage 2 reverse on time set to: %.1f MINS", stage2_params.fanRevONTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_s1_st2_ontime)) {
    stage2_params.sprinklerOnTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "S1 Stage 2 on time set to: %.1f MINS", stage2_params.sprinklerOnTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_s1_st2_offtime)) {
    stage2_params.sprinklerOffTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "S1 Stage 2 off time set to: %.1f MINS", stage2_params.sprinklerOffTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  // F1 and S1 STAGE 3 on/off time
  if (mqtt.isTopicEqual(topic, sub_f1_st3_ontime)) {
    stage3_params.fanOnTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 3 on time set to: %.1f MINS", stage3_params.fanOnTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_st3_offtime)) {
    stage3_params.fanOffTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "F1 Stage 3 off time set to: %.1f MINS", stage3_params.fanOffTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_f1_rev_st3_ontime)) {
    stage3_params.fanRevONTime = mqtt.responseToFloat(payload, len);
    char buffer[60];
    sprintf(buffer, "F1 Stage 3 reverse on time set to: %.1f MINS", stage3_params.fanRevONTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_s1_st3_ontime)) {
    stage3_params.sprinklerOnTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "S1 Stage 3 on time set to: %.1f MINS", stage3_params.sprinklerOnTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_s1_st3_offtime)) {
    stage3_params.sprinklerOffTime = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "S1 Stage 3 off time set to: %.1f MINS", stage3_params.sprinklerOffTime);
    logger.println(buffer);
    update_default_parameters = true;
  }

  // Sub A and Sub B value update
  if (mqtt.isTopicEqual(topic, sub_A)) {
    room.A = mqtt.responseToFloat(payload, len);
    // room.A = atoi((char *)payload);
    char buffer[40];
    sprintf(buffer, "A set to: %.4f", room.A);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_B)) {
    room.B = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "B set to: %.4f", room.B);
    logger.println(buffer);
    update_default_parameters = true;
  }

  // PID update
  if (mqtt.isTopicEqual(topic, sub_P)) {
    Kp = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "P set to: %.4f", Kp);
    logger.println(buffer);
    kp_has_changed = 1;
  }

  if (mqtt.isTopicEqual(topic, sub_I)) {
    Ki = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "I set to: %.4f", Ki);
    logger.println(buffer);
    ki_has_changed = 1;
  }

  if (mqtt.isTopicEqual(topic, sub_D)) {
    Kd = mqtt.responseToFloat(payload, len);
    char buffer[40];
    sprintf(buffer, "D set to: %.4f", Kd);
    logger.println(buffer);
    kd_has_changed = 1;
  }

  if (kp_has_changed == 1 && ki_has_changed == 1 && kd_has_changed == 1) {
    air_in_feed_PID.SetTunings(Kp, Ki, Kd);
    logger.println(F("New PID parameter updated"));
    kp_has_changed = ki_has_changed = kd_has_changed = 0;
  }

  // if (mqtt.isTopicEqual(topic, sub_coefPID)) {
  //   const uint8_t value = mqtt.responseToInt(payload, len);
  //   room.coef_pid_fwd = value;
  //   room.coef_pid_rev = value;
  //   char buffer[40];
  //   sprintf(buffer, "coef PID (legacy) : %d", value);
  //   logger.print(buffer);
  // }

  if (mqtt.isTopicEqual(topic, sub_coefPIDFwd)) {
    room.coef_pid_fwd = mqtt.responseToInt(payload, len);
    char buffer[40];
    sprintf(buffer, "coef PID forward : %d", room.coef_pid_fwd);
    logger.print(buffer);
  }

  if (mqtt.isTopicEqual(topic, sub_coefPIDRev)) {
    room.coef_pid_rev = mqtt.responseToInt(payload, len);
    char buffer[40];
    sprintf(buffer, "coef PID reverse : %d", room.coef_pid_rev);
    logger.print(buffer);
  }

  // Target temperature Ts & Tc update
  if (mqtt.isTopicEqual(topic, sub_ts_set)) {
    temp_set.ts = mqtt.responseToFloat(payload, len);
    char buffer[50];
    sprintf(buffer, "Ts Condition set to: %.2f", temp_set.ts);
    logger.println(buffer);
    update_default_parameters = true;
  }

  if (mqtt.isTopicEqual(topic, sub_tc_set)) {
    temp_set.tc = mqtt.responseToFloat(payload, len);
    // Tc_cond = N_tset->N_tc_set;
    char buffer[50];
    sprintf(buffer, "Tc Condition set to: %.2f", temp_set.tc);
    logger.println(buffer);
    update_default_parameters = true;
  }

  // START  
  if (mqtt.isTopicEqual(topic, sub_start)) {
    const bool value = mqtt.responseToInt(payload, len);
    if (!value) return;

    handleInputs(START);
    logger.println("START BUTTON PRESSED ON NODE RED");
  }
  
  // D_START
  if (mqtt.isTopicEqual(topic, sub_d_start)) {
    const bool value = mqtt.responseToInt(payload, len);
    if (!value) return;
    
    handleInputs(D_START);
    logger.println("d_start BUTTON PRESSED ON NODE RED");
  }

  if(update_default_parameters) controller.updateDefaultParameters(stage1_params, stage2_params, stage3_params, room, temp_set);
}

void stopRoutine() {
  logger.println("PROCESS STOP INITIATED");
  controller.writeDigitalOutput(VALVE_IO, LOW);
  controller.turnOnFan(false);
  controller.writeAnalogOutput(AIR_PWM, 0);

  Output = 0;
  coef_output = 0;

  mqtt.publishData(m_F1, false);
  mqtt.publishData(m_S1, false);
  setStage(IDLE);

  stage_1.destroy();
  stage_2.destroy();
  stage_3.destroy();

  // Reset all timers using memset for efficiency
  memset(&timers, 0, sizeof(SystemTimers));

  logger.println("Stage 0 Status Send packet ");
}

bool isValidTemperature(float temp, float minTemp, float maxTemp, const String& sensorName) {
  bool is_valid = temp >= minTemp && temp <= maxTemp;
  if(is_valid) sendTemperaturaAlert(temp, sensorName);

  return is_valid;
}

void updateTemperature() {
  float ta_raw = controller.readTempFrom(TA_AI);
  float tc_raw = controller.readTempFrom(TC_AI);
  const bool use_ir_ts = controller.isTsContactLess() && controller.hasIRSensor();
  float ts_raw = use_ir_ts ? controller.getIRTemp() : controller.readTempFrom(TS_AI);
  
  
  TA = isValidTemperature(ta_raw, TA_MIN, TA_MAX, "TA") ? ta_raw : TA_DEF;
  if (!controller.isLoraTc()) TC = isValidTemperature(tc_raw, TC_MIN, TC_MAX, "TC") ? tc_raw : TC_DEF;
  TS = isValidTemperature(ts_raw, TS_MIN, TS_MAX, "TS") ? ts_raw : TS_DEF;

  getTempAvg();
}

void sendTemperaturaAlert(float temp, String sensor){
  // const String msg = "{\"temp\":" + String(temp) + ", \"sensor\":" + sensor + "}";
  // mqtt.publishData(SPOILED_SENSOR, msg);
}

void setStage(SystemState stage) {
  if (currentState.stage > IDLE && stage == IDLE) logger.setFileName(DEFAULT_LOG_FILE);
  else if (currentState.stage == IDLE && stage > IDLE) logger.setFileName(controller.getDateTime());
  
  currentState.stage = stage;
  currentState.step = 0;

  mqtt.publishData(STAGE, stage);
  controller.saveLastState(currentState);

  for (int i = STAGE1; i <= STAGE3; i++) {
    if (stageLedPins[i] != -1) digitalWrite(stageLedPins[i], LOW);
  }

  if (stage >= STAGE1 && stage <= STAGE3 && stageLedPins[stage] != -1) {
      digitalWrite(stageLedPins[stage], HIGH);
  }
}

bool hasIntervalPassed(uint32_t &previousMillis, uint32_t interval, bool to_min) {
  unsigned long currentMillis = millis();
  if(to_min) interval *= 60000;
  if (previousMillis == 0) {
    previousMillis = millis() - interval;
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Restablecer el temporizador después de que ha pasado el intervalo
    return true;
  }
  return false; 
}

void publishStateChange(const char* topic, int state, const String& message) {
  mqtt.publishData(topic, state);
  logger.println(message);
}

void getTempAvg() {
  sensorTs.addValue(TS_F); // Añadir el valor al buffer de Ts
  sensorTc.addValue(TC_F);
  sensorTa.addValue(TA_F);

  sensorTsPT100.addValue(controller.readTempFrom(TS_AI));

  // mqtt.publishData(AVG_TS_TOPIC, temp_data.avg_ts);
  // mqtt.publishData(AVG_TC_TOPIC, temp_data.avg_tc);
  // mqtt.publishData(AVG_TA_TOPIC, temp_data.avg_ta);
}

void publishPID(){
  if (hasIntervalPassed(timers.stage2.pid_publish, TIME_ACQ_DELAY + 1)) {
    // logger.println("Coef Output Actual Output is " + String(coef_output));
    const float output_float = float(coef_output);
    pid_output = ((output_float - 0) / (255 - 0)) * (100 - 0) + 0;

    char buffer[50];
    sprintf(buffer, "PID Output /100 is %.2f", pid_output);
    publishStateChange(PID_OUTPUT, pid_output, buffer);
  }
}

void publishTemperatures() {
// void publishTemperatures() {
  temp_data.ta = TA_F;
  temp_data.ts = TS_F;
  temp_data.tc = TC_F;
  temp_data.ti = TI_F;
  temp_data.avg_ts = sensorTs.getAverage();
  temp_data.avg_tc = sensorTc.getAverage();
  temp_data.avg_ta = sensorTa.getAverage();

  const float ts_pt100 = sensorTsPT100.getAverage();

  mqtt.publishData(TA_TOPIC, temp_data.ta);
  mqtt.publishData(TS_TOPIC, temp_data.ts);
  mqtt.publishData(TC_TOPIC, temp_data.tc);
  mqtt.publishData(TI_TOPIC, temp_data.ti);
  mqtt.publishData(TS_PT100_TOPIC, ts_pt100);
  if (controller.hasIRSensor()) {
    const float ts_ir_mlx = controller.getIRTemp();
    mqtt.publishData(TS_IR_MLX_TOPIC, ts_ir_mlx);
  }

  mqtt.publishData(AVG_TA_TOPIC, temp_data.avg_ta);
  mqtt.publishData(AVG_TS_TOPIC, temp_data.avg_ts);
  mqtt.publishData(AVG_TC_TOPIC, temp_data.avg_tc);

  String keys[] = {"Ts", "Tc", "Ta"};
  float values[] = {TS, TC, TA};
  const size_t size = sizeof(keys) / sizeof(keys[0]);

  const String msg = controller.jsonBuilder(keys, values, size);

  logger.println(msg);

  // String keys[] = {"Ts", "Tc", "Ta"};
  // float values[] = {TS, TC, TA};
  // const size_t size = sizeof(keys) / sizeof(keys[0]);
}

void initStage1(){
  logger.println("Stage 1 Initiated");

  // #ifdef SD_Logs
  //   logger.setFileName(controller.getDateTime());
  // #endif

  publishStateChange(m_F1, false, "Stage 1 init M_F1 stop published ");
  publishStateChange(m_F2, false, "Stage 1 init M_F2 stop published ");
  publishStateChange(m_S1, false, "Stage 1 init M_S1 stop published");

  controller.writeDigitalOutput(VALVE_IO, LOW);
  controller.turnOnFan(false);
}

void initStage2(){
  logger.println("Stage 2 Initiated");
  
  publishStateChange(m_F1, false, "Stage 2 init M_F1 stop published ");
  publishStateChange(m_F2, false, "Stage 2 init M_F2 stop published ");
  publishStateChange(m_S1, false, "Stage 2 init M_S1 stop published");
}

void initStage3(){
  logger.println("Stage 3 Initiated");

  publishStateChange(m_F2, false, "Stage 3 M_F2 init published ");
  publishStateChange(m_F1, false, "Stage 3 F1 init published ");
  publishStateChange(m_S1, false, "Stage 3 S1 init published ");
}

void destroyStage1(){
  logger.println("Stage 1 Destroyed");

  controller.writeDigitalOutput(VALVE_IO, LOW);
  controller.turnOnFan(false);
}

void destroyStage2(){
  logger.println("Stage 2 Destroyed");

  Output = 0;
  coef_output = 0;

  controller.writeAnalogOutput(AIR_PWM, 0);
  controller.writeDigitalOutput(VALVE_IO, LOW);

  controller.turnOnFan(false);
}

void destroyStage3(){
  logger.println("Stage 3 Destroyed");

  controller.writeAnalogOutput(AIR_PWM, 0);
  controller.writeDigitalOutput(VALVE_IO, LOW);
  controller.turnOnFan(false);

}

void onMQTTConnect() {
  mqtt.publishData(m_F1, controller.getFanState());
  // mqtt.publishData(m_F2, fan_2);
  mqtt.publishData(m_S1, controller.readDigitalInput(VALVE_IO));
  mqtt.publishData(STAGE, currentState.stage);
  mqtt.publishData(TA_TOPIC, TA);
  mqtt.publishData(TS_TOPIC, TS);
  mqtt.publishData(TC_TOPIC, TC);
  mqtt.publishData(TI_TOPIC, TI);
  mqtt.publishData(PID_OUTPUT, pid_output);
  mqtt.publishData(SETPOINT, Setpoint);
}

void asyncLoopSprinkler(uint32_t &timer, uint32_t offTime, uint32_t onTime) {
  bool isSprinklerOn = controller.readDigitalInput(VALVE_IO);

  if (!isSprinklerOn && hasIntervalPassed(timer, offTime, true)) {
    // Es tiempo de encender el aspersor
    controller.writeDigitalOutput(VALVE_IO, HIGH);
    char buffer[40];
    sprintf(buffer, "Sprinkler ON at: %lu", millis());
    logger.println(buffer);
    
    publishStateChange(m_S1, true, "Stage 2 S1 Start published "); 
  } 
  
  else if (isSprinklerOn && hasIntervalPassed(timer, onTime, true)) {
    // Es tiempo de apagar el aspersor
    controller.writeDigitalOutput(VALVE_IO, LOW);
    char buffer[40];
    sprintf(buffer, "Sprinkler OFF at: %lu", millis());
    logger.println(buffer);

    publishStateChange(m_S1, false, "Stage 2 S1 Stop published ");
  }
}

void aknowledgementRoutine(){
  mqtt.publishData(STAGE, currentState.stage);

  mqtt.publishData(m_F1, controller.getFanState());
  mqtt.publishData(m_F1_CCW, controller.readDigitalInput(FAN_CCW_IO));

  mqtt.publishData(m_S1, controller.readDigitalInput(VALVE_IO));

  mqtt.publishData(SETPOINT, Setpoint);

  // publishTemperatures();
  publishPID();

  // Remaining mins usando epoch: sin overflow, correcto cruzando días/medianoche
  DateTime current_date = controller.getDateTime();
  const long remaining_mins = controller.getRemainingMinutesToStage2(current_date);
  mqtt.publishData(REMAINING_D_START, remaining_mins > 0 ? remaining_mins : 0);

  String keys[] = {AVG_TA_TOPIC, AVG_TS_TOPIC, AVG_TC_TOPIC, PID_OUTPUT};
  float values[] = {temp_data.avg_ta, temp_data.avg_ts, temp_data.avg_tc, pid_output};
  const size_t size = sizeof(keys) / sizeof(keys[0]);

  const String msg = controller.jsonBuilder(keys, values, size);

  if (currentState.stage > IDLE) controller.saveLogToSD(msg);
}

void turn_on_flush_routine() {
  controller.writeDigitalOutput(FLUSH_IO, HIGH);
  turn_off_flush.restartDelayed(10*1000);
}

void turn_off_flush_routine() {
  controller.writeDigitalOutput(FLUSH_IO, LOW);
}
