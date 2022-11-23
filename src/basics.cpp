#include <basics.h>
 
/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup(){
  #ifdef ARDUINO_ARCH_ESP32
    // ESP32 seems to be a little more complex:
    configTime(0, 0, MY_NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", MY_TZ, 1); // Set environment variable with your time zone
    tzset();
  #else
    // ESP8266
    configTime(MY_TZ, MY_NTP_SERVER); // --> for the ESP8266 only
  #endif
}


time_t now;                       // this is the epoch
tm dti;                           // the structure tm holds time information in a more convient way
char dateTimeInfo[74]={'\0'};     // Date and time info String
/**
 * *******************************************************************
 * @brief   create date and time as String
 * @param   none
 * @return  pointer to date and time String
 * *******************************************************************/
const char * getDateTimeString() {
   /* ---------------- INFO ---------------------------------
  dti.tm_year + 1900  // years since 1900
  dti.tm_mon + 1      // January = 0 (!)
  dti.tm_mday         // day of month
  dti.tm_hour         // hours since midnight  0-23
  dti.tm_min          // minutes after the hour  0-59
  dti.tm_sec          // seconds after the minute  0-61*
  dti.tm_wday         // days since Sunday 0-6
  dti.tm_isdst        // Daylight Saving Time flag
  --------------------------------------------------------- */
  time(&now);                       // read the current time
  localtime_r(&now, &dti);          // update the structure tm with the current time
  sprintf(dateTimeInfo, "%d.%d.%d - %02i:%02i:%02i", dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec);
  return dateTimeInfo;
}

/**
 * *******************************************************************
 * @brief   Setup for OTA Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupOTA(){
 ArduinoOTA
    .onStart([]() {
      // actions to do when OTA starts
      storeData(); // store Data before update
      delay(500);
    });

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();
}

/**
 * *******************************************************************
 * @brief   Setup for general WiFi Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PW);
  WiFi.hostname(HOSTNAME);
}

/**
 * *******************************************************************
 * @brief   Check WiFi status and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void check_wifi(){

    // Check WiFi connectivity
   int wifi_retry = 0;
   while(WiFi.status() != WL_CONNECTED && wifi_retry < 5 ) {
     wifi_retry++;
     Serial.println("WiFi not connected. Trying to connect...");
     WiFi.disconnect();
     WiFi.mode(WIFI_OFF);
     delay(10);
     WiFi.mode(WIFI_STA);
     WiFi.begin(WIFI_SSID, WIFI_PW);
     delay(WIFI_RECONNECT);
   }
   if(wifi_retry >= 5) {
     Serial.println("\nWifi connection not possible, rebooting...");
     storeData(); // store Data before reboot
     delay(500);
     ESP.restart();
   } else if (wifi_retry > 0){
     Serial.println("WiFi reconnected");
     Serial.println("IP address: ");
     Serial.println(WiFi.localIP());
   }
}


/**
 * *******************************************************************
 * @brief   Call all basic Setup functions at once
 * @param   none
 * @return  none
 * *******************************************************************/
void basic_setup() {
    // WiFi
    setup_wifi();

    // OTA
    setupOTA();
    
    //NTP
    ntpSetup();
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
long wifiRssi;
int wifiSignal;

void sendWiFiInfo() {
 // check  RSSI
  wifiRssi = WiFi.RSSI();
  
  //dBm to Quality:
  if(wifiRssi <= -100)
      wifiSignal = 0;
  else if(wifiRssi >= -50)
      wifiSignal = 100;
  else
      wifiSignal = 2 * (wifiRssi + 100);

    IPAddress localIP = WiFi.localIP();
    char bufIP[20];
    sprintf(bufIP, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

    DynamicJsonDocument wifiJSON(1024);

    wifiJSON["status"] = "online";
    wifiJSON["rssi"] = wifiRssi;
    wifiJSON["signal"] = wifiSignal;
    wifiJSON["ip"] = bufIP;
    wifiJSON["date-time"] = getDateTimeString();

    String sendWififJSON;
    serializeJson(wifiJSON, sendWififJSON);
    mqttPublish(addTopic("/wifi"),String(sendWififJSON).c_str(), false); 

    // wifi status
    mqttPublish(addTopic("/status"), "online", false);

}
