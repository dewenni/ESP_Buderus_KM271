#include <ESPmDNS.h>
#include <WiFi.h>
#include <algorithm>
#include <basics.h>
#include <esp_wifi.h>
#include <webUI.h>
#include <WiFiMgn.h>

s_wifi wifi; // global WiFi information

static muTimer wifiReconnectTimer;
static int wifi_retry = 0;
static const char *TAG = "WIFI";
static auto &ota = EspSysUtil::OTA::getInstance();

static bool scanRunning = false;
static bool scanAllSsids = false;
static unsigned long scanStarted = 0;
static wifi_mode_t previousMode = WIFI_OFF;
static char scanSsid[sizeof(config.wifi.ssid)] = {};
static constexpr unsigned long SCAN_TIMEOUT_MS = 20000;
static constexpr int MAX_SCAN_RESULTS = 32;

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected to configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
static void onWiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi_retry = 0;
  ESP_LOGI(TAG, "Connected to AP successfully!");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is disconnected from configured AP
 * @param   none
 * @return  none
 * *******************************************************************/
static void onWiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  wifi.connected = false;
  ESP_LOGI(TAG, "WiFi-Disconnected");
}

/**
 * *******************************************************************
 * @brief   callback function if WiFi is connected and client got IP
 * @param   none
 * @return  none
 * *******************************************************************/
static void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  ESP_LOGI(TAG, "WiFi connected");
  snprintf(wifi.ipAddress, sizeof(wifi.ipAddress), "%s", WiFi.localIP().toString().c_str());
  ESP_LOGI(TAG, "IP address: %s", wifi.ipAddress);
  uint8_t *bssid = WiFi.BSSID();
  ESP_LOGI(TAG, "BSSID: %02X:%02X:%02X:%02X:%02X:%02X, Ch: %d, RSSI: %d dBm",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
           (int)WiFi.channel(), (int)WiFi.RSSI());
  wifi.connected = true;
}

/**
 * *******************************************************************
 * @brief   Parse and validate a BSSID string into a byte array
 * @param   bssidStr  MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param   bssid     output byte array (6 bytes)
 * @return  true if valid, false otherwise
 * *******************************************************************/
static bool parseBSSID(const char *bssidStr, uint8_t *bssid) {
  if (strlen(bssidStr) != 17) return false;
  for (int i = 0; i < 17; ++i) {
    if (i % 3 == 2 ? bssidStr[i] != ':' : !isxdigit(static_cast<unsigned char>(bssidStr[i]))) {
      return false;
    }
  }
  int pos = 0;
  int matched = sscanf(bssidStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx%n",
                       &bssid[0], &bssid[1], &bssid[2], &bssid[3], &bssid[4], &bssid[5], &pos);
  return matched == 6 && pos == 17;
}

bool setWiFiBssid(const char *value) {
  uint8_t bssid[6];
  if (value[0] != '\0' && !parseBSSID(value, bssid)) {
    return false;
  }
  snprintf(config.wifi.bssid, sizeof(config.wifi.bssid), "%s", value);
  for (char &c : config.wifi.bssid) {
    c = toupper(static_cast<unsigned char>(c));
  }
  return true;
}

/**
 * *******************************************************************
 * @brief   Start WiFi connection, with optional BSSID locking
 * @param   none
 * @return  none
 * *******************************************************************/
static void beginWiFiConnection() {
  // Reapply these settings after the reconnect path resets the WiFi stack.
  WiFi.setSleep(WIFI_PS_NONE);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  uint8_t bssid[6];
  if (strlen(config.wifi.bssid) == 17 && parseBSSID(config.wifi.bssid, bssid)) {
    WiFi.begin(config.wifi.ssid, config.wifi.password, 0, bssid);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s (BSSID lock: %s)", config.wifi.ssid, config.wifi.bssid);
  } else {
    WiFi.begin(config.wifi.ssid, config.wifi.password);
    ESP_LOGI(TAG, "WiFi Mode STA - Trying connect to: %s", config.wifi.ssid);
  }
  WiFi.hostname(config.wifi.hostname);
  MDNS.begin(config.wifi.hostname);
}

