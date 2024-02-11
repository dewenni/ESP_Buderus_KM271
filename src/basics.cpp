#include <basics.h>
#include <webTools.h>

/* P R O T O T Y P E S ********************************************************/ 
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/  
s_wifi wifi;  // global WiFi Informations
muTimer wifiReconnectTimer = muTimer();           // timer for reconnect delay
int wifi_retry = 0;

/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup(){
  #ifdef ARDUINO_ARCH_ESP32
    // ESP32 seems to be a little more complex:
    configTime(0, 0, config.ntp.server); // 0, 0 because we will use TZ in the next line
    setenv("TZ", config.ntp.tz, 1); // Set environment variable with your time zone
    tzset();
  #else
    // ESP8266
    configTime(NTP_TZ, NTP_SERVER); // --> for the ESP8266 only
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
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  wifi_retry=0;
  Serial.println("Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void checkWiFi(){
  if (wifiReconnectTimer.delayOnTrigger(!WiFi.isConnected(), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();
    if (wifi_retry < 5) {
      wifi_retry++;
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.wifi.ssid, config.wifi.password);
      WiFi.hostname(config.wifi.hostname);
      MDNS.begin(config.wifi.hostname);
      Serial.print("WiFi Mode STA - Trying connect to: ");
      Serial.print(config.wifi.ssid);
      Serial.print(" - attempt: ");
      Serial.print(wifi_retry);
      Serial.println("/5");
    }
    else {
      Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
      Serial.println("Wifi connection not possible, esp rebooting...");
      Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
      storeData(); // store Data before reboot
      delay(500);
      ESP.restart();
    }
  }
}

/**
 * *******************************************************************
 * @brief   Setup for general WiFi Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupWiFi() {
  
  if (setupMode) {
    // start Accesspoint for initial setup
    IPAddress ip(192,168,4,1);
    IPAddress gateway(192,168,4,1); 
    IPAddress subnet(255,255,255,0);
    WiFi.softAPConfig(ip, gateway, subnet);
    WiFi.softAP("ESP-Buderus-km271");
    Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
    Serial.println("> WiFi Mode: AccessPoint <");
    Serial.println("1. connect your device to SSID: ESP-Buderus-km271");
    Serial.println("2. open Browser and go to Address: http://192.168.4.1");
    Serial.println("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
  }
  else {
    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    if (config.ip.enable){
      // manual IP-Settings
      IPAddress manIp;
      manIp.fromString(config.ip.ipaddress);
      IPAddress manSubnet;
      manSubnet.fromString(config.ip.subnet);
      IPAddress manGateway;
      manGateway.fromString(config.ip.gateway);
      IPAddress manDns;
      manDns.fromString(config.ip.dns);
      WiFi.config(manIp, manGateway, manSubnet, manDns);
    }

    // connect to configured wifi AP
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    WiFi.hostname(config.wifi.hostname);
    MDNS.begin(config.wifi.hostname);
    Serial.print("WiFi Mode STA - Trying connect to: ");
    Serial.println(config.wifi.ssid);
  }
  
}


/**
 * *******************************************************************
 * @brief   Call all basic Setup functions at once
 * @param   none
 * @return  none
 * *******************************************************************/
void basicSetup() {
    // WiFi
    setupWiFi();
    
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
 * @param   value as int8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* int8ToString(int8_t value){
  static char ret_str[5];
  snprintf(ret_str, sizeof(ret_str), "%i", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint8ToString(uint8_t value){
  static char ret_str[5];
  snprintf(ret_str, sizeof(ret_str), "%u", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint16_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint16ToString(uint16_t value){
  static char ret_str[10];
  snprintf(ret_str, sizeof(ret_str), "%u", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @details because the format specifier %llu is not supported by any 
 *          platforms, we need to convert it this way
 * @param   value as uint64_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint64ToString(uint64_t value){
  static char ret_str[21];
  char* ptr = ret_str;
  int num_digits = 0;
  do {
    *ptr++ = (value % 10) + '0';
    value /= 10;
    num_digits++;
  } while (value != 0);
  *ptr-- = '\0';
  char* start_ptr = ret_str;
  char tmp_char;
  while (start_ptr < ptr) {
    tmp_char = *start_ptr;
    *start_ptr++ = *ptr;
    *ptr-- = tmp_char;
  }
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
 * @brief   create String from integer - with 3 digits
 * @param   value as float
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* floatToString4(float value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.4f", value);
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

/**
 * *******************************************************************
 * @brief   safe strcat function
 * @param   dest string destination
 * @param   src string source
 * @param   dest_size destination size
 * @return  none
 * *******************************************************************/
char *strcat_safe(char *dest, const char *src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        // not enough space
        return NULL;
    }   
    strcat(dest, src);
    return dest;
}