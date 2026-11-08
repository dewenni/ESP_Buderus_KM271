#pragma once

struct s_wifi {
  bool connected = false;
  long rssi = 0;
  int signal = 0;
  char ipAddress[20] = {0};
};
extern s_wifi wifi;

// Connection setup, cyclic processing and status.
void setupWiFi();
void wifiCyclic();
bool wifiIsConnected();
void sendWiFiInfo();

// BSSID settings and asynchronous WLAN searches.
bool setWiFiBssid(const char *value);
void requestWiFiApScan(const char *ssid);
void requestWiFiSsidScan();
