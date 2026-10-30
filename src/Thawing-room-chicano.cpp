#include "Thawing-room-chicano.h"
#include "esp_task_wdt.h"   // Task Watchdog (punto 4)

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
// Semilla = valor default hasta la primera lectura buena (evita arrancar en 0).
float TA = TA_DEF, TA_F = TA_DEF;  //Ta
float TS = TS_DEF, TS_F = TS_DEF;  //Ts
float TC = TC_DEF, TC_F = TC_DEF;  //Tc
float TI = 0, TI_F = 0;  //Ti optional

// Flags de fallo por sensor (punto 1): true = última lectura fuera de rango físico.
// Mientras true, se conserva el último valor bueno y ya se publicó la alarma.
bool ta_fault = false, ts_fault = false, tc_fault = false;
// millis() del último aviso de cada sensor, para el re-aviso periódico mientras falla.
uint32_t ta_fault_report = 0, ts_fault_report = 0, tc_fault_report = 0;

// ########################### Buffer ##########################
SensorBuffer sensorTs(BUFFER_SIZE);  // Crear una instancia para el sensor Ts
SensorBuffer sensorTc(10);  // Crear otra instancia para el sensor Tc
SensorBuffer sensorTa(10);  // Crear otra instancia para el sensor Ta

SensorBuffer sensorTsPT100(10);  // Crear otra instancia para el sensor TsPT100

// SystemState currentState = IDLE;
StageState currentState = {IDLE, 0};
// Cuándo se entró a la etapa actual, para la página de control. Tras un reinicio con
// proceso en curso no se sabe: se toma el arranque y se marca stage_since_boot.
uint32_t stage_since_ms = 0;
bool stage_since_boot = false;

