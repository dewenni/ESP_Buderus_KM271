#pragma once

// ======================================================
// INCLUDES
// ======================================================

// include intern
#include <config.h>
#include <mqtt.h>

// include extern
#include <Arduino.h>
#include <SPI.h>
#include <ArduinoOTA.h> 
#include <ArduinoJSON.h>
#include <muTimer.h>

/* Configuration of NTP */
#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3 " 

void ntpSetup();
void setupOTA();
void setup_wifi();
void check_wifi();
void basic_setup();
void sendWiFiInfo();
const char * getDateTimeString();
void storeData();
