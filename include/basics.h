#pragma once

/* I N C L U D E S ****************************************************/  

// include intern
#include <config.h>
#include <mqtt.h>
#include <message.h>

// include extern
#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <muTimer.h>
#include "esp_system.h"
#include <HTTPClient.h>
#include "stringHelper.h"

/* D E C L A R A T I O N S ****************************************************/  
typedef struct {                                        
  long rssi;
  int signal;
  char ipAddress[20];
} s_wifi;
extern s_wifi wifi;


/* P R O T O T Y P E S ********************************************************/ 
void checkWiFi();
void basicSetup();
void sendWiFiInfo();
void storeData();
void getUptime(char *buffer, size_t bufferSize);