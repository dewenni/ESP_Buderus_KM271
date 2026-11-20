// includes
#include <ArduinoOTA.h>
#include <WiFiMgn.h>
#include <basics.h>
#include <config.h>
#include <esp_ota_ops.h>
#include <km271.h>
#include <message.h>
#include <mqtt.h>
#include <oilmeter.h>
#include <sensor.h>
#include <simulation.h>
#include <telnet.h>
#include <webUI.h>
#include <webUIupdates.h>

/* D E C L A R A T I O N S ****************************************************/
static muTimer heartbeat = muTimer();      // timer for heartbeat signal
static muTimer setupModeTimer = muTimer(); // timer for heartbeat signal
static muTimer dstTimer = muTimer();       // timer to check daylight saving time change
static muTimer ntpTimer = muTimer();       // timer to check ntp sync
static muTimer wdtTimer = muTimer();       // timer to reset wdt

static EspSysUtil::MRD32 *mrd;   // Double-Reset-Detector
static bool main_reboot = true;  // reboot flag
static int dst_old;              // reminder for change of daylight saving time
static bool dst_ref;             // init flag fpr dst reference
static bool ntpSynced;           // ntp sync flag
static bool ntpInit = false;     // init flag for ntp sync
static const char *TAG = "MAIN"; // LOG TAG

static auto &wdt = EspSysUtil::Wdt::getInstance();
static auto &ota = EspSysUtil::OTA::getInstance();

// The core otherwise confirms a pending image before setup() runs.
extern "C" bool verifyRollbackLater() { return true; }

static constexpr uint32_t OTA_VALIDATE_UPTIME = 30000;
static bool otaImageValidated = false;

static void handleArduinoOTA() {
  static bool running = false;
  static char password[sizeof(config.service.password)] = {};
  static char hostname[sizeof(config.wifi.hostname)] = {};
  if (ota.isActive()) {
    if (running) ArduinoOTA.handle();
    return;
  }
  bool allowed = serviceAccessEnabled();
  const char *desiredHostname = config.wifi.hostname[0] ? config.wifi.hostname : "ESP-Buderus-KM271";
  if (running && (!allowed || strcmp(password, servicePassword()) || strcmp(hostname, desiredHostname))) {
    ArduinoOTA.end();
    running = false;
  }
  if (allowed && !running) {
    snprintf(password, sizeof(password), "%s", servicePassword());
    snprintf(hostname, sizeof(hostname), "%s", desiredHostname);
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword(password);
    ArduinoOTA.begin();
    running = true;
  }
  if (running) ArduinoOTA.handle();
}

/**
 * *******************************************************************
 * @brief   confirm the running image once it has proven itself, so the
 *          bootloader stops considering it for rollback
 * @param   none
 * @return  none
 * *******************************************************************/
static void markAppValid() {
  if (otaImageValidated || ota.isActive())
    return;
  const uint32_t uptime = millis();
  if (uptime < OTA_VALIDATE_UPTIME)
    return;

  static uint32_t lastAttempt = 0;
  if (uptime - lastAttempt < 10000)
    return;
  lastAttempt = uptime;
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (running == nullptr)
    return;
  const esp_err_t result = esp_ota_get_state_partition(running, &state);
  if (result == ESP_ERR_NOT_FOUND || result == ESP_ERR_NOT_SUPPORTED) {
    otaImageValidated = true; // No rollback state on this installation.
  } else if (result == ESP_OK) {
    if (state != ESP_OTA_IMG_PENDING_VERIFY) {
      otaImageValidated = true;
    } else if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      otaImageValidated = true;
      ESP_LOGI(TAG, "OTA image confirmed - rollback cancelled");
    } else {
      ESP_LOGE(TAG, "failed to confirm OTA image; will retry");
    }
  }
}

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

  // Message Service Setup (before use of ESP_LOGx)
  messageSetup();

  // Cache firmware metrics before any network service or OTA can run.
  initFirmwareInfo();

  // check for multiple reset
  mrd = new EspSysUtil::MRD32(MRD_TIMEOUT, MRD_RETRIES);
  if (mrd->detectMultipleResets()) {
    ESP_LOGI(TAG, "SETUP-MODE-REASON: MRD detected");
    setupMode = true;
  }

  // initial configuration (can also activate the Setup Mode)
  configSetup();

  // setup watchdog timer
  if (!setupMode) {
    wdt.enable();
  }

  // basic setup functions
  basicSetup();

  // Setup OTA
  ArduinoOTA.onStart([]() {
    ESP_LOGI(TAG, "OTA-started");
    storeData();   // store Data before update
    wdt.disable(); // disable watchdog timer
    ota.setActive(true);
  });
  ArduinoOTA.onEnd([]() {
    ESP_LOGI(TAG, "OTA-finished");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    ESP_LOGI(TAG, "OTA-error");
    if (!setupMode) {
      wdt.enable();
    }
    ota.setActive(false);
  });
  handleArduinoOTA();

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
  if (wdt.isActive() && wdtTimer.cycleTrigger(2000)) {
    esp_task_wdt_reset();
  }

  // OTA Update
  handleArduinoOTA();

  // double reset detector
  mrd->loop();

  // confirm a freshly flashed image once it has proven itself
  markAppValid();

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

  // WiFi scans and automatic reconnect
  wifiCyclic();

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
      digitalWrite(config.gpio.led_wifi, !wifiIsConnected()); // (true=LED off)
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