/**
 * *******************************************************************
 * @brief   check WiFi connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
static void checkWiFi() {
  if (scanRunning) {
    return;
  }

  // Ethernet is also not connected - so we need to establish WiFi
  if (wifiReconnectTimer.delayOnTrigger((!wifi.connected && !eth.connected), WIFI_RECONNECT)) {
    wifiReconnectTimer.delayReset();

    wifi_retry++;

    // Keep retrying indefinitely instead of rebooting after a handful of
    // attempts. A router reboot, an access point being replaced or an ISP
    // outage is not this device's fault, and restarting drops the decoded
    // boiler state, both log ring buffers and the NTP sync while fixing
    // nothing - the boiler itself keeps running regardless. The old
    // behaviour turned any outage longer than 2.5 minutes into a permanent
    // reboot loop, which also made it nearly impossible to push an OTA
    // update during the outage.
    //
    // The exception is a last-resort restart: the ESP32 WiFi stack can end
    // up in a state that reconnecting alone does not clear, and by then
    // nothing is working anyway, so a reboot costs nothing and may be the
    // only way back without physical access to the device.
    if (wifi_retry >= WIFI_RETRY_REBOOT && !ota.isActive()) {
      ESP_LOGI(TAG, "no WiFi after %i attempts - rebooting as a last resort", wifi_retry);
      storeData(); // store Data before reboot
      EspSysUtil::RestartReason::saveLocal("no wifi connection");
      yield();
      delay(1000);
      yield();
      ESP.restart();
    }

    // Periodically tear the stack down completely before reconnecting; this
    // clears most wedged states without needing a restart.
    if ((wifi_retry % WIFI_RETRY_HARD_RESET) == 0) {
      ESP_LOGI(TAG, "WiFi - resetting the stack before retry %i", wifi_retry);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);
    }

    WiFi.mode(WIFI_STA);
    beginWiFiConnection();
    ESP_LOGI(TAG, " - attempt: %i", wifi_retry);
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
    beginWiFiConnection();
  }
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

static void publishScanResult(JsonDocument &result) {
  String json;
  serializeJson(result, json);
  // EspWebUI writes text updates through innerHTML. Escape data from SSIDs
  // before sending it to the hidden result element; the browser uses JSON.parse.
  json.replace("&", "\\u0026");
  json.replace("<", "\\u003c");
  json.replace(">", "\\u003e");
  webUI.wsUpdateWebText("p12_wifi_ap_results", json.c_str(), false);
}

static void publishScanStatus(const char *status, const char *ssid, bool allSsids = false) {
  JsonDocument result;
  result["status"] = status;
  result["ssid"] = ssid;
  result["scan"] = allSsids ? "ssid" : "ap";
  publishScanResult(result);
}

static void finishScan() {
  WiFi.scanDelete();
  // Scanning needs station mode. Keep a setup access point running and restore
  // AP-only/off mode afterwards if we temporarily enabled the station.
  if (!(previousMode & WIFI_STA)) {
    WiFi.mode(previousMode);
  }
  scanRunning = false;
}

static void startWiFiScan(const char *ssid, bool allSsids) {
  if (scanRunning || EspSysUtil::OTA::getInstance().isActive()) {
    publishScanStatus("busy", ssid, allSsids);
    return;
  }
  if (!allSsids && (strlen(ssid) == 0 || strlen(ssid) > 32)) {
    publishScanStatus("invalid_ssid", ssid);
    return;
  }

  snprintf(scanSsid, sizeof(scanSsid), "%s", ssid);
  scanAllSsids = allSsids;
  previousMode = WiFi.getMode();
  scanStarted = millis();
  scanRunning = true;
  WiFi.setScanTimeout(SCAN_TIMEOUT_MS);
  const int16_t state = WiFi.scanNetworks(true, true, false, 150, 0, allSsids ? nullptr : scanSsid);
  if (state == WIFI_SCAN_FAILED) {
    publishScanStatus("failed", scanSsid, scanAllSsids);
    finishScan();
  } else {
    publishScanStatus("scanning", scanSsid, scanAllSsids);
  }
}

void requestWiFiApScan(const char *ssid) { startWiFiScan(ssid, false); }

void requestWiFiSsidScan() { startWiFiScan("", true); }

static void processWiFiScan() {
  if (!scanRunning) {
    return;
  }
  if (EspSysUtil::OTA::getInstance().isActive() || millis() - scanStarted >= SCAN_TIMEOUT_MS) {
    esp_wifi_scan_stop();
    publishScanStatus("failed", scanSsid, scanAllSsids);
    finishScan();
    return;
  }

  const int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    return;
  }
  if (count < 0) {
    esp_wifi_scan_stop();
    publishScanStatus("failed", scanSsid, scanAllSsids);
    finishScan();
    return;
  }

  JsonDocument result;
  result["status"] = "done";
  result["ssid"] = scanSsid;
  result["scan"] = scanAllSsids ? "ssid" : "ap";
  JsonArray accessPoints = result["aps"].to<JsonArray>();
  const String currentBssid = WiFi.isConnected() ? WiFi.BSSIDstr() : String();
  const String currentSsid = WiFi.isConnected() ? WiFi.SSID() : String();
  uint8_t indices[256];
  const int resultCount = std::min<int>(count, 256);
  for (int i = 0; i < resultCount; ++i) indices[i] = i;
  std::sort(indices, indices + resultCount, [](uint8_t a, uint8_t b) { return WiFi.RSSI(a) > WiFi.RSSI(b); });
  char returnedSsids[MAX_SCAN_RESULTS][33] = {};
  int ssidCount = 0;
  // Arduino's scan result accessors use uint8_t indices.
  for (int n = 0; n < resultCount && accessPoints.size() < MAX_SCAN_RESULTS; ++n) {
    const uint8_t i = indices[n];
    const String ssid = WiFi.SSID(i);
    if (scanAllSsids) {
      // Hidden SSIDs have no selectable name. Keep manual entry available.
      if (ssid.isEmpty()) continue;
      bool duplicate = false;
      for (int j = 0; j < ssidCount; ++j) {
        if (ssid == returnedSsids[j]) duplicate = true;
      }
      if (duplicate) continue;
      snprintf(returnedSsids[ssidCount++], sizeof(returnedSsids[0]), "%s", ssid.c_str());
    } else if (ssid != scanSsid) {
      continue;
    }
    JsonObject ap = accessPoints.add<JsonObject>();
    const String bssid = WiFi.BSSIDstr(i);
    ap["bssid"] = bssid;
    if (scanAllSsids) ap["ssid"] = ssid;
    ap["rssi"] = WiFi.RSSI(i);
    ap["channel"] = WiFi.channel(i);
    ap["current"] = scanAllSsids ? ssid == currentSsid : bssid.equalsIgnoreCase(currentBssid);
  }
  publishScanResult(result);
  finishScan();
}

// Run in setup mode as well, so the AP and SSID search also works there.
void wifiCyclic() {
  processWiFiScan();
  if (!setupMode) {
    checkWiFi();
  }
}

bool wifiIsConnected() { return WiFi.isConnected(); }
