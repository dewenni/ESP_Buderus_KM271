#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <MycilaMQTT.h>
#include <config.h>
#include <language.h>

/* P R O T O T Y P E S ********************************************************/
const char *addTopic(const char *suffix);
void mqttSetup();
void checkMqtt();
void mqttPublish(const char *sendtopic, const char *payload, boolean retained);
