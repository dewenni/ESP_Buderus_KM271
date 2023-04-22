
#include <config.h>
#include <basics.h>
#include <mqtt.h>
#include <km271.h>
#include <webUI.h>
#include <oilmeter.h>

// Double-Reset-Detector
#define ESP_DRD_USE_LITTLEFS    true
#define DRD_TIMEOUT 10
#define DRD_ADDRESS 0
#include <ESP_DoubleResetDetector.h>
DoubleResetDetector* drd;

/* D E C L A R A T I O N S ****************************************************/  
muTimer mainTimer = muTimer();        // timer for cyclic info
muTimer heartbeat = muTimer();        // timer for heartbeat signal
muTimer setupModeTimer = muTimer();   // timer for heartbeat signal
muTimer dstTimer = muTimer();         // timer to check daylight saving time change

bool main_reboot = true;        // reboot flag
int dst_old;                    // reminder for change of daylight saving time

/**
 * *******************************************************************
 * @brief   function layer to store data before update or reboot
 * @param   none
 * @return  none
 * *******************************************************************/
void storeData(){
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
void setup()
{
  //Enable serial port
  Serial.begin(115200);
  while(!Serial) {} // Wait

  // check for double reset
  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  if (drd->detectDoubleReset())
  {
    setupMode = true;
  }

  // initial configuration
  configSetup();

  // basic setup function (WiFi, OTA)
  basic_setup();

  // MQTT
  if (config.mqtt.enable && !setupMode) {
    mqttSetup();
  }

  // send initial WiFi infos
  if (!setupMode) {
    sendWiFiInfo();
  }
  
  // setup for km271
  if (!setupMode) {
    km271ProtInit(config.gpio.km271_RX, config.gpio.km271_TX);
  }

  // setup Oilmeter
  if (config.oilmeter.use_hardware_meter && !setupMode) {
    setupOilmeter();
  }

  // webUI Setup
  if (config.webUI.enable) { 
    webUISetup();
  }

} 

/**
 * *******************************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void loop()
{

  // double reset detector
  drd->loop();

  // chck WiFi
  if (!setupMode) {
    check_wifi();
  }

  // chck MQTT
  if (config.mqtt.enable && !setupMode) {
    mqttCyclic();
  }

  
  if (setupMode){
    // LED to Signal Setup-Mode
    digitalWrite(LED_BUILTIN, setupModeTimer.cycleOnOff(100,500));
    digitalWrite(21, setupModeTimer.cycleOnOff(100,500));
  }
  else {
    // LED for WiFi connected
    if (config.gpio.led_wifi != -1)
      digitalWrite(config.gpio.led_wifi, WiFi.status() != WL_CONNECTED); // (true=LED off)
    // LED for KM271 LogMode active
    if (config.gpio.led_logmode != -1)
      digitalWrite(config.gpio.led_logmode, !km271GetLogMode()); // (true=LED off)
    // LED for heartbeat
    if (config.gpio.led_heartbeat != -1)
      digitalWrite(config.gpio.led_heartbeat, heartbeat.cycleOnOff(1000,1000));
  }

  // cyclic call for KM271
  if (!setupMode) {
    cyclicKM271();
  }

  // cyclic Oilmeter
  if (config.oilmeter.use_hardware_meter && !setupMode) {
    cyclicOilmeter();
  }

  // send cyclic infos
  if (mainTimer.cycleTrigger(10000) && !setupMode)
  {
    sendWiFiInfo();
    sendKM271Info();
    sendKM271Debug();
  }

  if (config.webUI.enable) {
    webUICylic(); // call webUI
  }

  // check every hour if DST has changed
  if (config.ntp.enable) {
    if (dstTimer.cycleTrigger(3600000))
    {
      time_t now;                       
      tm dti;                           
      time(&now);                                   // read the current time
      localtime_r(&now, &dti);                      // update the structure tm with the current time
      if (dst_old!=dti.tm_isdst && !main_reboot){   // check if DST has changed
        km271SetDateTimeNTP();                      // change date and time on buderus
      }
      dst_old=dti.tm_isdst;    
    }
  }


  main_reboot = false; // reset reboot flag
}

