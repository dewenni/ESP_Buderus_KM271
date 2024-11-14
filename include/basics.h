#pragma once

/* I N C L U D E S ****************************************************/

// include intern
#include <config.h>
#include <message.h>
#include <mqtt.h>

// include extern
#include "esp_system.h"
#include "stringHelper.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <muTimer.h>
#include <stdint.h>
#include <stdlib.h>

/* D E C L A R A T I O N S ****************************************************/
struct s_wifi {
  bool connected = false;
  long rssi = 0;
  int signal = 0;
  char ipAddress[20] = {0};
};
extern s_wifi wifi;

struct s_eth {
  bool connected = false;
  char ipAddress[20];
  uint8_t linkSpeed;
  bool fullDuplex;
  bool linkUp;
};
extern s_eth eth;

/* P R O T O T Y P E S ********************************************************/
void checkWiFi();
void basicSetup();
void sendWiFiInfo();
void sendETHInfo();
void storeData();
void getUptime(char *buffer, size_t bufferSize);
void sendSysInfo();
void setupETH();