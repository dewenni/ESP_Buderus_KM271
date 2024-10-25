#include <WiFi.h>
#include <basics.h>
#include <km271.h>
#include <message.h>
#include <mqtt.h>
#include <mqttDiscovery.h>
#include <oilmeter.h>
#include <simulation.h>

/* D E C L A R A T I O N S ****************************************************/
WiFiClient espClient;
Mycila::MQTT mqtt_client;
s_mqtt_cmds mqttCmd; // exts that are used as topics for KM271 commands
s_cfg_topics cfgTopics;
s_cfg_arrays cfgArrays;
s_km271_msg infoMsg;
muTimer mqttReconnectTimer = muTimer(); // timer for reconnect delay
int mqtt_retry = 0;
bool bootUpMsgDone, setupDone = false;
int targetIndex = -1;

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char *addCfgCmdTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s/setvalue/%s", config.mqtt.topic, suffix);
  return newTopic;
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void onMqttMessage(const char *topic, const char *payload) {

  char payloadCopy[512];

  // Überprüfe, ob der Payload gültig ist. Falls null, setze ihn auf einen leeren String.
  if (payload == NULL) {
    payloadCopy[0] = '\0';
    msgLn("Payload empty");
  } else {
    strncpy(payloadCopy, payload, sizeof(payloadCopy) - 1); // Kopiere Payload, lasse Platz für '\0'
    payloadCopy[sizeof(payloadCopy) - 1] = '\0';            // Stelle sicher, dass der String null-terminiert ist

    msg("Payload: ");
    msgLn(payload);
    msg("PayloadCopy: ");
    msgLn(payloadCopy);
  }
  int len = strlen(payloadCopy);
  Serial.print("PayloadCopy len: ");
  Serial.println(len);

  long intVal = 0;
  float floatVal = 0.0;

  if (len > 0) {
    char *endPtr;

    intVal = strtol(payloadCopy, &endPtr, 10);
    if (*endPtr != '\0') {
      printf("Invalid integer conversion\n");
    }

    floatVal = strtof(payloadCopy, &endPtr);
    if (*endPtr != '\0') {
      printf("Invalid float conversion\n");
    }
  }

  // restart ESP command
  if (strcasecmp(topic, addTopic(mqttCmd.RESTART[config.mqtt.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, "restart requested!", "");
    saveRestartReason("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }
  // send sim data
  else if (strcasecmp(topic, addTopic(mqttCmd.SIMDATA[config.mqtt.lang])) == 0) {
    if (config.sim.enable) {
      km271Msg(KM_TYP_MESSAGE, "start sending sim data", "");
      startSimData();
    } else {
      km271Msg(KM_TYP_MESSAGE, "simulation mode not active", "");
    }
  }
  // Service command
  else if (strcasecmp(topic, addTopic(mqttCmd.SERVICE[config.mqtt.lang])) == 0) {
    if (len == 23) { // 23 characters: 8 Hex-values + 7 separators "_"
      uint8_t hexArray[8];
      // Iteration thru payload
      char *token = strtok(payloadCopy, "_");
      size_t i = 0;
      while (token != NULL && i < 8) {
        // check, if the substring contains valid hex values
        size_t tokenLen = strlen(token);
        if (tokenLen != 2 || !isxdigit(token[0]) || !isxdigit(token[1])) {
          // invalid hex values found
          km271Msg(KM_TYP_MESSAGE, "invalid hex parameter", "");
          return;
        }
        hexArray[i] = strtol(token, NULL, 16); // convert hex strings to uint8_t
        token = strtok(NULL, "_");             // next substring
        i++;
      }
      // check if all 8 hex values are found
      if (i == 8) {
        // everything seems to be valid - call service function
        km271Msg(KM_TYP_MESSAGE, "service message accepted", "");
        km271sendServiceCmd(hexArray);
      } else {
        // not enough hex values found
        km271Msg(KM_TYP_MESSAGE, "not enough hex parameter", "");
      }
    } else {
      // invalid parameter length
      km271Msg(KM_TYP_MESSAGE, "invalid parameter size", "");
    }
  }
  // enable / disable debug
  else if (strcasecmp(topic, addTopic(mqttCmd.DEBUG[config.mqtt.lang])) == 0) {
    if (intVal == 1) {
      config.debug.enable = true;
      configSaveToFile();
      km271Msg(KM_TYP_MESSAGE, "debug enabled", "");
    } else if (intVal == 0) {
      config.debug.enable = false;
      configSaveToFile();
      km271Msg(KM_TYP_MESSAGE, "debug disabled", "");
    }
  }
  // set debug filter
  else if (strcasecmp(topic, addTopic(mqttCmd.SET_DEBUG_FLT[config.mqtt.lang])) == 0) {
    char errMsg[256];
    if (setDebugFilter(payloadCopy, strlen(payloadCopy), errMsg, sizeof(errMsg))) {
      km271Msg(KM_TYP_MESSAGE, "filter accepted", "");
    } else {
      km271Msg(KM_TYP_MESSAGE, errMsg, "");
    }
  }
  // get debug filter
  else if (strcasecmp(topic, addTopic(mqttCmd.GET_DEBUG_FLT[config.mqtt.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, config.debug.filter, "");
  }
  // set date and time
  else if (strcasecmp(topic, addTopic(mqttCmd.DATETIME[config.mqtt.lang])) == 0) {
    km271SetDateTimeNTP();
  }
  // set oilmeter
  else if (strcasecmp(topic, addTopic(mqttCmd.OILCNT[config.mqtt.lang])) == 0) {
    cmdSetOilmeter(intVal);
  }
  // homeassistant/status
  else if (strcmp(topic, "homeassistant/status") == 0) {
    if (config.mqtt.ha_enable && strcmp(payloadCopy, "online") == 0) {
      km271Msg(KM_TYP_MESSAGE, "send discovery configuration", "");
      mqttDiscoverySendConfig(); // send discovery configuration
    }
  }

  // HK1 Betriebsart
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_OPMODE[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC1_OPMODE, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.OPMODE[config.mqtt.lang]) / sizeof(cfgArrays.OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.OPMODE[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC1_OPMODE, targetIndex);
      }
    }
  }
  // HK2 Betriebsart
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_OPMODE[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC2_OPMODE, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.OPMODE[config.mqtt.lang]) / sizeof(cfgArrays.OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.OPMODE[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC2_OPMODE, targetIndex);
      }
    }
  }
  // HK1 Programm
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_PROGRAM[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.HC_PROGRAM[config.mqtt.lang]) / sizeof(cfgArrays.HC_PROGRAM[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.HC_PROGRAM[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, targetIndex);
      }
    }
  }
  // HK2 Programm
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_PROGRAM[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.HC_PROGRAM[config.mqtt.lang]) / sizeof(cfgArrays.HC_PROGRAM[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.HC_PROGRAM[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, targetIndex);
      }
    }
  }
  // HK1 Auslegung
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_INTERPR[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, intVal);
  }
  // HK2 Auslegung
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_INTERPR[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK1 Tag-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_DAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, floatVal);
  }
  // HK2 Tag-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_DAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, floatVal);
  }
  // HK1 Nacht-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_NIGHT_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, floatVal);
  }
  // HK2 Nacht-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_NIGHT_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, floatVal);
  }
  // HK1 Ferien-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_HOLIDAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, floatVal);
  }
  // HK2 Ferien-Soll Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_HOLIDAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, floatVal);
  }
  // WW Betriebsart
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.WW_OPMODE[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_WW_OPMODE, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.OPMODE[config.mqtt.lang]) / sizeof(cfgArrays.OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.OPMODE[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_WW_OPMODE, targetIndex);
      }
    }
  }
  // HK1 Sommer-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_SUMMER_THRESHOLD[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC1_SUMMER, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.SUMMER[config.mqtt.lang]) / sizeof(cfgArrays.SUMMER[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.SUMMER[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i + 9; // Values are 9..31 and array index is 0..22 - index 0 = value 9 / index 22 = value 31
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC1_SUMMER, targetIndex);
      }
    }
  }
  // HK1 Frost-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_FROST_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, intVal);
  }
  // HK2 Sommer-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_SUMMER_THRESHOLD[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC2_SUMMER, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.SUMMER[config.mqtt.lang]) / sizeof(cfgArrays.SUMMER[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.SUMMER[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i + 9; // Values are 9..31 and array index is 0..22 - index 0 = value 9 / index 22 = value 31
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_HC2_SUMMER, targetIndex);
      }
    }
  }
  // HK2 Frost-Ab Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_FROST_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, intVal);
  }
  // WW-Temperatur
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.WW_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, intVal);
  }
  // HK1 Ferien Tage
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_HOLIDAY_DAYS[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, intVal);
  }
  // HK2 Ferien Tage
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_HOLIDAY_DAYS[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, intVal);
  }
  // WW Pump Cycles
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.WW_CIRCULATION[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, intVal);
    } else if (len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.CIRC_INTERVAL[config.mqtt.lang]) / sizeof(cfgArrays.CIRC_INTERVAL[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.CIRC_INTERVAL[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      if (targetIndex != -1) {
        km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, targetIndex);
      }
    }
  }
  // HK1 Reglereingriff
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_SWITCH_ON_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_ON_TEMP, intVal);
  }
  // HK2 Reglereingriff
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_SWITCH_ON_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_ON_TEMP, intVal);
  }
  // HK1 Absenkungsart
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC1_REDUCTION_MODE[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, intVal);
    } else {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.REDUCT_MODE[config.mqtt.lang]) / sizeof(cfgArrays.REDUCT_MODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.REDUCT_MODE[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, targetIndex);
    }
  }
  // HK2 Absenkungsart
  else if (strcasecmp(topic, addCfgCmdTopic(cfgTopics.HC2_REDUCTION_MODE[config.mqtt.lang])) == 0) {
    if (isNumber(payloadCopy)) {
      km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, intVal);
    } else {
      targetIndex = -1;
      int arraySize = sizeof(cfgArrays.REDUCT_MODE[config.mqtt.lang]) / sizeof(cfgArrays.REDUCT_MODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(cfgArrays.REDUCT_MODE[config.mqtt.lang][i], payloadCopy) == 0) {
          targetIndex = i;
          break;
        }
      }
      km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, targetIndex);
    }
  }
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect() {
  mqtt_retry = 0;
  msgLn("MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();

  mqtt_client.subscribe(addTopic("/cmd/#"), [](const String &topic, const String &payload) { onMqttMessage(topic.c_str(), payload.c_str()); });

  mqtt_client.subscribe(addTopic("/setvalue/#"), [](const String &topic, const String &payload) { onMqttMessage(topic.c_str(), payload.c_str()); });

  mqtt_client.subscribe(addTopic("homeassistant/status"),
                        [](const String &topic, const String &payload) { onMqttMessage(topic.c_str(), payload.c_str()); });
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup() {

  mqtt_client.onConnect([]() { onMqttConnect(); });

  Mycila::MQTT::Config mqtt_config;
  mqtt_config.server = config.mqtt.server;
  mqtt_config.port = config.mqtt.port;
  mqtt_config.username = config.mqtt.user;
  mqtt_config.password = config.mqtt.password;
  mqtt_config.clientId = config.wifi.hostname;
  mqtt_config.willTopic = addTopic("/status");

  mqtt_client.setAsync(false);
  mqtt_client.begin(mqtt_config);
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void checkMqtt() {

  if (config.mqtt.enable && !setupMode && !setupDone) {
    mqttSetup();
    setupDone = true;
  }

  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.isConnected()) {
    bootUpMsgDone = true;
    char restartReason[64];
    char tempMessage[128];
    getRestartReason(restartReason, sizeof(restartReason));
    snprintf(tempMessage, sizeof(tempMessage), "%s\n(%s)", infoMsg.RESTARTED[config.lang], restartReason);
    km271Msg(KM_TYP_MESSAGE, tempMessage, "");

    // send initial mqtt discovery messages after restart
    if (config.mqtt.ha_enable) {
      mqttDiscoverySendConfig();
    }
  }
  // call mqtt cyclic function if activated
  if (config.mqtt.ha_enable && mqtt_client.isConnected()) {
    mqttDiscoveryCyclic();
  }
}

/**
 * *******************************************************************
 * @brief   MQTT Publish function for external use
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttPublish(const char *sendtopic, const char *payload, boolean retained) { mqtt_client.publish(sendtopic, payload, retained); }
