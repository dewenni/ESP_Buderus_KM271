#pragma once

#include <mqttBasic.h>

// Called by mqttBasic on the loop task, never from the AsyncTCP callback.
void mqttCustomConnected();
void mqttCustomCyclic();
void processMqttMessage(const s_MqttMessage &message);
