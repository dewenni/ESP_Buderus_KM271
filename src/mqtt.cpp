#include <WiFi.h>
#include <basics.h>
#include <km271.h>
#include <language.h>
#include <message.h>
#include <mqtt.h>
#include <mqttDiscovery.h>
#include <oilmeter.h>
#include <queue>
#include <simulation.h>

#define PAYLOAD_LEN 512
#define MAX_MQTT_CMD 20

struct s_MqttMessage {
  char topic[512];
  char payload[PAYLOAD_LEN];
  int len;
};

/* D E C L A R A T I O N S ****************************************************/
static void processMqttMessage();
static AsyncMqttClient mqtt_client;
static bool bootUpMsgDone, setupDone = false;
static int targetIndex = -1;
static const char *TAG = "MQTT"; // LOG TAG
static char lastError[64] = "---";
static int mqtt_retry = 0;
static muTimer mqttReconnectTimer;
static char haStatusTopic[128];
static bool km271sendRequest = false;
static int km271sendCnt = 0;
static muTimer km271sendTimer;
std::queue<s_MqttMessage> mqttCmdQueue;

/**
 * *******************************************************************
 * @brief   add message to mqtt command buffer
 * @param   topic, payload, len
 * @return  none
 * *******************************************************************/
void addMqttCmd(const char *topic, const char *payload, int len) {
  if (mqttCmdQueue.size() < MAX_MQTT_CMD) {
    s_MqttMessage message;
    strncpy(message.topic, topic, sizeof(message.topic) - 1);
    message.topic[sizeof(message.topic) - 1] = '\0';

    strncpy(message.payload, payload, sizeof(message.payload) - 1);
    message.payload[sizeof(message.payload) - 1] = '\0';

    message.len = len;

    mqttCmdQueue.push(message);
    ESP_LOGD(TAG, "add msg to buffer: %s, %s", topic, payload);
  } else {
    ESP_LOGE(TAG, "too many commands within too short time");
  }
}

/**
 * *******************************************************************
 * @brief   mqtt publish wrapper
 * @param   topic, payload, retained
 * @return  none
 * *******************************************************************/
void mqttPublish(const char *topic, const char *payload, boolean retained) { mqtt_client.publish(topic, 0, retained, payload); }

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
void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {

  s_MqttMessage msgCpy;

  msgCpy.len = len;

  if (topic == NULL) {
    msgCpy.topic[0] = '\0';
  } else {
    strncpy(msgCpy.topic, topic, sizeof(msgCpy.topic) - 1);
    msgCpy.topic[sizeof(msgCpy.topic) - 1] = '\0';
  }
  if (payload == NULL) {
    msgCpy.payload[0] = '\0';
  } else if (len > 0 && len < PAYLOAD_LEN) {
    memcpy(msgCpy.payload, payload, len);
    msgCpy.payload[len] = '\0';
  }

  addMqttCmd(msgCpy.topic, msgCpy.payload, msgCpy.len);

  ESP_LOGI(TAG, "msg received | topic: %s | payload: %s", msgCpy.topic, msgCpy.payload);
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets connected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttConnect(bool sessionPresent) {
  mqtt_retry = 0;
  ESP_LOGI(TAG, "MQTT connected");
  // Once connected, publish an announcement...
  sendWiFiInfo();
  // ... and resubscribe
  mqtt_client.subscribe(addTopic("/cmd/#"), 0);
  mqtt_client.subscribe(addTopic("/setvalue/#"), 0);

  if (config.mqtt.ha_enable) {
    mqtt_client.subscribe(haStatusTopic, 0);
  }
}

/**
 * *******************************************************************
 * @brief   callback function if MQTT gets disconnected
 * @param   none
 * @return  none
 * *******************************************************************/
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

  switch (reason) {
  case AsyncMqttClientDisconnectReason::TCP_DISCONNECTED:
    snprintf(lastError, sizeof(lastError), "TCP DISCONNECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_UNACCEPTABLE_PROTOCOL_VERSION:
    snprintf(lastError, sizeof(lastError), "MQTT UNACCEPTABLE PROTOCOL VERSION");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_IDENTIFIER_REJECTED:
    snprintf(lastError, sizeof(lastError), "MQTT IDENTIFIER REJECTED");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_SERVER_UNAVAILABLE:
    snprintf(lastError, sizeof(lastError), "MQTT SERVER UNAVAILABLE");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_MALFORMED_CREDENTIALS:
    snprintf(lastError, sizeof(lastError), "MQTT MALFORMED CREDENTIALS");
    break;
  case AsyncMqttClientDisconnectReason::MQTT_NOT_AUTHORIZED:
    snprintf(lastError, sizeof(lastError), "MQTT NOT AUTHORIZED");
    break;
  case AsyncMqttClientDisconnectReason::TLS_BAD_FINGERPRINT:
    snprintf(lastError, sizeof(lastError), "TLS BAD FINGERPRINT");
    break;
  default:
    snprintf(lastError, sizeof(lastError), "UNKNOWN ERROR");
    break;
  }
}

/**
 * *******************************************************************
 * @brief   is MQTT connected
 * @param   none
 * @return  none
 * *******************************************************************/
bool mqttIsConnected() { return mqtt_client.connected(); }

const char *mqttGetLastError() { return lastError; }

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
  mqtt_client.connected();

  if (config.mqtt.ha_enable) {
    snprintf(haStatusTopic, sizeof(haStatusTopic), "%s/status", config.mqtt.ha_topic);
  }

  ESP_LOGI(TAG, "MQTT setup done!");
}

