#include <mqtt.h>
#include <mqttCustom.h>
#include <basics.h>
#include <WiFiMgn.h>
#include <language.h>
#include <message.h>
#include <mqttDiscovery.h>
#include <km271.h>
#include <oilmeter.h>
#include <simulation.h>
#include <stdint.h>

static const char *TAG = "MQTT";
static bool bootUpMsgDone = false;
static char haStatusTopic[128];
static int targetIndex = -1;
static bool km271sendRequest = false;
static int km271sendCnt = 0;
static muTimer km271sendTimer;

void mqttCustomConnected() {
  sendWiFiInfo();
  mqttSubscribe(addTopic("/cmd/#"));
  mqttSubscribe(addTopic("/setvalue/#"));
  const auto &settings = mqttGetConnectionConfig();
  snprintf(haStatusTopic, sizeof(haStatusTopic), "%s/status", settings.ha_topic);
  if (settings.ha_enable) {
    mqttSubscribe(haStatusTopic);
  }
}

void mqttCustomCyclic() {
  // send bootup messages after restart and established mqtt connection
  if (!bootUpMsgDone && mqttIsConnected()) {
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
  if (config.mqtt.ha_enable && mqttIsConnected()) {
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

static bool parseOilmeterCounter(const char *payload, int len, long &value) {
  if (len <= 0 || static_cast<size_t>(len) >= MQTT_PAYLOAD_LEN || payload[len] != '\0') {
    return false;
  }
  long parsed = 0;
  for (int i = 0; i < len; ++i) {
    if (payload[i] < '0' || payload[i] > '9') {
      return false;
    }
    const int digit = payload[i] - '0';
    if (parsed > (INT32_MAX - digit) / 10) {
      return false;
    }
    parsed = parsed * 10 + digit;
  }
  value = parsed;
  return true;
}

void processMqttMessage(const s_MqttMessage &message) {

  s_MqttMessage msgCpy = message;

  ESP_LOGD(TAG, "process msg from buffer: %s, %s", msgCpy.topic, msgCpy.payload);

  // payload as number
  // wide enough that the range checks inside km271sendCmd() see the value the
  // user actually sent: as int16_t, publishing 286 to a 30..60 setpoint wrapped
  // to 30 and was accepted as valid, and the oil counter (in 1/100 L) could only
  // express 0..327.67 L before wrapping negative
  long intVal = 0;
  float floatVal = 0.0;
  if (msgCpy.len > 0) {
    // strtol() has defined overflow behaviour; atol() does not.
    intVal = strtol(msgCpy.payload, nullptr, 10);
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
      bool validHex = true;
      while (token != NULL && i < 8) {
        // check, if the substring contains valid hex values
        size_t tokenLen = strlen(token);
        if (tokenLen != 2 || !isxdigit(token[0]) || !isxdigit(token[1])) {
          // Reject the malformed service command without sending it.
          km271Msg(KM_TYP_MESSAGE, "invalid hex parameter", "");
          validHex = false;
          break;
        }
        hexArray[i] = strtol(token, NULL, 16); // convert hex strings to uint8_t
        token = strtok(NULL, "_");             // next substring
        i++;
      }
      // check if all 8 hex values are found
      if (!validHex) {
        // already reported above
      } else if (i == 8) {
        // everything seems to be valid - call service function
        if (km271sendServiceCmd(hexArray)) {
          km271Msg(KM_TYP_MESSAGE, "service message accepted", "");
        } // Rejections (including full queue/disconnect) are reported by the driver.
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
    long counter;
    if (parseOilmeterCounter(msgCpy.payload, msgCpy.len, counter)) {
      cmdSetOilmeter(counter);
    } else {
      ESP_LOGW(TAG, "Ignored invalid oilcounter command: '%s'", msgCpy.payload);
    }
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


}
