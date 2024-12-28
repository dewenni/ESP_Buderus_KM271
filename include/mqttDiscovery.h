#pragma once
#include <km271.h>
#include <mqtt.h>

void mqttDiscoverySetup();
void mqttDiscoveryCyclic();
void mqttDiscoverySendConfig();
void mqttDiscoveryResetConfig();