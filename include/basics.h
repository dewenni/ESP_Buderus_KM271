#pragma once

/* I N C L U D E S ****************************************************/  

// include intern
#include <config.h>
#include <mqtt.h>

// include extern
#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <muTimer.h>
#include "esp_system.h"

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
const char * getDateTimeString();
const char * getDateString();
const char * getTimeString();
const char* int8ToString(int8_t value);
const char* uint8ToString(uint8_t value);
const char *uint16ToString(uint16_t value);
const char *uint64ToString(uint64_t value);
const char* floatToString(float value);
const char* floatToString4(float value);
const char* doubleToString(double value);
char *strcat_safe(char *dest, const char *src, size_t dest_size);
void getRestartReason(char *reason, size_t reason_size);
void getUptime(char *buffer, size_t bufferSize);