#pragma once

#include <Arduino.h>

// Keep mqttBasic.cpp/.h identical in Buderus and Jarolift. Device-specific
// subscriptions, discovery and commands belong in mqttCustom.cpp.
constexpr size_t MQTT_PAYLOAD_LEN = 512;
constexpr size_t MQTT_TOPIC_LEN = 512;

struct s_MqttMessage {
  char topic[MQTT_TOPIC_LEN];
  char payload[MQTT_PAYLOAD_LEN];
  int len;
};

struct s_cfg_mqtt;
const s_cfg_mqtt &mqttGetConnectionConfig();
const char *addTopic(const char *suffix);
const char *addCfgCmdTopic(const char *suffix);
void mqttSetup();
void mqttCyclic();
void mqttReconnect();
bool mqttIsReconnecting();
bool mqttIsConnecting();
bool mqttIsConnected();
const char *mqttGetLastError();
void mqttPublish(const char *topic, const char *payload, boolean retained);
void mqttSubscribe(const char *topic);
