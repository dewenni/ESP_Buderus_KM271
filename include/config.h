#pragma once
#include <Credentials.h>

/*--------------------------------------------------------------------------------
Introduction:
you can enable/disable different options/configurations using the different #defines

Example:
#define USE_OILMETER  => will activate the oilcounter function
//#define USE_OILMETER  => will disable the oilcounter function

/*-------------------------------------------------------------------------------
Hardware Configuration
--------------------------------------------------------------------------------*/
#define LED_WIFI        21      // LED for WiFi Status
#define LED_HEARBEAT    22      // LED for heartbeat
#define LED_LOGMODE     23      // LED for LogMode

#define RXD2   4                // ESP32 RX-pin for KM271 communication, align with hardware
#define TXD2   2                // ESP32 TX-pin for KM271 communication, align with hardware

/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION             "v2.2.0"    // internal program version
#define DEBUG_ON                        // enable debug messages
#define LANG                0           // 0=GERMAN / 1=ENGLISH

#define HOSTNAME "ESP_Buderus_KM271"    // WiFi Hostname
#define MQTT_TOPIC "esp_heizung"        // MQTT Topic Prefix

#define WIFI_RECONNECT      10000        // Delay between wifi reconnection tries
#define MQTT_RECONNECT      10000        // Delay between mqtt reconnection tries

/*--------------------------------------------------------------------------------
Feature Configuration
--------------------------------------------------------------------------------*/
#define USE_OILMETER                    // disable if you dont use the oilmeter
#define USE_WEBUI                       // disable WebUI if you dont want to use it

/*--------------------------------------------------------------------------------
Logamatic Configuration
--------------------------------------------------------------------------------*/
#define USE_HC1                                 // Use HeatingCircuit_1
//#define USE_HC2                                 // Use HeatingCircuit_2

#define USE_ALARM_MSG                           // activates the use of Messages based on 0x0300, 0x0307, 0x030e, 0x0315 that are not supportet for every Logamatic models (e.g. HS 2105M)

#define USE_NTP                                 // Use NTP (Network-Time-Protocol) to set date and time of Logamatic
#define NTP_SERVER "de.pool.ntp.org"            // NTP Server
#define NTP_TZ "CET-1CEST,M3.5.0,M10.5.0/3 "    // NTP TimeZone

/*--------------------------------------------------------------------------------
Optional: calculation of oil consumption based on burner runtime
--------------------------------------------------------------------------------*/
//#define USE_CALCULATED_CONSUMPTION      // use calculated oil consumption
                                        // calculation: Consumption_Litre = Runtime_Minutes / 60 * CFG_CONSUMPTION_KG_H / CFG_OIL_DENSITY_KG_L

#define CFG_CONSUMPTION_KG_H 2.0        // oil consumption in "kilograms per hour" - see documentation of your heater
#define CFG_OIL_DENSITY_KG_L 0.85       // densitiy of oil in "kilograms per Litre"

