#pragma once
#include <Credentials.h>

#define USE_OILMETER                // disable if you dont use the oilmeter
// #define DEBUG_ON                 // enable debug messages

#define USE_HC1                     // Use HeatingCircuit_1
#define USE_HC2                     // Use HeatingCircuit_2

#define HOSTNAME "ESP_Buderus_KM271"
#define MQTT_TOPIC "esp_heizung"

#define WIFI_RECONNECT      5000    // Delay between wifi reconnection tries
#define MQTT_RECONNECT      5000    // Delay between mqtt reconnection tries
