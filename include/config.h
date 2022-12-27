#pragma once
#include <Credentials.h>

/*--------------------------------------------------------------------------------
Introduction:
you can enable/disable different options/configurations using the different #defines

Example:
#define USE_OILMETER  => will activate the oilcounter function
//#define USE_OILMETER  => will disable the oilcounter function

----------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION             v1.3.2      // internal program version
#define DEBUG_ON                        // enable debug messages
#define LANG                0           // 0=GERMAN / 1=ENGLISH

#define HOSTNAME "ESP_Buderus_KM271"    // WiFi Hostname
#define MQTT_TOPIC "esp_heizung"        // MQTT Topic Prefix

#define WIFI_RECONNECT      5000        // Delay between wifi reconnection tries
#define MQTT_RECONNECT      5000        // Delay between mqtt reconnection tries

/*--------------------------------------------------------------------------------
Feature Configuration
--------------------------------------------------------------------------------*/
#define USE_OILMETER                    // disable if you dont use the oilmeter

/*--------------------------------------------------------------------------------
Logamatic Configuration
--------------------------------------------------------------------------------*/
#define USE_HC1                         // Use HeatingCircuit_1
#define USE_HC2                         // Use HeatingCircuit_2

