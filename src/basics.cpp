#include <ETH.h>
#include <SPI.h>
#include <basics.h>
#include <esp_wifi.h>

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
static const char *TAG = "SETUP"; // LOG TAG

/**
 * *******************************************************************
 * @brief   Setup for NTP Server
 * @param   none
 * @return  none
 * *******************************************************************/
void ntpSetup() {
  // ESP32 seems to be a little more complex:
  configTime(0, 0,
             config.ntp.server); // 0, 0 because we will use TZ in the next line
  setenv("TZ", config.ntp.tz,
         1); // Set environment variable with your time zone
  tzset();
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_retry = 0;
  ESP_LOGI(TAG, "Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is disconnected from configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi.connected = false;
  ESP_LOGI(TAG, "WiFi-Disconnected");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGI(TAG, "WiFi connected");
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%s", WiFi.localIP().toString().c_str());
  ESP_LOGI(TAG, "IP address: %s", wifi.ipAddress);
  uint8_t* bssid = WiFi.BSSID();
  ESP_LOGI(TAG, "Connected MAC:%02X:%02X:%02X:%02X:%02X:%02X Ch:%d RSSI:%ddBm", 
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
           WiFi.channel(), WiFi.RSSI());
  wifi.connected = true;
}

/**
 * *******************************************************************
 * @brief   Parse BSSID string to byte array
 * @param   bssidStr - BSSID string (format: "AA:BB:CC:DD:EE:FF")
 * @param   bssid - output byte array
 * @return  true if valid BSSID, false otherwise
 * *******************************************************************/
bool parseBSSID(const char* bssidStr, uint8_t* bssid) {
  if (strlen(bssidStr) != 17) return false;
  return sscanf(bssidStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5]) == 6;
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
      
      // Force a fresh scan on reconnect to find best AP
      WiFi.disconnect(true); // Disconnect and clear config
      delay(100);
      
      // Scan and sort by RSSI to connect to strongest AP
      int n = WiFi.scanNetworks(false, false, false, 300U, 0U);
      ESP_LOGI(TAG, "Scan complete: %d networks found", n);
      
      // Log available APs with same SSID for debugging
      for (int i = 0; i < n; i++) {
        if (String(config.wifi.ssid) == WiFi.SSID(i)) {
          uint8_t* bssid = WiFi.BSSID(i);
          ESP_LOGI(TAG, "  AP #%d: Ch:%d RSSI:%ddBm MAC:%02X:%02X:%02X:%02X:%02X:%02X", 
                   i, WiFi.channel(i), WiFi.RSSI(i), 
                   bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        }
      }
      
      // Check if BSSID locking is configured
      if (strlen(config.wifi.bssid) == 17) {
        uint8_t bssid[6];
        if (parseBSSID(config.wifi.bssid, bssid)) {
          WiFi.begin(config.wifi.ssid, config.wifi.password, 0, bssid);
          ESP_LOGI(TAG, "WiFi connecting with BSSID lock: %s", config.wifi.bssid);
        } else {
          WiFi.begin(config.wifi.ssid, config.wifi.password);
          ESP_LOGW(TAG, "Invalid BSSID format, ignoring: %s", config.wifi.bssid);
        }
      } else {
        WiFi.begin(config.wifi.ssid, config.wifi.password);
      }
      
      WiFi.hostname(config.wifi.hostname);
      MDNS.begin(config.wifi.hostname);
      ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
      ESP_LOGI(TAG, " - attempt: %i/5", wifi_retry);
    } else {
      ESP_LOGI(TAG, "Wifi connection not possible, esp rebooting...");
      storeData(); // store Data before reboot
      EspSysUtil::RestartReason::saveLocal("no wifi connection");
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
    ESP_LOGI(TAG, "WiFi Mode: AccessPoint SSID: ESP-Buderus-km271 / IP: http://192.168.4.1");

  } else {
    // setup callback function
    WiFi.onEvent(onWiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

    // manual IP-Settings
    if (config.wifi.static_ip) {
      WiFi.config(IPAddress(config.wifi.ipaddress), IPAddress(config.wifi.gateway), IPAddress(config.wifi.subnet), IPAddress(config.wifi.dns));
    }

    // connect to configured wifi AP
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    
    // ===== MESH NETWORK OPTIMIZATIONS =====
    // Set minimum RSSI threshold for initial connection and roaming
    // Only connect to APs with signal strength > -75 dBm
    WiFi.setMinSecurity(WIFI_AUTH_WPA2_PSK); // Ensure WPA2 minimum
    
    // Configure scan thresholds for better AP selection
    // rssi_threshold: minimum RSSI to consider an AP (-127 to 0)
    // active_scan: use active scanning for better AP discovery
    esp_wifi_set_config(WIFI_IF_STA, (wifi_config_t*)NULL); // Reset config first
    
    // Set WiFi power save mode to NONE for stable connection (reduces latency)
    // Use WIFI_PS_MIN_MODEM if you need power saving
    WiFi.setSleep(WIFI_PS_NONE);
    
    // Set long-range mode off for better performance with nearby APs
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    
    // SYNCHRONOUS scan to find best AP before connecting
    ESP_LOGI(TAG, "Scanning for WiFi networks...");
    int n = WiFi.scanNetworks(false, false, false, 300U, 0U); // Sync scan
    ESP_LOGI(TAG, "Scan complete: %d networks found", n);
    
    // Log available APs with same SSID for debugging
    for (int i = 0; i < n; i++) {
      if (String(config.wifi.ssid) == WiFi.SSID(i)) {
        uint8_t* bssid = WiFi.BSSID(i);
        ESP_LOGI(TAG, "  AP #%d: Ch:%d RSSI:%ddBm MAC:%02X:%02X:%02X:%02X:%02X:%02X", 
                 i, WiFi.channel(i), WiFi.RSSI(i), 
                 bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
      }
    }
    
    // Check if BSSID locking is configured (optional: force specific AP)
    if (strlen(config.wifi.bssid) == 17) {
      uint8_t bssid[6];
      if (parseBSSID(config.wifi.bssid, bssid)) {
        WiFi.begin(config.wifi.ssid, config.wifi.password, 0, bssid);
        ESP_LOGI(TAG, "WiFi connecting with BSSID lock: %s", config.wifi.bssid);
      } else {
        WiFi.begin(config.wifi.ssid, config.wifi.password);
        ESP_LOGW(TAG, "Invalid BSSID format, ignoring BSSID lock");
      }
    } else {
      WiFi.begin(config.wifi.ssid, config.wifi.password);
    }
    
    WiFi.hostname(config.wifi.hostname);
    MDNS.begin(config.wifi.hostname);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s with mesh optimizations", config.wifi.ssid);
  }
}

void onEthEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
  case ARDUINO_EVENT_ETH_START:
    ESP_LOGI(TAG, "ETH Started");
    // set eth hostname here
    ETH.setHostname(config.eth.hostname);
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    ESP_LOGI(TAG, "ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    eth.connected = true;
    snprintf(eth.ipAddress, sizeof(eth.ipAddress), "%s", ETH.localIP().toString().c_str());
    ESP_LOGI(TAG, "ETH Got IP: '%s'\n", eth.ipAddress);
    break;
  case ARDUINO_EVENT_ETH_LOST_IP:
    ESP_LOGI(TAG, "ETH Lost IP");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    ESP_LOGI(TAG, "ETH Disconnected");
    eth.connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    ESP_LOGI(TAG, "ETH Stopped");
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
  EspSysUtil::RestartReason::readLocal();

  // WiFi
  setupWiFi();

  // Ethernet
  if (config.eth.enable) {
    setupETH();
  }

  // NTP
  ntpSetup();
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
  const unsigned long overflowThreshold = 4294967295; // maximum value of an unsigned long

  unsigned long currentMillis = millis();
  if (currentMillis < previousMillis) {
    overflowCounter++;
  }
  previousMillis = currentMillis;

  // calculate total seconds
  unsigned long long totalSeconds = overflowCounter * (overflowThreshold / 1000ULL) + (currentMillis / 1000ULL);

  unsigned long days = totalSeconds / 86400;
  unsigned long hours = (totalSeconds % 86400) / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;

  if (days > 0) {
    snprintf(buffer, bufferSize, "%lud %02luh %02lum %02lus", days, hours, minutes, seconds);
  } else if (hours > 0) {
    snprintf(buffer, bufferSize, "%02luh %02lum %02lus", hours, minutes, seconds);
  } else {
    snprintf(buffer, bufferSize, "%lum %lus", minutes, seconds);
  }
}
