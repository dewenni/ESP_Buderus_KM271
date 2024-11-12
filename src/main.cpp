// includes
#include <ArduinoOTA.h>
#include <ESP_DoubleResetDetector.h>
#include <basics.h>
#include <config.h>
#include <esp_task_wdt.h>
#include <km271.h>
#include <message.h>
#include <mqtt.h>
#include <oilmeter.h>
#include <sensor.h>
#include <simulation.h>
#include <telnet.h>
#include <webUI.h>

/* D E C L A R A T I O N S ****************************************************/
muTimer heartbeat = muTimer();      // timer for heartbeat signal
muTimer setupModeTimer = muTimer(); // timer for heartbeat signal
muTimer dstTimer = muTimer();       // timer to check daylight saving time change
muTimer ntpTimer = muTimer();       // timer to check ntp sync

DoubleResetDetector *drd; // Double-Reset-Detector
bool main_reboot = true;  // reboot flag
int dst_old;              // reminder for change of daylight saving time
bool dst_ref;             // init flag fpr dst reference
bool ntpSynced;           // ntp sync flag
bool ntpInit = false;     // init flag for ntp sync
bool otaActive = false;   // OTA active flag

esp_task_wdt_config_t twdt_config{timeout_ms : 10000U, idle_core_mask : 0b10, trigger_panic : true};

/**
 * *******************************************************************
 * @brief   is OTA update active?
 * @param   none
 * @return  true if update is active
 * *******************************************************************/
bool otaActiveState() { return otaActive; }

/**
 * *******************************************************************
 * @brief   function layer to store data before update or reboot
 * @param   none
 * @return  none
 * *******************************************************************/
void storeData() {
  if (config.oilmeter.use_hardware_meter) {
    cmdStoreOilmeter();
  }
}

/**
 * *******************************************************************
 * @brief   Main Setup routine
 * @param   none
 * @return  none
 * *******************************************************************/
void setup() {

  // setup watchdog timer
  if (!setupMode) {
    esp_task_wdt_init(&twdt_config); // Initialize ESP32 Task WDT
    esp_task_wdt_add(NULL);          // Subscribe to the Task WDT
  }

  // Message Service Setup
  messageSetup();

  // check for double reset
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset()) {
    setupMode = true;
  }

  // initial configuration
  configSetup();

  // basic setup functions
  basicSetup();

  // Setup OTA
  ArduinoOTA.onStart([]() {
    storeData();               // store Data before update
    esp_task_wdt_delete(NULL); // disable watchdog timer
    otaActive = true;
  });
  ArduinoOTA.onEnd([]() {
    esp_task_wdt_add(NULL); // re-activate Watchdog
    otaActive = false;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    esp_task_wdt_add(NULL); // re-activate Watchdog
    otaActive = false;
  });
  ArduinoOTA.setHostname(config.wifi.hostname);
  ArduinoOTA.begin();

  // setup for km271
  if (!setupMode) {
    km271ProtInit(config.gpio.km271_RX, config.gpio.km271_TX);
  }

  // setup Oilmeter
  if (config.oilmeter.use_hardware_meter && !setupMode) {
    setupOilmeter();
  }

  // webUI Setup
  webUISetup();

  // Sensor Setup
  setupSensor();

  // telnet Setup
  setupTelnet();
}

/**
 * *******************************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void loop() {

  // reset watchdog
  if (!setupMode) {
    esp_task_wdt_reset();
  }

  // OTA Update
  ArduinoOTA.handle();

  // double reset detector
  drd->loop();

  // webUI Cyclic
  webUICyclic();

  // Sensor Cyclic
  cyclicSensor();

  // Message Service
  messageCyclic();

  // get simulation telegrams of KM271
  simDataCyclic();

  // telnet communication
  cyclicTelnet();

  // check if config has changed
  configCyclic();

  // check WiFi - automatic reconnect
  if (!setupMode) {
    checkWiFi();
  }

  // check MQTT - automatic reconnect
  if (config.mqtt.enable && !setupMode) {
    mqttCyclic();
  }

  if (setupMode) {
    // LED to Signal Setup-Mode
    digitalWrite(LED_BUILTIN, setupModeTimer.cycleOnOff(100, 500));
    digitalWrite(21, setupModeTimer.cycleOnOff(500, 100));
  } else {
    // LED for WiFi connected
    if (config.gpio.led_wifi != -1)
      digitalWrite(config.gpio.led_wifi, WiFi.status() != WL_CONNECTED); // (true=LED off)
    // LED for KM271 LogMode active
    if (config.gpio.led_logmode != -1)
      digitalWrite(config.gpio.led_logmode, !km271GetLogMode()); // (true=LED off)
    // LED for heartbeat
    if (config.gpio.led_heartbeat != -1)
      digitalWrite(config.gpio.led_heartbeat, heartbeat.cycleOnOff(1000, 1000));
  }

  // cyclic call for KM271
  if (!setupMode && !config.sim.enable) {
    cyclicKM271();
  }

  // cyclic Oilmeter
  if (config.oilmeter.use_hardware_meter && !setupMode) {
    cyclicOilmeter();
  }

  if (config.ntp.enable) {
    // check every hour if DST has changed
    if (dstTimer.cycleTrigger(3600000)) {
      time_t now;
      tm dti;
      time(&now);              // read the current time
      localtime_r(&now, &dti); // update the structure tm with the current time

      if (!dst_ref) { // save actual DST as reference after bootup
        dst_ref = true;
        dst_old = dti.tm_isdst;
      }

      if (dst_old != dti.tm_isdst && !main_reboot) { // check if DST has changed
        km271SetDateTimeNTP();                       // change date and time on buderus
      }
      dst_old = dti.tm_isdst;
    }
    // set ntp time on logamatic after bootup and successful ntp sync
    if (ntpTimer.cycleTrigger(10000)) {
      struct tm timeInfo;
      ntpSynced = getLocalTime(&timeInfo, 5);
      if (ntpSynced && !ntpInit && config.ntp.auto_sync) {
        km271SetDateTimeNTP();
        ntpInit = true;
      }
    }
  }

  main_reboot = false; // reset reboot flag
}