/**
 * *******************************************************************
 * @brief   MQTT cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttCyclic() {

  // process incoming messages
  if (!mqttCmdQueue.empty()) {
    processMqttMessage();
  }

  // call setup when connection is established
  if (config.mqtt.enable && !setupMode && !setupDone && (eth.connected || wifi.connected)) {
    mqttSetup();
    setupDone = true;
  }

  // automatic reconnect to mqtt broker if connection is lost - try 5 times, then reboot
  if (!mqtt_client.connected() && (wifi.connected || eth.connected)) {
    if (mqtt_retry == 0) {
      mqtt_retry++;
      mqtt_client.connect();
      ESP_LOGI(TAG, "MQTT - connection attempt: 1/5");
    } else if (mqttReconnectTimer.delayOnTrigger(true, MQTT_RECONNECT * mqtt_retry)) {
      mqttReconnectTimer.delayReset();
      if (mqtt_retry < 5) {
        mqtt_retry++;
        mqtt_client.connect();
        ESP_LOGI(TAG, "MQTT - connection attempt: %i/5", mqtt_retry);
      } else {
        ESP_LOGI(TAG, "MQTT connection not possible, esp rebooting...");
        storeData(); // store Data before reboot
        EspSysUtil::RestartReason::saveLocal("no mqtt connection");
        yield();
        delay(1000);
        yield();
        ESP.restart();
      }
    }
  }

  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqtt_client.connected()) {
    bootUpMsgDone = true;
    char tempMessage[128];
    snprintf(tempMessage, sizeof(tempMessage), "%s\n(%s)", KM_INFO_MSG::RESTARTED[config.lang], EspSysUtil::RestartReason::get());
    km271Msg(KM_TYP_MESSAGE, tempMessage, "");

    // send initial mqtt discovery messages after restart
    if (config.mqtt.ha_enable) {
      mqttDiscoverySendConfig();
    }
  }

  // call mqttDiscovery cyclic function if HA is activated
  if (config.mqtt.ha_enable && mqtt_client.connected()) {
    mqttDiscoveryCyclic();
  }

  // on request send HA-Discovery config and all km271 config and status values
  if (km271sendTimer.cycleTrigger(3000) && km271sendRequest) {
    switch (km271sendCnt) {
    case 0:
      mqttDiscoverySendConfig(); // send discovery configuration
      break;
    case 1:
      sendAllKmCfgValues(); // send all config values
      break;
    case 2:
      sendAllKmStatValues(); // send all status values
      km271sendRequest = false;
      break;
    default:
      km271sendCnt = -1;
      break;
    }
    km271sendCnt = (km271sendCnt + 1) % 3;
  }
}

/**
 * *******************************************************************
 * @brief   MQTT callback function for incoming message
 * @param   topic, payload
 * @return  none
 * *******************************************************************/
