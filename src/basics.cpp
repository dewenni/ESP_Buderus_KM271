#include <basics.h>

/* P R O T O T Y P E S ********************************************************/
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/
s_wifi wifi;                            // global WiFi Informations
muTimer wifiReconnectTimer = muTimer(); // timer for reconnect delay
int wifi_retry = 0;
esp_reset_reason_t reset_reason;
char intRestartReason[64];

/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup() {
#ifdef ARDUINO_ARCH_ESP32
  // ESP32 seems to be a little more complex:
  configTime(0, 0,
             config.ntp.server); // 0, 0 because we will use TZ in the next line
  setenv("TZ", config.ntp.tz,
         1); // Set environment variable with your time zone
  tzset();
#else
  // ESP8266
  configTime(NTP_TZ, NTP_SERVER); // --> for the ESP8266 only
#endif
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_retry = 0;
  msgLn("Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  msgLn("WiFi connected");
  msgLn("IP address: ");
  msgLn(WiFi.localIP().toString().c_str());
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void checkWiFi() {
  if (wifiReconnectTimer.delayOnTrigger(!WiFi.isConnected(), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();
    if (wifi_retry < 5) {
      wifi_retry++;
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.wifi.ssid, config.wifi.password);
      WiFi.hostname(config.wifi.hostname);
      MDNS.begin(config.wifi.hostname);
      msg("WiFi Mode STA - Trying connect to: ");
      msg(config.wifi.ssid);
      msg(" - attempt: ");
      msg(int8ToString(wifi_retry));
      msgLn("/5");
    } else {
      msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
      msgLn("Wifi connection not possible, esp rebooting...");
      msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
      storeData(); // store Data before reboot
      saveRestartReason("no wifi connection");
      yield();
      delay(1000);
      yield();
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
    IPAddress ip(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, subnet);
    WiFi.softAP("ESP-Buderus-km271");
    msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
    msgLn("> WiFi Mode: AccessPoint <");
    msgLn("1. connect your device to SSID: ESP-Buderus-km271");
    msgLn("2. open Browser and go to Address: http://192.168.4.1");
    msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
  } else {
    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    if (config.ip.enable) {
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
    msg("WiFi Mode STA - Trying connect to: ");
    msgLn(config.wifi.ssid);
  }
}

/**
 * *******************************************************************
 * @brief   Call all basic Setup functions at once
 * @param   none
 * @return  none
 * *******************************************************************/
void basicSetup() {

  // check internal restart reason
  readRestartReason(intRestartReason, sizeof(intRestartReason));

  // WiFi
  setupWiFi();

  // NTP
  ntpSetup();

  // Bestimme den Grund für den letzten Neustart
  reset_reason = esp_reset_reason();
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

  // dBm to Quality:
  if (wifi.rssi <= -100)
    wifi.signal = 0;
  else if (wifi.rssi >= -50)
    wifi.signal = 100;
  else
    wifi.signal = 2 * (wifi.rssi + 100);

  IPAddress localIP = WiFi.localIP();
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);
  JsonDocument wifiJSON;
  wifiJSON["status"] = "online";
  wifiJSON["rssi"] = wifi.rssi;
  wifiJSON["signal"] = wifi.signal;
  wifiJSON["ip"] = wifi.ipAddress;
  wifiJSON["date-time"] = getDateTimeString();

  char sendWififJSON[255];
  serializeJson(wifiJSON, sendWififJSON);
  mqttPublish(addTopic("/wifi"), sendWififJSON, false);

  // wifi status
  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   build sysinfo structure and send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendSysInfo() {

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  char restartReason[64];
  getRestartReason(restartReason, sizeof(restartReason));
  // ESP Heap and Flash usage
  char heap[10];
  snprintf(heap, sizeof(heap), "%.1f %%", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize());
  char flash[10];
  snprintf(flash, sizeof(flash), "%.1f %%", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace());

  JsonDocument sysInfoJSON;
  sysInfoJSON["uptime"] = uptimeStr;
  sysInfoJSON["restart_reason"] = restartReason;
  sysInfoJSON["heap"] = heap;
  sysInfoJSON["flash"] = flash;
  char sendInfoJSON[255] = {'\0'};
  serializeJson(sysInfoJSON, sendInfoJSON);
  mqttPublish(addTopic("/sysinfo"), sendInfoJSON, false);
}

/**
 * *******************************************************************
 * @brief   generate uptime message
 * @param   buffer
 * @param   bufferSize
 * @return  none
 * *******************************************************************/
void getUptime(char *buffer, size_t bufferSize) {
  static unsigned long previousMillis = 0;
  static unsigned long overflowCounter = 0;
  const unsigned long overflowThreshold = 4294967295; // Maximalwert von millis() bevor es zum Überlauf kommt

  unsigned long currentMillis = millis();
  if (currentMillis < previousMillis) {
    // Ein Überlauf wurde detektiert
    overflowCounter++;
  }
  previousMillis = currentMillis;

  // Berechne die gesamte Uptime in Sekunden unter Berücksichtigung des
  // Überlaufs
  unsigned long long totalSeconds = overflowCounter * (overflowThreshold / 1000ULL) + (currentMillis / 1000ULL);

  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  // Verwende snprintf zum Formatieren der Ausgabe
  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02luh %02lum %02lus", days, hours, minutes, seconds);
  } else if (hours > 0) {
    snprintf(buffer, bufferSize, "%02luh %02lum %02lus", hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%lum %lus", minutes, seconds);
  }
}

/**
 * *******************************************************************
 * @brief   get last restart reason as string
 * @param   buffer
 * @param   bufferSize
 * @return  none
 * *******************************************************************/
void getRestartReason(char *reason, size_t reason_size) {
  
  if (strlen(intRestartReason)!=0) {
    snprintf(reason, reason_size, "%s", intRestartReason);
  } else {
    switch (reset_reason) {
    case ESP_RST_POWERON:
      strncpy(reason, "Power-on reset", reason_size);
      break;
    case ESP_RST_EXT:
      strncpy(reason, "External reset", reason_size);
      break;
    case ESP_RST_SW:
      strncpy(reason, "Software reset via esp_restart", reason_size);
      break;
    case ESP_RST_PANIC:
      strncpy(reason, "Software panic reset", reason_size);
      break;
    case ESP_RST_INT_WDT:
      strncpy(reason, "Interrupt watchdog reset", reason_size);
      break;
    case ESP_RST_TASK_WDT:
      strncpy(reason, "Task watchdog reset", reason_size);
      break;
    case ESP_RST_WDT:
      strncpy(reason, "Other watchdog reset", reason_size);
      break;
    case ESP_RST_DEEPSLEEP:
      strncpy(reason, "Woke up from deep-sleep", reason_size);
      break;
    case ESP_RST_BROWNOUT:
      strncpy(reason, "Brownout reset (voltage dip)", reason_size);
      break;
    case ESP_RST_SDIO:
      strncpy(reason, "Reset over SDIO", reason_size);
      break;
    default:
      strncpy(reason, "unknown Reset", reason_size);
      break;
    }
  }
}