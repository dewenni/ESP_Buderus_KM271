#include <ETH.h>
#include <SPI.h>
#include <basics.h>

#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1
#endif

/* P R O T O T Y P E S ********************************************************/
void ntpSetup();
void setupOTA();
void setup_wifi();

/* D E C L A R A T I O N S ****************************************************/
s_wifi wifi; // global WiFi Informations
s_eth eth;   // global ETH Informations

static muTimer wifiReconnectTimer = muTimer(); // timer for reconnect delay
static int wifi_retry = 0;
static esp_reset_reason_t reset_reason;
static char intRestartReason[64];
static const char *TAG = "SETUP"; // LOG TAG

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
  MY_LOGI(TAG, "Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is disconnected from configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi.connected = false;
  MY_LOGI(TAG, "WiFi-Disconnected");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  MY_LOGI(TAG, "WiFi connected");
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%s", WiFi.localIP().toString().c_str());
  MY_LOGI(TAG, "IP address: %s", wifi.ipAddress);
  wifi.connected = true;
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void checkWiFi() {

  // Ethernet is also not connected - so we need to establish WiFi
  if (wifiReconnectTimer.delayOnTrigger((!wifi.connected && !eth.connected), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();

    if (wifi_retry < 5) {
      wifi_retry++;
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.wifi.ssid, config.wifi.password);
      WiFi.hostname(config.wifi.hostname);
      MDNS.begin(config.wifi.hostname);
      MY_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
      MY_LOGI(TAG, " - attempt: %i/5", wifi_retry);
    } else {
      MY_LOGI(TAG, "\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
      MY_LOGI(TAG, "Wifi connection not possible, esp rebooting...");
      MY_LOGI(TAG, "\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
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
    MY_LOGI(TAG, "\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
    MY_LOGI(TAG, "> WiFi Mode: AccessPoint <");
    MY_LOGI(TAG, "1. connect your device to SSID: ESP-Buderus-km271");
    MY_LOGI(TAG, "2. open Browser and go to Address: http://192.168.4.1");
    MY_LOGI(TAG, "\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
  } else {
    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    if (config.wifi.static_ip) {
      // manual IP-Settings
      IPAddress manIp;
      manIp.fromString(config.wifi.ipaddress);
      IPAddress manSubnet;
      manSubnet.fromString(config.wifi.subnet);
      IPAddress manGateway;
      manGateway.fromString(config.wifi.gateway);
      IPAddress manDns;
      manDns.fromString(config.wifi.dns);
      WiFi.config(manIp, manGateway, manSubnet, manDns);
    }

    // connect to configured wifi AP
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    WiFi.hostname(config.wifi.hostname);
    MDNS.begin(config.wifi.hostname);
    MY_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
  }
}

void onEthEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    MY_LOGI(TAG, "ETH Started");
    // set eth hostname here
    ETH.setHostname(config.eth.hostname);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    MY_LOGI(TAG, "ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    eth.connected = true;
    snprintf(eth.ipAddress, sizeof(eth.ipAddress), "%s", ETH.localIP().toString().c_str());
    MY_LOGI(TAG, "ETH Got IP: '%s'\n", eth.ipAddress);
    break;
  case ARDUINO_EVENT_ETH_LOST_IP:
    MY_LOGI(TAG, "ETH Lost IP");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    MY_LOGI(TAG, "ETH Disconnected");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    MY_LOGI(TAG, "ETH Stopped");
    eth.connected = false;
    break;
  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   Setup for general Ethernet Function
 * @param   none
 * @return  none
 * *******************************************************************/
void setupETH() {

  pinMode(config.eth.gpio_irq, OUTPUT);
  pinMode(config.eth.gpio_rst, OUTPUT);

  Network.onEvent(onEthEvent);

  SPI.setFrequency(SPI_MASTER_FREQ_8M);
  SPI.begin(config.eth.gpio_sck, config.eth.gpio_miso, config.eth.gpio_mosi);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, config.eth.gpio_cs, config.eth.gpio_irq, config.eth.gpio_rst, SPI);

  if (config.eth.static_ip) {
    ETH.config(IPAddress(config.eth.ipaddress), IPAddress(config.eth.gateway), IPAddress(config.eth.subnet), IPAddress(config.eth.dns));
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

  // Ethernet
  if (config.eth.enable) {
    setupETH();
  }

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

  JsonDocument wifiJSON;

  if (wifi.connected) {
    // check  RSSI
    wifi.rssi = WiFi.RSSI();

    // dBm to Quality:
    if (wifi.rssi <= -100)
      wifi.signal = 0;
    else if (wifi.rssi >= -50)
      wifi.signal = 100;
    else
      wifi.signal = 2 * (wifi.rssi + 100);

    wifiJSON["status"] = wifi.connected ? "connected" : "disconnected";
    wifiJSON["rssi"] = wifi.rssi;
    wifiJSON["signal"] = wifi.signal;
    wifiJSON["ip"] = wifi.ipAddress;
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  } else {
    wifiJSON["status"] = "disconnected";
    wifiJSON["rssi"] = "--";
    wifiJSON["signal"] = "--";
    wifiJSON["ip"] = "--";
    wifiJSON["date_time"] = EspStrUtil::getDateTimeString();
  }

  char sendWififJSON[255];
  serializeJson(wifiJSON, sendWififJSON);
  mqttPublish(addTopic("/wifi"), sendWififJSON, false);

  // wifi status
  mqttPublish(addTopic("/status"), "online", false);
}

/**
 * *******************************************************************
 * @brief   Send WiFi Information in JSON format via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendETHInfo() {
  ETH.desc();

  eth.fullDuplex = ETH.fullDuplex();
  eth.linkUp = ETH.linkUp();
  eth.linkSpeed = ETH.linkSpeed();

  JsonDocument ethJSON;
  ethJSON["ip"] = eth.ipAddress;
  ethJSON["status"] = eth.connected ? "connected" : "disconnected";
  ethJSON["link_up"] = eth.linkUp ? "active" : "inactive";
  ethJSON["link_speed"] = eth.linkSpeed;
  ethJSON["full_duplex"] = eth.linkUp ? "full-duplex" : "---";
  ethJSON["date_time"] = EspStrUtil::getDateTimeString();

  char sendEthJSON[255];
  serializeJson(ethJSON, sendEthJSON);
  mqttPublish(addTopic("/eth"), sendEthJSON, false);

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
  // ESP Heap and Flash usage
  char heap[10];
  snprintf(heap, sizeof(heap), "%.1f %%", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize());
  char flash[10];
  snprintf(flash, sizeof(flash), "%.1f %%", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace());

  JsonDocument sysInfoJSON;
  sysInfoJSON["uptime"] = uptimeStr;
  sysInfoJSON["restart_reason"] = EspSysUtil::RestartReason::get();
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

  if (strlen(intRestartReason) != 0) {
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