void processMqttMessage() {

  s_MqttMessage msgCpy = mqttCmdQueue.front();

  ESP_LOGD(TAG, "process msg from buffer: %s, %s", msgCpy.topic, msgCpy.payload);

  // payload as number
  int16_t intVal = 0;
  float floatVal = 0.0;
  if (msgCpy.len > 0) {
    intVal = atoi(msgCpy.payload);
    floatVal = atoff(msgCpy.payload);
  }

  // restart ESP command
  if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::RESTART[config.mqtt.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, "restart requested!", "");
    EspSysUtil::RestartReason::saveLocal("mqtt command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }
  // send sim data
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::SIMDATA[config.mqtt.lang])) == 0) {
    if (config.sim.enable) {
      km271Msg(KM_TYP_MESSAGE, "start sending sim data", "");
      startSimData();
    } else {
      km271Msg(KM_TYP_MESSAGE, "simulation mode not active", "");
    }
  }
  // Service command
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::SERVICE[config.mqtt.lang])) == 0) {
    if (msgCpy.len == 23) { // 23 characters: 8 Hex-values + 7 separators "_"
      uint8_t hexArray[8];
      // Iteration thru payload
      char *token = strtok(msgCpy.payload, "_");
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
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::DEBUG[config.mqtt.lang])) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::SET_DEBUG_FLT[config.mqtt.lang])) == 0) {
    char errMsg[256];
    if (setDebugFilter(msgCpy.payload, strlen(msgCpy.payload), errMsg, sizeof(errMsg))) {
      km271Msg(KM_TYP_MESSAGE, "filter accepted", "");
    } else {
      km271Msg(KM_TYP_MESSAGE, errMsg, "");
    }
  }
  // get debug filter
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::GET_DEBUG_FLT[config.mqtt.lang])) == 0) {
    km271Msg(KM_TYP_MESSAGE, config.debug.filter, "");
  }
  // set date and time
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::DATETIME[config.mqtt.lang])) == 0) {
    km271SetDateTimeNTP();
  }
  // set oilmeter
  else if (strcasecmp(msgCpy.topic, addTopic(MQTT_CMD::OILCNT[config.mqtt.lang])) == 0) {
    cmdSetOilmeter(intVal);
  }
  // homeassistant/status
  else if (config.mqtt.ha_enable && strcmp(msgCpy.topic, haStatusTopic) == 0) {
    if (config.mqtt.ha_enable && strcmp(msgCpy.payload, "online") == 0) {
      km271Msg(KM_TYP_MESSAGE, "send discovery configuration", "");
      km271sendRequest = true;
    }
  }

  // HK1 Betriebsart
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC1_OPMODE, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC2_OPMODE, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, intVal);
  }
  // HK2 Auslegung
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK1 Tag-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, floatVal);
  }
  // HK2 Tag-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, floatVal);
  }
  // HK1 Nacht-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, floatVal);
  }
  // HK2 Nacht-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, floatVal);
  }
  // HK1 Ferien-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, floatVal);
  }
  // HK2 Ferien-Soll Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, floatVal);
  }
  // WW Betriebsart
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_WW_OPMODE, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::OPMODE[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC1_SUMMER, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::SUMMER[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::SUMMER[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::SUMMER[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, intVal);
  }
  // HK2 Sommer-Ab Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC2_SUMMER, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::SUMMER[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::SUMMER[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::SUMMER[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, intVal);
  }
  // WW-Temperatur
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, intVal);
  }
  // HK1 Ferien Tage
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, intVal);
  }
  // HK2 Ferien Tage
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, intVal);
  }
  // WW Pump Cycles
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, intVal);
    } else if (msgCpy.len > 0) {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::CIRC_INTERVAL[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::CIRC_INTERVAL[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::CIRC_INTERVAL[config.mqtt.lang][i], msgCpy.payload) == 0) {
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
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_ON_TEMP, intVal);
  }
  // HK2 Reglereingriff
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang])) == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_ON_TEMP, intVal);
  }
  // HK1 Absenkungsart
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, intVal);
    } else {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][i], msgCpy.payload) == 0) {
          targetIndex = i;
          break;
        }
      }
      km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, targetIndex);
    }
  }
  // HK2 Absenkungsart
  else if (strcasecmp(msgCpy.topic, addCfgCmdTopic(KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang])) == 0) {
    if (EspStrUtil::isNumber(msgCpy.payload)) {
      km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, intVal);
    } else {
      targetIndex = -1;
      int arraySize = sizeof(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang]) / sizeof(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][0]);
      for (int i = 0; i < arraySize; i++) {
        if (strcasecmp(KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][i], msgCpy.payload) == 0) {
          targetIndex = i;
          break;
        }
      }
      km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, targetIndex);
    }
  }

  mqttCmdQueue.pop(); // next entry in Queue
}