#include <basics.h>

/* P R O T O T Y P E S ********************************************************/ 
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/  
s_wifi wifi;  // global WiFi Informations



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


/**
 * *******************************************************************
 * @brief   create date and time as String
 * @param   none
 * @return  pointer to date and time String (local static memory)
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
  static char dateTimeInfo[74]={'\0'};      // Date and time info String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%02d.%02d.%02d - %02i:%02i:%02i", dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec);
  return dateTimeInfo;
}

/**
 * *******************************************************************
 * @brief   create date  String
 * @param   none
 * @return  pointer to date String (local static memory)
 * *******************************************************************/
const char * getDateString() {
  static char dateInfo[74]={'\0'};          // Date String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(dateInfo, sizeof(dateInfo), "%02d.%02d.%02d", dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900));
  return dateInfo;
}

/**
 * *******************************************************************
 * @brief   create time as String
 * @param   none
 * @return  pointer to time String (local static memory)
 * *******************************************************************/
const char * getTimeString() {
  static char timeInfo[74]={'\0'};          // Date and time info String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(timeInfo, sizeof(timeInfo), "%02i:%02i:%02i", dti.tm_hour, dti.tm_min, dti.tm_sec);
  return timeInfo;
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
void sendWiFiInfo() {
 // check  RSSI
  wifi.rssi = WiFi.RSSI();
  
  //dBm to Quality:
  if(wifi.rssi <= -100)
      wifi.signal = 0;
  else if(wifi.rssi >= -50)
      wifi.signal = 100;
  else
      wifi.signal = 2 * (wifi.rssi + 100);

    IPAddress localIP = WiFi.localIP();
    snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
    DynamicJsonDocument wifiJSON(255);
    wifiJSON["status"] = "online";
    wifiJSON["rssi"] = wifi.rssi;
    wifiJSON["signal"] = wifi.signal;
    wifiJSON["ip"] = wifi.ipAddress;
    wifiJSON["date-time"] = getDateTimeString();

    char sendWififJSON[255];
    serializeJson(wifiJSON, sendWififJSON);
    mqttPublish(addTopic("/wifi"),sendWififJSON, false); 

    // wifi status
    mqttPublish(addTopic("/status"), "online", false);

}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint8ToString(uint8_t value){
  static char ret_str[4];
  snprintf(ret_str, sizeof(ret_str), "%u", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint64_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint64ToString(uint64_t value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%llu", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as int8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* int8ToString(int8_t value){
  static char ret_str[4];
  snprintf(ret_str, sizeof(ret_str), "%i", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as float
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* floatToString(float value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.1f", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as double
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* doubleToString(double value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.1lf", value);
  return ret_str;
}