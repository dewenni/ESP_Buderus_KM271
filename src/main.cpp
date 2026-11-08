// includes
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include <basics.h>
#include <config.h>
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

// The bootloader is built with rollback enabled, but the Arduino core cancels
// it inside initArduino() - before setup() runs - by marking the freshly
// flashed image valid immediately. That means rollback only ever caught images
// that fail to boot at all, not the far more likely case of an image that
// boots and then cannot reach the network or dies later in startup. Since OTA
// is the only update channel to this device, that is the difference between
// "reboots into the previous firmware" and "someone has to travel to it with a
// USB cable".
//
// Overriding this weak symbol defers the decision to us; markAppValid() below
// makes it once the device has proven it actually works.
bool verifyRollbackLater() { return true; }

#define OTA_VALIDATE_MIN_UPTIME 120000  // healthy for 2 min with a network
#define OTA_VALIDATE_MAX_UPTIME 600000  // ... or 10 min regardless, see below
static bool otaImageValidated = false;

/**
 * *******************************************************************
 * @brief   confirm the running image once it has proven itself, so the
 *          bootloader stops considering it for rollback
 * @param   none
 * @return  none
 * *******************************************************************/
static void markAppValid() {
  if (otaImageValidated) {
    return;
  }

  // Normal case: the network came up and we have been alive for a while, so
  // the image is good. The fallback on uptime alone covers a device that is
  // legitimately offline - in setup/AP mode, or with no WiFi configured yet -
  // which must not be rolled back just for having no network.
  bool healthy = (millis() > OTA_VALIDATE_MIN_UPTIME) && (wifi.connected || eth.connected);
  if (!healthy && millis() < OTA_VALIDATE_MAX_UPTIME) {
    return;
  }

  otaImageValidated = true;

  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY) {
    if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
      ESP_LOGI(TAG, "OTA image confirmed - rollback cancelled");
    } else {
      ESP_LOGE(TAG, "failed to confirm OTA image");
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
  ArduinoOTA.setHostname(config.wifi.hostname);
  // Without a password ArduinoOTA skips its challenge-response entirely, so
  // anyone on the network could push arbitrary firmware (espota.py -i <ip>)
  // with no credentials at all - a second flash path that bypasses the auth
  // on the web UI's /update route. Guard it with the same device password.
  ArduinoOTA.setPassword(devicePassword());
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
  if (wdt.isActive() && wdtTimer.cycleTrigger(2000)) {
    esp_task_wdt_reset();
  }

  // OTA Update
  ArduinoOTA.handle();

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