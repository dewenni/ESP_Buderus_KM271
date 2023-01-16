
#include <basics.h>
#include <mqtt.h>
#include <km271.h>
#include <webUI.h>
#include <oilmeter.h>

/* P I N - A S S I G N M E N T *************************************************/  
#define LED_WIFI        21     // LED for WiFi Status
#define LED_HEARBEAT    22     // LED for heartbeat
#define LED_LOGMODE     23     // LED for LogMode


/* D E C L A R A T I O N S ****************************************************/  
muTimer mainTimer = muTimer();    // timer for cyclic info
muTimer heartbeat = muTimer();    // timer for heartbeat signal
muTimer dstTimer = muTimer();     // timer to check daylight saving time change


bool main_reboot = true;        // reboot flag
int dst_old;                    // reminder for change of daylight saving time 

/**
 * *******************************************************************
 * @brief   function layer to store data before update or reboot
 * @param   none
 * @return  none
 * *******************************************************************/
void storeData(){
  #ifdef USE_OILMETER
    cmdStoreOilmeter();
  #endif
}

/**
 * *******************************************************************
 * @brief   Main Setup routine
 * @param   none
 * @return  none
 * *******************************************************************/
void setup()
{
  // basic setup function (WiFi, OTA)
  basic_setup();

  // MQTT
  mqttSetup();

  //Enable serial port
  Serial.begin(115200);
  while(!Serial) {} // Wait

  pinMode(LED_WIFI, OUTPUT);        // LED for Wifi-Status
  pinMode(LED_HEARBEAT, OUTPUT);    // LED for heartbeat
  pinMode(LED_LOGMODE, OUTPUT);     // LED for LogMode-Status

  // send initial WiFi infos
  sendWiFiInfo();

  // setup for km271
  km271ProtInit(RXD2, TXD2);

  // setup Oilmeter
  #ifdef USE_OILMETER
    setupOilmeter();
  #endif

  // webUI Setup
  #ifdef USE_WEBUI
    webUISetup(); 
  #endif

}

/**
 * *******************************************************************
 * @brief   Main Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void loop()
{
  // WiFi + MQTT
  check_wifi();
  mqttCyclic();

  // LED for WiFi connected
  digitalWrite(LED_WIFI, WiFi.status() != WL_CONNECTED); // (true=LED off)
  // LED for KM271 LogMode active
  digitalWrite(LED_LOGMODE, !km271GetLogMode()); // (true=LED off)
  // LED for heartbeat
  digitalWrite(LED_HEARBEAT, heartbeat.cycleOnOff(1000,1000));

  // OTA Update
  ArduinoOTA.handle();

  // cyclic call for KM271
  cyclicKM271();

  // cyclic Oilmeter
  #ifdef USE_OILMETER
    cyclicOilmeter();
  #endif

  // send cyclic infos
  if (mainTimer.cycleTrigger(10000))
  {
    sendWiFiInfo();
    sendKM271Info();
  }

  #ifdef USE_WEBUI
    webUICylic(); // call webUI
  #endif

  // check every hour if DST has changed
  if (dstTimer.cycleTrigger(3600000))
  {
    time_t now;                       
    tm dti;                           
    time(&now);                                   // read the current time
    localtime_r(&now, &dti);                      // update the structure tm with the current time
    if (dst_old!=dti.tm_isdst && !main_reboot){   // check if DST has changed
      km271SetDateTime();                         // change date and time on buderus
    }
    dst_old=dti.tm_isdst;    
  }



  main_reboot = false; // reset reboot flag
}