// Bloqueo por mantenimiento (ver setMaintenanceLock). Persistido en NVS "maint": un
// reinicio o un corte de luz a mitad de la reparación NO debe liberarlo.
bool     maintenance_lock = false;
uint32_t maintenance_since = 0;     // epoch LOCAL de activación (0 = desconocido)
uint8_t  maintenance_source = SRC_SYSTEM;

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
// Es el registro de "por qué nos reiniciamos": tiene que sobrevivir en la tarjeta, así
// que va a INFO/CRIT y no por println() (que queda en DEBUG y solo llega a Serial).
void logResetReason() {
  // Sello del binario. Con despliegue manual, tres personas y 9 cuartos, "¿qué firmware
  // tiene puesto este equipo?" es una pregunta constante; que la conteste el propio log.
  LOG_I(TAG_APP, "[BOOT] Firmware compilado " __DATE__ " " __TIME__);

  const esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:  LOG_I(TAG_APP, "[BOOT] Arranque por energia");            break;
    case ESP_RST_SW:       LOG_I(TAG_APP, "[BOOT] Reinicio por software");           break;
    case ESP_RST_PANIC:    LOG_C(TAG_APP, "[BOOT] Reinicio por panic / crash");      break;
    case ESP_RST_INT_WDT:  LOG_C(TAG_APP, "[BOOT] Reinicio por interrupt WDT");      break;
    case ESP_RST_TASK_WDT: LOG_C(TAG_APP, "[BOOT] Reinicio por task WDT");           break;
    case ESP_RST_WDT:      LOG_C(TAG_APP, "[BOOT] Reinicio por WDT (otro)");         break;
    case ESP_RST_BROWNOUT: LOG_C(TAG_APP, "[BOOT] Reinicio por brownout (baja tension)"); break;
    default:               LOG_I(TAG_APP, "[BOOT] Causa de reinicio desconocida (%d)", (int)reason); break;
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

    // Único punto del firmware que escribe la SD de logs. Vive acá a propósito:
    // así el lazo de control (core 1) nunca se bloquea en I/O y no hay dos tasks
    // sobre el mismo bus SPI, con lo que tampoco hace falta un mutex.
    logger.drain();
    // Mismo hilo que el Logger: EventLog también toca la SD (events.csv).
    events.drain();
    // Reinicios pedidos desde la web: se ejecutan acá, después de drenar, para que el
    // evento "restart requested" llegue a flash antes de que se apague todo.
    serviceRestart();

    // printStackUsage(); // Monitorea el uso de la pila
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void setup() {
  controller.init();

  logger.begin();   // cola del logger; el reloj se lo alimenta loop() con setClock()
  events.begin();   // bitácora de eventos (SPIFFS ya montado por controller.init())
  calib.begin();   // calibración de 3 puntos por sonda (/calibration.txt); antes de la primera lectura

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

  // online = hay config válida Y WiFi conectado (modo estación). En modo AP/portal
  // queda false: sin red ni broker no tiene sentido intentar MQTT.
  bool online = false;
  if (!config_ok) {
    // Sin config.txt no hay credenciales que probar: portal de configuración directo.
    // startConfigPortal levanta el AP y registra el servidor web.
    logger.println(F("No config - starting AP config portal"));
    controller.startConfigPortal();
  } else {
    online = controller.connectToWiFi(/* web_server */ true, /* web_serial */ true, /* OTA */ true);
    if (!online) {
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

  // Flush periódico: deshabilitado por ahora (no hay flag de configuración que lo
  // active). Las tasks turn_on/off_flush quedan registradas para habilitarlo a futuro
  // con turn_on_flush.enable() cuando se agregue la clave de config correspondiente.

  DateTime current_date = controller.getDateTime();
  char dateBuffer[60];
  sprintf(dateBuffer, "Current date: %d:%d %d/%d", current_date.hour(), current_date.minute(), current_date.day(), current_date.month());
  logger.println(dateBuffer);
  // Alimentar el reloj ya en setup: sin esto los eventos de arranque salen sin hora,
  // porque loop() todavía no corrió.
  logger.setClock(current_date, controller.isTimeTrusted());
  loadMaintenanceLock();
  logBootEvents(config_ok, online);

  StageState last_state = controller.getLastState();
  char stateBuffer[50];
  sprintf(stateBuffer, "Last state: %d %d", last_state.stage, last_state.step);
  logger.println(stateBuffer);
  if (last_state.stage != IDLE) {
    currentState = last_state;
    stage_since_ms = millis();
    stage_since_boot = true;
    events.setStage(last_state.stage);
    events.cycleResume(SRC_SYSTEM, "Resumed Stage %d after controller restart", (int)last_state.stage);
  } else {
    events.cycleClear();
  }
  showStageLeds();   // tras un reinicio con proceso en curso las luces quedaban apagadas

  // Solo levantar MQTT si estamos online (estación con WiFi). En modo AP/portal
  // IP_ADDRESS puede estar vacío y no hay red: intentar conectar sería un gasto inútil
  // (con TLS además se cuelga ~30s en el handshake hacia un host inalcanzable).
  if (online) {
    // ANTES de connect(): connect() ya suscribe, y la suscripción necesita el prefijo.
    // PREFIX_TOPIC viene vacío si config.txt no trae "TOPIC" -> queda DEFAULT_TOPIC_PREFIX.
    mqtt.setTopicPrefix(PREFIX_TOPIC);
    mqtt.setCallback(callback);
    mqtt.onConnect(onMQTTConnect);
    mqtt.setTLS(controller.isTLSEnabled());  // verifica el broker contra el CA embebido (mqtt_certs.h)

    // TLS valida la vigencia del certificado contra el reloj del sistema. No basta con
    // que el RTC dé una fecha "razonable": si está atrasada respecto al notBefore del
    // cert, el handshake falla con -9984. Por eso, igual que el sketch de referencia,
    // forzamos una sync NTP fresca ANTES de conectar siempre que haya WiFi.
    if (controller.isTLSEnabled() && controller.isWiFiConnected()) {
      logger.println(F("TLS: sincronizando hora por NTP antes de conectar MQTT"));
      controller.forceNTPSync();
    }

    mqtt.connect(IP_ADDRESS, PORT, MQTT_ID, USERNAME, MQTT_PASSWORD);
  } else {
    logger.println(F("AP/offline mode - skipping MQTT connect"));
  }

  xTaskCreatePinnedToCore(backgroundTasks, "communicationTask", 20000, NULL, 1, &communicationTask, 0);

  logResetReason();  // log why we (re)started — written after logger is ready
  logger.println(F("===========> Reboted!! <==========="));

  //Turn the PID on
  air_in_feed_PID.SetMode(AUTOMATIC);
  air_in_feed_PID.SetSampleTime(3000);
  air_in_feed_PID.SetTunings(Kp, Ki, Kd);

  // El rango del PID (0-255 por defecto de la librería, no se llama a SetOutputLimits)
  // no coincide con el del PWM (RESOLUTION bits). Hoy eso hace que el infeed de aire
  // trabaje al tope, que es como opera la planta a propósito. NO se corrige acá: se
  // deja registrado para que el día que cambie -por ejemplo si una versión nueva del
  // core empieza a recortar el duty- se vea en el log en vez de perseguirlo a ciegas.
  {
    const uint32_t pwm_max = (1UL << RESOLUTION) - 1UL;
    if (pwm_max < 255UL) {
      LOG_W(TAG_APP, "PID 0-255 contra PWM de %u bits (max %lu): la salida de aire satura",
            (unsigned)RESOLUTION, (unsigned long)pwm_max);
    }
  }

  delay(750);

  // Armar el watchdog al FINAL del setup: todo lo legítimamente lento (WiFi, NTP,
  // handshake TLS) ya ocurrió arriba. A partir de acá, si el loop se cuelga > WDT_TIMEOUT_MS
  // el equipo se reinicia y recupera el estado del ciclo desde flash.
  setUpWatchdog();
}

// ---- Task Watchdog (punto 4) ----
// Reinicia si la task del loop() deja de alimentar el WDT. No vigila las idle tasks
// (idle_core_mask = 0) para NO disparar por bloqueos legítimos de WiFi/OTA en el core 0.
void setUpWatchdog() {
  esp_task_wdt_config_t wdt_cfg = {
    .timeout_ms     = WDT_TIMEOUT_MS,
    .idle_core_mask = 0,
    .trigger_panic  = true,
  };
  // El core de Arduino ya inicializa el TWDT; si es así, reconfigurar en vez de re-init.
  if (esp_task_wdt_init(&wdt_cfg) == ESP_ERR_INVALID_STATE) {
    esp_task_wdt_reconfigure(&wdt_cfg);
  }
  controller.endBootPhase();   // desde acá el NTP no puede bloquear (ver Controller::ntpServerIP)
  esp_task_wdt_add(NULL);   // vigilar la task actual (loopTask, core 1)
  esp_task_wdt_reset();
  logger.println(F("[BOOT] Watchdog armado"));
}

void loop() {
  esp_task_wdt_reset();   // alimentar el watchdog en cada iteración (punto 4)
  runner.execute();
  // if is for testing porpuse comment this "if" and replace DateTime "now" for: DateTime now(__DATE__, __TIME__);
  DateTime current_date = controller.getDateTime();

  // Se le pasa el reloj al logger desde acá, que es donde ya se lee. El drenador corre
  // en el core 0 y así no necesita tocar el RTC: sin I2C compartido entre cores y sin
  // escribir el estado del RTC de Controller desde dos tasks.
  logger.setClock(current_date, controller.isTimeTrusted());

  // if (!controller.isRTCConnected()) {  
  //   logger.println("RTC not connected"); 
  //   while (true) delay(1000);
  // }

  if(controller.isWiFiConnected()) {
    mqtt.loop();
  }
  watchBrokerLink();

  updateTemperature();

  // if ((TA) > (LOW_TEMP_LIMIT) && (TA) < (HIGH_TEMP_LIMIT))
   TA_F = TA;  // if the temperature over the limit it will not be considered
  
  // if ((TS) > (LOW_TEMP_LIMIT) && (TS) < (HIGH_TEMP_LIMIT))
   TS_F = TS;
  
  // if ((TC) > (LOW_TEMP_LIMIT) && (TC) < (HIGH_TEMP_LIMIT))
   TC_F = TC;
  
  // if ((TI) > (LOW_TEMP_LIMIT) && (TI) < (HIGH_TEMP_LIMIT))
   TI_F = TI;

  serviceRemoteControl(current_date);
  handleInputs();
  serviceMaintenanceLeds();
  serviceCalibration();

  handleStage();

  // if (hasIntervalPassed(get_temp_timer, TIME_ACQ_DELAY)) publishTemperatures();  
  
}


bool handleInputs(button_type override, EventSource override_source) {
  button_type button = NONE;
  bool pressed_btn = false;

  // override llega del callback MQTT (SCADA) o de la página de control (WEB). Si en la
  // misma vuelta además se soltó un botón físico, gana el botón, y el origen tiene que
  // decir eso.
  EventSource source = override ? override_source : SRC_MANUAL;
  if (override) pressed_btn = button = override;

  if      (d_start_button.released()) { button = D_START; source = SRC_MANUAL; }
  else if (start_btn.released())      { button = START;   source = SRC_MANUAL; }
  else if (stop_btn.released())       { button = STOP;    source = SRC_MANUAL; }

  if ((button == D_START || button == START) && maintenance_lock) {
    // Nada arranca en mantenimiento, venga de donde venga. STOP sí pasa.
    events.emit(EV_ALERT, source, "%s blocked: maintenance lockdown is active",
                button == START ? "Start" : "Delayed start");
    LOG_W(TAG_STAGE, "Arranque bloqueado por mantenimiento (origen %s)", EventLog::sourceName(source));
    mqtt.publishData(MAINTENANCE_STATE, 1);
    return !pressed_btn;
  }

  if      (button == D_START)         setStage(STAGE1, source);
  else if (button == START)           setStage(STAGE2, source);
  else if (button == STOP)            stopRoutine(source); 

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
    char reason[48];
    if (overdue > 1) snprintf(reason, sizeof(reason), "scheduled time reached, %ld min late", overdue);
    else             snprintf(reason, sizeof(reason), "scheduled time reached");
    stage_1.destroy();
    setStage(STAGE2, SRC_AUTO, reason);
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
    char reason[64];
    snprintf(reason, sizeof(reason), "Ts %.1f >= %.1f and Tc %.1f >= %.1f", TS_F, temp_set.ts, TC_F, temp_set.tc);
    stage_2.destroy();
    setStage(STAGE3, SRC_AUTO, reason);
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

  // La Etapa 3 NO tiene condición de fin automática, por diseño: el producto permanece
  // en descongelado hasta que producción presiona STOP (al sacar el pescado del cuarto).
  // El fin del ciclo lo dispara stopRoutine() vía botón STOP físico o MQTT (sub_stop).
}

void idle(){
  // delay(1000);
}

//// fct Callback ==> RECEIVE MQTT MESSAGES ////////////////////////////////////////////////////////////////////
static void handleMqttMessage(char *topic, byte *payload, unsigned int len) {
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
    updateSensorReading(tc_raw, TC, TC_MIN, TC_MAX, "TC", tc_fault, ADC_MEDIAN_NO_PIN, tc_fault_report);

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
  

  // Bloqueo por mantenimiento: va ANTES del corte por etapa, para poder activarlo
  // también con el proceso corriendo.
  if (mqtt.isTopicEqual(topic, sub_maintenance)) {
    setMaintenanceLock(mqtt.responseToInt(payload, len) != 0, SRC_REMOTE);
    return;
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

// ---- Parámetros que puede mover el SCADA ----
// En vez de tocar los ~30 bloques del callback, se toma una foto de los valores antes
// de procesar el mensaje y se compara después: solo lo que cambió de verdad queda en
// la bitácora, aunque el SCADA reenvíe el mismo valor.
struct WatchedParam { const char* name; const char* unit; char kind; void* ptr; };
static const WatchedParam watched_params[] = {
  {"Stage 2 start hour",          "",    'u', &stage2_hour},
  {"Stage 2 start minute",        "",    'u', &stage2_minute},
  {"Stage 2 start day",           "",    'u', &stage2_day},
  {"Stage 2 start month",         "",    'u', &stage2_month},
  {"Stage 1 F1 on time",          " min", 'f', &stage1_params.fanOnTime},
  {"Stage 1 F1 off time",         " min", 'f', &stage1_params.fanOffTime},
  {"Stage 1 F1 reverse on time",  " min", 'f', &stage1_params.fanRevONTime},
  {"Stage 1 S1 on time",          " min", 'f', &stage1_params.sprinklerOnTime},
  {"Stage 1 S1 off time",         " min", 'f', &stage1_params.sprinklerOffTime},
  {"Stage 2 F1 on time",          " min", 'f', &stage2_params.fanOnTime},
  {"Stage 2 F1 off time",         " min", 'f', &stage2_params.fanOffTime},
  {"Stage 2 F1 reverse on time",  " min", 'f', &stage2_params.fanRevONTime},
  {"Stage 2 S1 on time",          " min", 'f', &stage2_params.sprinklerOnTime},
  {"Stage 2 S1 off time",         " min", 'f', &stage2_params.sprinklerOffTime},
  {"Stage 3 F1 on time",          " min", 'f', &stage3_params.fanOnTime},
  {"Stage 3 F1 off time",         " min", 'f', &stage3_params.fanOffTime},
  {"Stage 3 F1 reverse on time",  " min", 'f', &stage3_params.fanRevONTime},
  {"Stage 3 S1 on time",          " min", 'f', &stage3_params.sprinklerOnTime},
  {"Stage 3 S1 off time",         " min", 'f', &stage3_params.sprinklerOffTime},
  {"A",                           "",    'f', &room.A},
  {"B",                           "",    'f', &room.B},
  {"PID Kp",                      "",    'd', &Kp},
  {"PID Ki",                      "",    'd', &Ki},
  {"PID Kd",                      "",    'd', &Kd},
  {"PID coef forward",            "",    'u', &room.coef_pid_fwd},
  {"PID coef reverse",            "",    'u', &room.coef_pid_rev},
  {"Ts set-point",                " C",  'f', &temp_set.ts},
  {"Tc set-point",                " C",  'f', &temp_set.tc},
  {"Ts from IR (chooseTs)",       "",    'b', &is_rts_ir},
};
static const size_t WATCHED_PARAM_COUNT = sizeof(watched_params) / sizeof(watched_params[0]);

static double readParam(const WatchedParam &p) {
  switch (p.kind) {
    case 'u': return *(const uint8_t*)p.ptr;
    case 'b': return *(const bool*)p.ptr ? 1 : 0;
    case 'f': return *(const float*)p.ptr;
    default:  return *(const double*)p.ptr;
  }
}

void callback(char *topic, byte *payload, unsigned int len) {
  double before[WATCHED_PARAM_COUNT];
  for (size_t i = 0; i < WATCHED_PARAM_COUNT; i++) before[i] = readParam(watched_params[i]);
  const bool lora_tc_before = controller.isLoraTc();
  const bool ts_ir_before   = controller.isTsContactLess();

  handleMqttMessage(topic, payload, len);

  for (size_t i = 0; i < WATCHED_PARAM_COUNT; i++) {
    const double after = readParam(watched_params[i]);
    if (fabs(after - before[i]) < 1e-6) continue;
    events.emit(EV_PARAM, SRC_REMOTE, "%s: %g -> %g%s",
                watched_params[i].name, before[i], after, watched_params[i].unit);
  }
  if (controller.isLoraTc() != lora_tc_before) {
    events.emit(EV_PARAM, SRC_REMOTE, "Tc source: %s", controller.isLoraTc() ? "LoRa" : "wired probe");
  }
  if (controller.isTsContactLess() != ts_ir_before) {
    events.emit(EV_PARAM, SRC_REMOTE, "Ts source: %s", controller.isTsContactLess() ? "IR sensor" : "PT-100 probe");
  }
}

// ---- Enlace con el SCADA ----
// Un corte corto de WiFi/broker es normal (el cliente reconecta solo): solo se registra
// si dura más de un minuto, y la recuperación solo si antes se registró el corte.
void watchBrokerLink() {
  static uint32_t down_since = 0;
  static bool was_up = false, reported = false;
  if (controller.isAPMode()) return;

  if (controller.isWiFiConnected() && mqtt.isConnected()) {
    if (reported) {
      events.emit(EV_SYSTEM, SRC_SYSTEM, "SCADA connection restored after %lu min",
                  (unsigned long)((millis() - down_since) / 60000UL));
    }
    was_up = true; down_since = 0; reported = false;
    return;
  }
  if (!was_up) return;   // todavía no conectó nunca en este arranque: no es un corte
  if (down_since == 0) { down_since = millis(); return; }
  if (!reported && millis() - down_since >= 60000UL) {
    reported = true;
    events.emit(EV_ALERT, SRC_SYSTEM, controller.isWiFiConnected()
                ? "Lost connection to the MQTT broker: SCADA cannot see or control the room"
                : "Wi-Fi lost: SCADA cannot see or control the room");
  }
}

// ---- Eventos de arranque ----
// ---- Página de control (/control) ----
// Corre en el loop: ejecuta el comando que haya dejado un handler web y publica la foto
// del proceso. Los comandos entran por handleInputs(), el mismo camino que el SCADA, así
// que respetan las mismas reglas y quedan en la bitácora con origen WEB.
void serviceRemoteControl(const DateTime &now) {
  ControlCommand cmd;
  if (remote.take(cmd)) {
    const SystemState before = currentState.stage;
    char msg[96];
    bool ok = false;

    if (cmd.action == CTRL_STOP) {
      if (before == IDLE) {
        snprintf(msg, sizeof(msg), "The process was already stopped.");
      } else {
        handleInputs(STOP, SRC_WEB);
        ok = currentState.stage == IDLE;
        snprintf(msg, sizeof(msg), ok ? (before == STAGE3 ? "Process finished." : "Process stopped.")
                                      : "The stop did not go through.");
      }
    } else if (cmd.action == CTRL_LOCK_ON || cmd.action == CTRL_LOCK_OFF) {
      const bool on = cmd.action == CTRL_LOCK_ON;
      ok = setMaintenanceLock(on, SRC_WEB) || maintenance_lock == on;
      snprintf(msg, sizeof(msg), on ? "Maintenance lockdown enabled. Starting is blocked."
                                    : "Maintenance lockdown lifted. Starting is allowed again.");
    } else if (maintenance_lock) {
      // Lo activó el SCADA entre la validación del handler y esta vuelta.
      snprintf(msg, sizeof(msg), "Not started: maintenance lockdown is active.");
    } else if (before != IDLE) {
      // Entre la validación del handler y esta vuelta alguien más arrancó el proceso.
      snprintf(msg, sizeof(msg), "Not started: the process is already in Stage %d.", (int)before);
    } else if (cmd.action == CTRL_START) {
      handleInputs(START, SRC_WEB);
      ok = currentState.stage == STAGE2;
      snprintf(msg, sizeof(msg), ok ? "Process started in Stage 2." : "The start did not go through.");
    } else if (cmd.action == CTRL_DSTART) {
      stage2_hour   = cmd.hour;
      stage2_minute = cmd.minute;
      stage2_day    = cmd.day;
      stage2_month  = cmd.month;
      handleInputs(D_START, SRC_WEB);
      ok = currentState.stage == STAGE1;
      if (ok) snprintf(msg, sizeof(msg), "Delayed start: Stage 2 at %02u:%02u on %02u/%02u.",
                       stage2_hour, stage2_minute, stage2_day, stage2_month);
      else    snprintf(msg, sizeof(msg), "The delayed start did not go through.");
    } else {
      snprintf(msg, sizeof(msg), "Unknown action.");
    }
    remote.finish(cmd.id, ok, msg);
    LOG_I(TAG_APP, "Web control: %s", msg);
  }

  static uint32_t last_publish = 0;
  if (millis() - last_publish < 500) return;
  last_publish = millis();

  ControlStatus s = {};
  const SystemState stage = currentState.stage;
  s.stage = stage;
  Stage *running = stage == STAGE1 ? &stage_1 : stage == STAGE2 ? &stage_2 : stage == STAGE3 ? &stage_3 : nullptr;
  s.step = running ? running->getCurrentStep() : 0;
  s.starting = (stage == STAGE2 || stage == STAGE3) && s.step <= 1 && !controller.getFanState();
  s.cycle = events.cycle();
  s.cycle_active = events.cycleActive();
  s.stage_since = stage_since_ms ? logger.epochAt(stage_since_ms) : 0;
  s.stage_since_boot = stage_since_boot;

  s.ta = TA_F;  s.ts = TS_F;  s.tc = TC_F;
  s.avg_ta = sensorTa.getAverage();
  s.avg_ts = sensorTs.getAverage();
  s.avg_tc = sensorTc.getAverage();
  s.ta_fault = ta_fault;  s.ts_fault = ts_fault;  s.tc_fault = tc_fault;
  s.ta_counts = controller.getAdcDiag(TA_AI).filtered;
  // 0xFFFF = la lectura no sale del ADC (Ts por IR, Tc por LoRa): no hay cuentas.
  s.ts_counts = (controller.isTsContactLess() && controller.hasIRSensor()) ? 0xFFFF : controller.getAdcDiag(TS_AI).filtered;
  s.tc_counts = controller.isLoraTc() ? 0xFFFF : controller.getAdcDiag(TC_AI).filtered;
  s.ts_set = temp_set.ts;
  s.tc_set = temp_set.tc;
  s.pid_setpoint = pid_setpoint;

  s.fan_on = controller.getFanState();
  s.fan_reverse = s.fan_on && controller.readDigitalInput(FAN_CCW_IO);
  s.damper = controller.readDigitalInput(AIR_DAMPER_IO);
  s.sprinkler = controller.readDigitalInput(VALVE_IO);
  s.air_duty = controller.getAirDuty();
  s.air_max = (uint8_t)((1U << RESOLUTION) - 1U);

  s.s2_hour = stage2_hour;  s.s2_minute = stage2_minute;
  s.s2_day = stage2_day;    s.s2_month = stage2_month;
  s.s2_set = controller.isStage2TimeSet();
  s.s2_remaining_min = s.s2_set ? controller.getRemainingMinutesToStage2(now) : -1;

  s.lock = maintenance_lock;
  s.lock_since = maintenance_since;
  s.lock_source = maintenance_source;

  s.wifi = controller.isWiFiConnected();
  s.mqtt = s.wifi && mqtt.isConnected();
  s.sd = logger.hasSD();
  s.time_trusted = controller.isTimeTrusted();
  s.time_source = controller.getTimeSource();

  remote.publish(s);
}

void logBootEvents(bool config_ok, bool online) {
  const esp_reset_reason_t reason = esp_reset_reason();
  const char* cause = "unknown cause";
  bool abnormal = false;
  switch (reason) {
    case ESP_RST_POWERON:  cause = "power-on";                           break;
    case ESP_RST_SW:       cause = "software restart";                   break;
    case ESP_RST_PANIC:    cause = "crash (panic)";          abnormal = true; break;
    case ESP_RST_INT_WDT:  cause = "interrupt watchdog";     abnormal = true; break;
    case ESP_RST_TASK_WDT: cause = "task watchdog";          abnormal = true; break;
    case ESP_RST_WDT:      cause = "watchdog";               abnormal = true; break;
    case ESP_RST_BROWNOUT: cause = "brownout (low voltage)"; abnormal = true; break;
    default: break;
  }
  events.emit(EV_SYSTEM, SRC_SYSTEM, "Controller started (%s) - firmware v%s, built %s %s",
              cause, VERSION, __DATE__, __TIME__);
  if (abnormal) events.emit(EV_ERROR, SRC_SYSTEM, "Unexpected restart: %s", cause);

  if (!config_ok)      events.emit(EV_ERROR, SRC_SYSTEM, "No valid config.txt: configuration portal started");
  else if (!online)    events.emit(EV_ALERT, SRC_SYSTEM, "Wi-Fi unavailable at boot: configuration portal started");

  if (!controller.isRTCConnected())       events.emit(EV_ERROR, SRC_SYSTEM, "RTC not detected: using fallback clock");
  else if (!controller.isTimeTrusted())   events.emit(EV_ALERT, SRC_SYSTEM, "Clock not set: event times may be wrong");

  if (!logger.hasSD()) events.emit(EV_ALERT, SRC_SYSTEM, "No SD card: only the last %d events are kept", EVENT_LOG_CAPACITY);
}

void stopRoutine(EventSource source) {
  const SystemState from = currentState.stage;
  LOG_I(TAG_STAGE, "PARO DE PROCESO iniciado (etapa %d)", (int)currentState.stage);
  controller.writeDigitalOutput(VALVE_IO, LOW);
  controller.turnOnFan(false);
  controller.writeAnalogOutput(AIR_PWM, 0);

  Output = 0;
  coef_output = 0;

  mqtt.publishData(m_F1, false);
  mqtt.publishData(m_S1, false);
  setStage(IDLE, source);

  // STOP en etapa 3 es el final normal del ciclo (la etapa 3 no termina sola); en
  // cualquier otra etapa es un paro. STOP con el cuarto en reposo no registra nada.
  if      (from == STAGE3) events.cycleEnd(source, true,  "Process finished");
  else if (from == ERROR)  events.cycleEnd(source, false, "Process stopped after an error");
  else if (from != IDLE)   events.cycleEnd(source, false, "Process stopped during Stage %d", (int)from);

  stage_1.destroy();
  stage_2.destroy();
  stage_3.destroy();

  // Reset all timers using memset for efficiency
  memset(&timers, 0, sizeof(SystemTimers));

  logger.println("Stage 0 Status Send packet ");
}

// Publica la alarma de sensor a MQTT y la loguea. fault=true al detectar el fallo,
// fault=false al recuperarse. `raw` es la lectura cruda que disparó el evento.
// OJO, cambio de formato: "raw" ahora son CUENTAS del ADC y los °C van en "temp".
// Antes "raw" traía °C, que es justo el dato que NO sirve para diagnosticar la cadena
// 4-20 mA (un lazo abierto y una sonda genuinamente fría dan ambos temperaturas bajas;
// solo las cuentas los distinguen). `channel` = ADC_MEDIAN_NO_PIN cuando el valor no
// viene del ADC (Ts por IR, Tc por LoRa): ahí no hay cuentas que publicar.
// Clasifica la falla a partir de las CUENTAS del ADC. Es lo que separa "se cortó el
// cable" de "hace frío de verdad", que hasta ahora se veían idénticos.
//   < ~819 cuentas -> por debajo de 4 mA: un lazo 4-20 mA vivo no puede estar ahí
//   = 4095         -> tope de escala: transmisor pegado
const char* classifyFault(uint8_t channel, bool fault) {
  if (!fault) return "OK";
  if (channel == ADC_MEDIAN_NO_PIN) return "OUT_OF_RANGE";  // no viene del ADC (IR / LoRa)

  const Controller::AdcDiag d = controller.getAdcDiag(channel);
  if (!d.valid)               return "NO_DATA";
  if (d.filtered < ADC_LOOP_MIN_COUNTS) return "OPEN_LOOP";
  if (d.filtered >= ADC__RESOLUTION)    return "SATURATED";
  return "OUT_OF_RANGE";
}

void sendSensorFault(const char* sensor, bool fault, float temp, uint8_t channel) {
  const char* reason = classifyFault(channel, fault);
  char msg[160];

  if (channel == ADC_MEDIAN_NO_PIN) {
    snprintf(msg, sizeof(msg), "{\"sensor\":\"%s\",\"fault\":%s,\"reason\":\"%s\",\"temp\":%.2f}",
             sensor, fault ? "true" : "false", reason, temp);
  } else {
    const Controller::AdcDiag d = controller.getAdcDiag(channel);
    snprintf(msg, sizeof(msg),
             "{\"sensor\":\"%s\",\"fault\":%s,\"reason\":\"%s\",\"temp\":%.2f,\"raw\":%u,\"med\":%u,\"fails\":%u}",
             sensor, fault ? "true" : "false", reason, temp, d.raw, d.filtered, d.fails);
  }

  mqtt.publishData(SENSOR_FAULT, String(msg));

  // A la SD con nivel según gravedad: un lazo abierto es una falla física que hay
  // que poder rastrear después; la recuperación es informativa.
  if (fault) LOG_E(TAG_ADC, "%s FALLA %s: %s", sensor, reason, msg);
  else       LOG_I(TAG_ADC, "%s recuperado (%.2f C)", sensor, temp);
}

// Cuentas crudas de los tres canales analógicos. No convierte a °C a propósito: la
// conversión es justo lo que está bajo sospecha (rampa ajustada en Excel sobre un
// módulo no lineal), así que este tópico publica lo que el ADC realmente ve.
// Lectura: 819 = 4 mA, 4095 = 20 mA. Por debajo de 819 el lazo está abierto.
void publishAdcDiagnostics() {
  const Controller::AdcDiag ta = controller.getAdcDiag(TA_AI);
  const Controller::AdcDiag ts = controller.getAdcDiag(TS_AI);
  const Controller::AdcDiag tc = controller.getAdcDiag(TC_AI);

  char msg[200];
  snprintf(msg, sizeof(msg),
           "{\"ta\":{\"raw\":%u,\"med\":%u,\"fails\":%u},"
            "\"ts\":{\"raw\":%u,\"med\":%u,\"fails\":%u},"
            "\"tc\":{\"raw\":%u,\"med\":%u,\"fails\":%u}}",
           ta.raw, ta.filtered, ta.fails,
           ts.raw, ts.filtered, ts.fails,
           tc.raw, tc.filtered, tc.fails);

  mqtt.publishData(ADC_RAW, String(msg));
  // DEBUG: es telemetría continua. A la SD solo llega si alguien baja el umbral para
  // diagnosticar; si no, saturaría la tarjeta con una línea cada 10 s.
  LOG_D(TAG_ADC, "%s", msg);
}

// Punto 1: si la lectura es válida actualiza `value`. Si NO lo es, conserva el último
// valor bueno (no toca `value`) y publica alarma una sola vez por transición. El ciclo
// NO se detiene: el proceso sigue con el último valor confiable.
bool updateSensorReading(float raw, float &value, float lo, float hi, const char* name,
                         bool &fault, uint8_t channel, uint32_t &last_report) {
  const bool valid = raw >= lo && raw <= hi;
  if (valid) {
    value = raw;
    if (fault) {                                    // recuperación
      fault = false;
      sendSensorFault(name, false, raw, channel);
      events.emit(EV_ALERT, SRC_SYSTEM, "Sensor %s recovered (%.1f C)", name, raw);
    }
    return true;
  }

  if (!fault) {                                     // nuevo fallo
    fault = true;
    last_report = millis();
    sendSensorFault(name, true, raw, channel);
    events.emit(EV_ALERT, SRC_SYSTEM, "Sensor %s fault: %s (reading %.1f C, last good value kept)",
                name, classifyFault(channel, true), raw);
    return false;
  }

  // Falla que persiste. Sin esto se avisaba UNA sola vez y después silencio: una sonda
  // que se muere de madrugada quedaba sin rastro y el ciclo seguía con el último valor
  // bueno congelado. Se re-avisa cada SENSOR_FAULT_REPEAT_MS para que quede en el log
  // y el SCADA pueda alarmar sobre algo que sigue presente.
  if (millis() - last_report >= SENSOR_FAULT_REPEAT_MS) {
    last_report = millis();
    sendSensorFault(name, true, raw, channel);
  }
  return false;
}

void updateTemperature() {
  float ta_raw = controller.readTempFrom(TA_AI);
  float tc_raw = controller.readTempFrom(TC_AI);
  const bool use_ir_ts = controller.isTsContactLess() && controller.hasIRSensor();
  float ts_raw = use_ir_ts ? controller.getIRTemp() : controller.readTempFrom(TS_AI);

  updateSensorReading(ta_raw, TA, TA_MIN, TA_MAX, "TA", ta_fault, TA_AI, ta_fault_report);
  // Cuando Tc llega por LoRa (isLoraTc), no se lee del canal analógico: lo actualiza el callback.
  if (!controller.isLoraTc()) updateSensorReading(tc_raw, TC, TC_MIN, TC_MAX, "TC", tc_fault, TC_AI, tc_fault_report);
  // Con Ts por IR el valor no viene del ADC, así que no hay cuentas que reportar.
  updateSensorReading(ts_raw, TS, TS_MIN, TS_MAX, "TS", ts_fault,
                      use_ir_ts ? ADC_MEDIAN_NO_PIN : TS_AI, ts_fault_report);

  getTempAvg();
}

void setStage(SystemState stage, EventSource source, const char* reason) {
  const SystemState from = currentState.stage;
  if (currentState.stage > IDLE && stage == IDLE) logger.setFileName(DEFAULT_LOG_FILE);
  else if (currentState.stage == IDLE && stage > IDLE) logger.setFileName(controller.getDateTime());
  
  // INFO: los cambios de etapa son EL evento del proceso. Van a la tarjeta para poder
  // reconstruir un ciclo después, a diferencia del ruido de DEBUG.
  LOG_I(TAG_STAGE, "Etapa %d -> %d", (int)currentState.stage, (int)stage);

  currentState.stage = stage;
  currentState.step = 0;
  stage_since_ms = millis();
  stage_since_boot = false;

  events.setStage(stage);
  if (stage == IDLE) {
    // El paro / fin lo registra stopRoutine(), que sabe en qué etapa estaba.
  } else if (from == IDLE || from == ERROR) {
    if (stage == STAGE1) {
      events.cycleStart(source, "Delayed start: Stage 1, Stage 2 scheduled for %02u:%02u on %02u/%02u",
                        stage2_hour, stage2_minute, stage2_day, stage2_month);
    } else {
      events.cycleStart(source, "Process started directly in Stage %d", (int)stage);
    }
  } else if (from == stage) {
    events.emit(EV_PROCESS, source, "Stage %d restarted", (int)stage);
  } else if (reason) {
    events.emit(EV_PROCESS, source, "Stage %d -> Stage %d (%s)", (int)from, (int)stage, reason);
  } else {
    events.emit(EV_PROCESS, source, "Stage %d -> Stage %d", (int)from, (int)stage);
  }

  mqtt.publishData(STAGE, stage);
  controller.saveLastState(currentState);

  showStageLeds();
}

// Luz fija de la etapa vigente. En bloqueo por mantenimiento manda el parpadeo
// (serviceMaintenanceLeds) y esto no toca nada.
void showStageLeds() {
  if (maintenance_lock) return;
  const SystemState stage = currentState.stage;
  for (int i = STAGE1; i <= STAGE3; i++) {
    if (stageLedPins[i] != -1) digitalWrite(stageLedPins[i], LOW);
  }
  if (stage >= STAGE1 && stage <= STAGE3 && stageLedPins[stage] != -1) {
    digitalWrite(stageLedPins[stage], HIGH);
  }
}

// ---- Asistente de calibración (/calibration) ----
// Aplica la calibración que haya guardado la web y le pasa al asistente las cuentas del
// ADC (la mediana, la misma que da la temperatura) cada CAL_SAMPLE_MS.
void serviceCalibration() {
  calib.service();

  static uint32_t last_sample = 0;
  if (millis() - last_sample < CAL_SAMPLE_MS) return;
  last_sample = millis();

  const bool avail[CAL_CHANNELS] = {
    true,
    !(controller.isTsContactLess() && controller.hasIRSensor()),
    !controller.isLoraTc()
  };
  const uint8_t pins[CAL_CHANNELS] = {TA_AI, TS_AI, TC_AI};
  for (uint8_t ch = 0; ch < CAL_CHANNELS; ch++) {
    if (!avail[ch]) continue;
    const Controller::AdcDiag d = controller.getAdcDiag(pins[ch]);
    if (d.valid) calib.sample(ch, d.filtered);
  }
  calib.setContext(maintenance_lock, currentState.stage, avail);
}

// ---- Bloqueo por mantenimiento ----
void loadMaintenanceLock() {
  Preferences prefs;
  prefs.begin("maint", true);
  maintenance_lock   = prefs.getBool("lock", false);
  maintenance_since  = prefs.getUInt("since", 0);
  maintenance_source = prefs.getUChar("src", SRC_SYSTEM);
  prefs.end();
  if (maintenance_lock) {
    LOG_W(TAG_APP, "Bloqueo por mantenimiento ACTIVO desde el arranque");
    events.emit(EV_ALERT, SRC_SYSTEM, "Maintenance lockdown still active after restart: starting is blocked");
  }
}

// Devuelve false si ya estaba en ese estado (no se registra nada).
// NO para un proceso en curso: un cambio de modo no debe mover actuadores por su cuenta;
// para eso está STOP, que sigue funcionando.
bool setMaintenanceLock(bool on, EventSource source) {
  if (on == maintenance_lock) {
    mqtt.publishData(MAINTENANCE_STATE, on);   // re-confirmar al SCADA aunque no cambie
    return false;
  }
  maintenance_lock   = on;
  maintenance_since  = on ? logger.epochAt(millis()) : 0;
  maintenance_source = source;

  Preferences prefs;
  prefs.begin("maint", false);
  prefs.putBool("lock", on);
  prefs.putUInt("since", maintenance_since);
  prefs.putUChar("src", (uint8_t)source);
  prefs.end();

  if (on) {
    if (currentState.stage != IDLE) {
      events.emit(EV_ALERT, source, "Maintenance lockdown enabled while Stage %d is running: the process keeps running until stopped",
                  (int)currentState.stage);
    } else {
      events.emit(EV_ALERT, source, "Maintenance lockdown enabled: starting is blocked");
    }
  } else {
    events.emit(EV_SYSTEM, source, "Maintenance lockdown lifted: starting is allowed again");
    showStageLeds();   // devolver las luces a la etapa real
  }
  LOG_I(TAG_APP, "Bloqueo por mantenimiento %s (origen %s)", on ? "ACTIVADO" : "levantado", EventLog::sourceName(source));
  mqtt.publishData(MAINTENANCE_STATE, on);
  return true;
}

// Las 3 luces de etapa parpadean juntas mientras dure el bloqueo: se ve desde el pasillo
// que el cuarto está en reparación. No bloqueante, 500 ms encendido / 500 ms apagado.
void serviceMaintenanceLeds() {
  if (!maintenance_lock) return;
  static uint32_t last_toggle = 0;
  static bool lit = false;
  if (millis() - last_toggle < 500) return;
  last_toggle = millis();
  lit = !lit;
  for (int i = STAGE1; i <= STAGE3; i++) {
    if (stageLedPins[i] != -1) digitalWrite(stageLedPins[i], lit ? HIGH : LOW);
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

// `value` es double, no int: por acá pasan tanto estados booleanos (m_F1, m_S1) como
// magnitudes con decimales (SETPOINT, PID_OUTPUT). Con int, el setpoint se truncaba a
// entero ANTES de publicarse -20.67 salía como "20.00"-, mientras que los mismos tópicos
// publicados desde aknowledgementRoutine sí llevaban los decimales. De ahí que en el
// broker el valor se viera entero casi siempre y con decimales de vez en cuando.
// Para los booleanos no cambia nada: true ya salía como "1.00".
void publishStateChange(const char* topic, double value, const String& message) {
  mqtt.publishData(topic, value);
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
  mqtt.publishData(MAINTENANCE_STATE, maintenance_lock);
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
  mqtt.publishData(MAINTENANCE_STATE, maintenance_lock);

  mqtt.publishData(m_F1, controller.getFanState());
  mqtt.publishData(m_F1_CCW, controller.readDigitalInput(FAN_CCW_IO));

  mqtt.publishData(m_S1, controller.readDigitalInput(VALVE_IO));

  mqtt.publishData(SETPOINT, Setpoint);

  // publishTemperatures();
  publishPID();

  publishAdcDiagnostics();  // cuentas crudas del ADC: diagnóstico de la cadena 4-20 mA

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
