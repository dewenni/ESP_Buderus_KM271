#include <WiFi.h>
#include <basics.h>
#include <km271.h>
#include <message.h>
#include <mqtt.h>
#include <oilmeter.h>
#include <simulation.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
WiFiClient espClient;
AsyncMqttClient mqtt_client;
s_mqtt_cmds mqttCmd; // exts that are used as topics for KM271 commands
s_km271_msg infoMsg;
muTimer mqttReconnectTimer = muTimer(); // timer for reconnect delay
int mqtt_retry = 0;
bool bootUpMsgDone = false;

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
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect(bool sessionPresent) {
  mqtt_retry = 0;
  msgLn("MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();
  // ... and resubscribe
  mqtt_client.subscribe(addTopic("/cmd/#"), 0);
  mqtt_client.subscribe(addTopic("/setvalue/#"), 0);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets disconnected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) { msgLn("MQTT disconnected"); }

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

// local copy of payload as string with NULL termination
#define PAYLOAD_LEN 128
  char payloadCopy[PAYLOAD_LEN] = {'\0'};
  if (len > 0 && len < PAYLOAD_LEN) {
    memcpy(payloadCopy, payload, len);
    payloadCopy[len] = '\0';
  }

  // payload as number
  long intVal = 0;
  float floatVal = 0.0;
  if (len > 0) {
    intVal = atoi(payloadCopy);
    floatVal = atoff(payloadCopy);
  }

  // restart ESP command
  if (strcmp(topic, addTopic(mqttCmd.RESTART[config.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, "restart requested!", "");
    saveRestartReason("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }
  // send sim data
  else if (strcmp(topic, addTopic(mqttCmd.SIMDATA[config.lang])) == 0) {
    if (config.sim.enable) {
      km271Msg(KM_TYP_MESSAGE, "start sending sim data", "");
      startSimData();
    } else {
      km271Msg(KM_TYP_MESSAGE, "simulation mode not active", "");
    }
  }
  // Service command
  else if (strcmp(topic, addTopic(mqttCmd.SERVICE[config.lang])) == 0) {
    if (len == 23) { // 23 characters: 8 Hex-values + 7 separators "_"
      uint8_t hexArray[8];
      // Iteration thru payload
      char *token = strtok(payload, "_");
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
  else if (strcmp(topic, addTopic(mqttCmd.DEBUG[config.lang])) == 0) {
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
  else if (strcmp(topic, addTopic(mqttCmd.SET_DEBUG_FLT[config.lang])) == 0) {
    char errMsg[256];
    if (setDebugFilter(payloadCopy, strlen(payloadCopy), errMsg, sizeof(errMsg))) {
      km271Msg(KM_TYP_MESSAGE, "filter accepted", "");
    } else {
      km271Msg(KM_TYP_MESSAGE, errMsg, "");
    }
  }
  // get debug filter
  else if (strcmp(topic, addTopic(mqttCmd.GET_DEBUG_FLT[config.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, config.debug.filter, "");
  }
  // set date and time
  else if (strcmp(topic, addTopic(mqttCmd.DATETIME[config.lang])) == 0) {
    km271SetDateTimeNTP();
  }
  // set oilmeter
  else if (strcmp(topic, addTopic(mqttCmd.OILCNT[config.lang])) == 0) {
    cmdSetOilmeter(intVal);
  }
  // HK1 Betriebsart
  else if (strcmp(topic, addTopic(mqttCmd.HC1_OPMODE[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, intVal);
  }
  // HK2 Betriebsart
  else if (strcmp(topic, addTopic(mqttCmd.HC2_OPMODE[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, intVal);
  }
  // HK1 Programm
  else if (strcmp(topic, addTopic(mqttCmd.HC1_PRG[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, intVal);
  }
  // HK2 Programm
  else if (strcmp(topic, addTopic(mqttCmd.HC2_PRG[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, intVal);
  }
  // HK1 Auslegung
  else if (strcmp(topic, addTopic(mqttCmd.HC1_INTERPRET[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, intVal);
  }
  // HK2 Auslegung
  else if (strcmp(topic, addTopic(mqttCmd.HC2_INTERPRET[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_SWITCH_OFF_THRESHOLD[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_SWITCH_OFF_THRESHOLD[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK1 Tag-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_DAY_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, floatVal);
  }
  // HK2 Tag-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_DAY_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, floatVal);
  }
  // HK1 Nacht-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_NIGHT_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, floatVal);
  }
  // HK2 Nacht-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_NIGHT_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, floatVal);
  }
  // HK1 Ferien-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_HOLIDAY_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, floatVal);
  }
  // HK2 Ferien-Soll Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_HOLIDAY_SETPOINT[config.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, floatVal);
  }
  // WW Betriebsart
  else if (strcmp(topic, addTopic(mqttCmd.WW_OPMODE[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, intVal);
  }
  // HK1 Sommer-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_SUMMER[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SUMMER, intVal);
  }
  // HK1 Frost-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC1_FROST[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, intVal);
  }
  // HK2 Sommer-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_SUMMER[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SUMMER, intVal);
  }
  // HK2 Frost-Ab Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.HC2_FROST[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, intVal);
  }
  // WW-Temperatur
  else if (strcmp(topic, addTopic(mqttCmd.WW_SETPOINT[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, intVal);
  }
  // HK1 Ferien Tage
  else if (strcmp(topic, addTopic(mqttCmd.HC1_HOLIDAYS[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, intVal);
  }
  // HK2 Ferien Tage
  else if (strcmp(topic, addTopic(mqttCmd.HC2_HOLIDAYS[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, intVal);
  }
  // WW Pump Cycles
  else if (strcmp(topic, addTopic(mqttCmd.WW_PUMP_CYCLES[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, intVal);
  }
  // HK1 Reglereingriff
  else if (strcmp(topic, addTopic(mqttCmd.HC1_SWITCH_ON_TEMP[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_ON_TEMP, intVal);
  }
  // HK2 Reglereingriff
  else if (strcmp(topic, addTopic(mqttCmd.HC2_SWITCH_ON_TEMP[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_ON_TEMP, intVal);
  }
  // HK1 Absenkungsart
  else if (strcmp(topic, addTopic(mqttCmd.HC1_REDUCTION_MODE[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, intVal);
  }
  // HK2 Absenkungsart
  else if (strcmp(topic, addTopic(mqttCmd.HC2_REDUCTION_MODE[config.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, intVal);
  }
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup() {
  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);
  mqtt_client.setServer(config.mqtt.server, config.mqtt.port);
  mqtt_client.setClientId(config.wifi.hostname);
  mqtt_client.setCredentials(config.mqtt.user, config.mqtt.password);
  mqtt_client.setWill(addTopic("/status"), 0, true, "offline");
  mqtt_client.setKeepAlive(10);
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void checkMqtt() {
  
  if (mqtt_client.connected()){
    mqttDiscoverySetup();
    mqttDiscoveryCyclic();
  }
  
  // automatic reconnect to mqtt broker if connection is lost - try 5 times, then reboot
  if (!mqtt_client.connected() && WiFi.isConnected()) {
    if (mqtt_retry == 0) {
      mqtt_retry++;
      mqtt_client.connect();
      msgLn("MQTT - connection attempt: 1/5");
    } else if (mqttReconnectTimer.delayOnTrigger(true, MQTT_RECONNECT)) {
      mqttReconnectTimer.delayReset();
      if (mqtt_retry < 5) {
        mqtt_retry++;
        mqtt_client.connect();
        msg("MQTT - connection attempt: ");
        msg(int8ToString(mqtt_retry));
        msgLn("/5");
      } else {
        msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
        msgLn("MQTT connection not possible, esp rebooting...");
        msgLn("\n! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !\n");
        storeData(); // store Data before reboot
        saveRestartReason("no mqtt connection");
        yield();
        delay(1000);
        yield();
        ESP.restart();
      }
    }
  }
  // send bootup message after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.connected()) {
    bootUpMsgDone = true;
    char restartReason[64];
    char tempMessage[128];
    getRestartReason(restartReason, sizeof(restartReason));
    snprintf(tempMessage, sizeof(tempMessage), "%s\n(%s)", infoMsg.RESTARTED[config.lang], restartReason);
    km271Msg(KM_TYP_MESSAGE, tempMessage, "");
  }
}

/**
 * *******************************************************************
 * @brief   MQTT Publish function for external use
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttPublish(const char *sendtopic, const char *payload, boolean retained) {
  uint8_t qos = 0;
  mqtt_client.publish(sendtopic, qos, retained, payload);
}
