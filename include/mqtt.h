#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>

// MQTT
#include <PubSubClient.h>


// ======================================================
// Prototypes
// ======================================================
const char * addTopic(const char *suffix);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqttCyclic();
void mqttSetup();
void mqtt_reconnect();
void mqttPublish(const char* sendtopic, const char* payload, boolean retained);
