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
#include <cerrno>
#include <climits>
#include <cmath>

static const char *TAG = "MQTT";
static bool bootUpMsgDone = false;
static char haStatusTopic[128];
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

// All setvalue configuration commands share topic matching and payload parsing.
static bool processMqttConfigCommand(const char *topic, const char *payload) {
  struct ConfigCommand {
    const char *topic;
    e_km271_sendCmd command;
    bool fractional;
    const char *const *choices;
    unsigned count;
    int choiceOffset; // Summer/winter choices map to 9..31 instead of 0..22.
  };
  const uint8_t language = config.mqtt.lang;
  const ConfigCommand commands[] = {
    // Heating circuit 1
    {KM_CFG_TOPIC::HC1_OPMODE[language], KM271_SENDCMD_HC1_OPMODE, false, KM_CFG_ARRAY::OPMODE[language], 3, 0},
    {KM_CFG_TOPIC::HC1_PROGRAM[language], KM271_SENDCMD_HC1_PROGRAMM, false, KM_CFG_ARRAY::HC_PROGRAM[language], 9, 0},
    {KM_CFG_TOPIC::HC1_HEATING_SYSTEM[language], KM271_SENDCMD_HC1_HEATING_SYSTEM, false, KM_CFG_ARRAY::HEATING_SYSTEM[language], 4, 0},
    {KM_CFG_TOPIC::HC1_REDUCTION_MODE[language], KM271_SENDCMD_HC1_REDUCTION_MODE, false, KM_CFG_ARRAY::REDUCT_MODE[language], 4, 0},
    {KM_CFG_TOPIC::HC1_INTERPR[language], KM271_SENDCMD_HC1_DESIGN_TEMP, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_MAX_TEMP[language], KM271_SENDCMD_HC1_MAX_TEMP, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_TEMP_OFFSET[language], KM271_SENDCMD_HC1_TEMP_OFFSET, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[language], KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_DAY_TEMP[language], KM271_SENDCMD_HC1_DAY_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_NIGHT_TEMP[language], KM271_SENDCMD_HC1_NIGHT_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[language], KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[language], KM271_SENDCMD_HC1_SUMMER, false, KM_CFG_ARRAY::SUMMER[language], 23, 9},
    {KM_CFG_TOPIC::HC1_FROST_THRESHOLD[language], KM271_SENDCMD_HC1_FROST, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[language], KM271_SENDCMD_HC1_HOLIDAYS, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[language], KM271_SENDCMD_HC1_SWITCH_ON_TEMP, false, nullptr, 0, 0},
    // Heating circuit 2
    {KM_CFG_TOPIC::HC2_OPMODE[language], KM271_SENDCMD_HC2_OPMODE, false, KM_CFG_ARRAY::OPMODE[language], 3, 0},
    {KM_CFG_TOPIC::HC2_PROGRAM[language], KM271_SENDCMD_HC2_PROGRAMM, false, KM_CFG_ARRAY::HC_PROGRAM[language], 9, 0},
    {KM_CFG_TOPIC::HC2_HEATING_SYSTEM[language], KM271_SENDCMD_HC2_HEATING_SYSTEM, false, KM_CFG_ARRAY::HEATING_SYSTEM[language], 4, 0},
    {KM_CFG_TOPIC::HC2_REDUCTION_MODE[language], KM271_SENDCMD_HC2_REDUCTION_MODE, false, KM_CFG_ARRAY::REDUCT_MODE[language], 4, 0},
    {KM_CFG_TOPIC::HC2_INTERPR[language], KM271_SENDCMD_HC2_DESIGN_TEMP, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_MAX_TEMP[language], KM271_SENDCMD_HC2_MAX_TEMP, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_TEMP_OFFSET[language], KM271_SENDCMD_HC2_TEMP_OFFSET, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[language], KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_DAY_TEMP[language], KM271_SENDCMD_HC2_DAY_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_NIGHT_TEMP[language], KM271_SENDCMD_HC2_NIGHT_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[language], KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, true, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[language], KM271_SENDCMD_HC2_SUMMER, false, KM_CFG_ARRAY::SUMMER[language], 23, 9},
    {KM_CFG_TOPIC::HC2_FROST_THRESHOLD[language], KM271_SENDCMD_HC2_FROST, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[language], KM271_SENDCMD_HC2_HOLIDAYS, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[language], KM271_SENDCMD_HC2_SWITCH_ON_TEMP, false, nullptr, 0, 0},
    // Hot water
    {KM_CFG_TOPIC::WW_OPMODE[language], KM271_SENDCMD_WW_OPMODE, false, KM_CFG_ARRAY::OPMODE[language], 3, 0},
    {KM_CFG_TOPIC::WW_TEMP[language], KM271_SENDCMD_WW_SETPOINT, false, nullptr, 0, 0},
    {KM_CFG_TOPIC::WW_CIRCULATION[language], KM271_SENDCMD_WW_PUMP_CYCLES, false, KM_CFG_ARRAY::CIRC_INTERVAL[language], 8, 0},
    {KM_CFG_TOPIC::WW_PROCESSING[language], KM271_SENDCMD_WW_PROCESSING, false, KM_CFG_ARRAY::ON_OFF[language], 2, 0},
    {KM_CFG_TOPIC::WW_PRIO[language], KM271_SENDCMD_WW_PRIORITY, false, KM_CFG_ARRAY::ON_OFF[language], 2, 0},
  };
  for (const auto &entry : commands) {
    if (strcasecmp(topic, addCfgCmdTopic(entry.topic)) != 0) {
      continue;
    }
    // Match complete localized choices, including labels such as "10 °C".
    for (unsigned i = 0; i < entry.count; ++i) {
      if (strcasecmp(payload, entry.choices[i]) == 0) {
        km271sendCmd(entry.command, static_cast<int>(i) + entry.choiceOffset);
        return true;
      }
    }
    char *end = nullptr;
    errno = 0;
    const float number = strtof(payload, &end);
    if (end == payload || *end != '\0' || errno == ERANGE || !std::isfinite(number)) {
      km271Msg(KM_TYP_MESSAGE, "invalid configuration value", "");
    } else if (entry.fractional) {
      km271sendCmdFlt(entry.command, number);
    } else if (static_cast<double>(number) >= INT_MIN && static_cast<double>(number) <= INT_MAX &&
               number == static_cast<int>(number)) {
      km271sendCmd(entry.command, static_cast<int>(number));
    } else {
      km271Msg(KM_TYP_MESSAGE, "invalid configuration value", "");
    }
    return true; // Matched topic; malformed payloads must not reach another handler.
  }
  return false;
}

void processMqttMessage(const s_MqttMessage &message) {

  s_MqttMessage msgCpy = message;

  ESP_LOGD(TAG, "process msg from buffer: %s, %s", msgCpy.topic, msgCpy.payload);

  if (processMqttConfigCommand(msgCpy.topic, msgCpy.payload)) {
    return;
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
    const long intVal = msgCpy.len > 0 ? strtol(msgCpy.payload, nullptr, 10) : 0;
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
}
