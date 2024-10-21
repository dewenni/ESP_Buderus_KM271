#include <basics.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
char discoveryPrefix[128];
char deviceName[32];
char statePrefix[128];
char deviceId[32];
char swVersion[32];

muTimer cyclicTimer = muTimer();
s_cfg_topics cfg_topics;
s_stat_topics stat_topics;
s_cfg_arrays cfgOptions;
s_error_topics errTopics;
s_mqtt_cmds mqttCmds;

bool sendMqttConfig = false;
bool resetMqttConfig = false;

typedef enum { OPT_NULL, OPT_OP_MODE, OPT_RED_MODE, OPT_HC_PRG, OPT_WW_CRC, OPT_SUMMER } OptType;
typedef enum { TYP_TEXT, TYP_SLIDER, TYP_NUM, TYP_OPT, TYP_BTN } DeviceType;
typedef enum { KM_CMD_BTN, KM_CONFIG, KM_STATUS, KM_INFO, KM_WIFI, KM_ALARM, KM_DEBUG, KM_SENS, KM_OIL, KM_SYSINFO } KmType;
typedef enum { VAL_SPLIT, VAL_ON_OFF, VAL_ERR_OK, VAL_WW_CIRC, VAL_SUMMER_THRESHOLD } ValTmpType;
typedef struct {
  const char *min;
  const char *max;
  const char *step;
  OptType optType;
} DeviceConfig;

/**
 * *******************************************************************
 * @brief   helper function to generate value template
 * @param   type string
 * @return  value template based on type
 * *******************************************************************/
const char *valueTmpl(ValTmpType type) {
  static char output[128];
  switch (type) {
  case VAL_SPLIT:
    snprintf(output, sizeof(output), "{{value.split(' ')[0]}}");
    break;

  case VAL_ON_OFF:
    if (config.mqtt.lang == 0) {
      snprintf(output, sizeof(output), "{{'AN' if value == '1' else 'AUS' }}");
    } else {
      snprintf(output, sizeof(output), "{{'ON' if value == '1' else 'OFF' }}");
    }
    break;

  case VAL_ERR_OK:
    if (config.mqtt.lang == 0) {
      snprintf(output, sizeof(output), "{{'FEHLER' if value == '1' else 'OK' }}");
    } else {
      snprintf(output, sizeof(output), "{{'ERROR' if value == '1' else 'OK' }}");
    }
    break;

  case VAL_WW_CIRC:
    if (config.mqtt.lang == 0) {
      snprintf(output, sizeof(output), "{%% if value == 'Aus' %%} 0 {%% elif value == 'An' %%} 7 {%% else %%} {{ value }} {%% endif %%}");
    } else {
      snprintf(output, sizeof(output), "{%% if value == 'off' %%} 0 {%% elif value == 'on' %%} 7 {%% else %%} {{ value }} {%% endif %%}");
    }
    break;

  case VAL_SUMMER_THRESHOLD:
    if (config.mqtt.lang == 0) {
      snprintf(output, sizeof(output),
               "{%% if value == 'Sommer' %%} 9 {%% elif value == 'Winter' %%} 31 {%% else %%} {{value.split(' ')[0]}} {%% endif %%}");
    } else {
      snprintf(output, sizeof(output),
               "{%% if value == 'summer' %%} 9 {%% elif value == 'winter' %%} 31 {%% else %%} {{value.split(' ')[0]}} {%% endif %%}");
    }
    break;

  default:
    break;
  }
  return output;
}

DeviceConfig textPar() { return (DeviceConfig){NULL, NULL, NULL, OPT_NULL}; }
DeviceConfig optPar(OptType optType) { return (DeviceConfig){NULL, NULL, NULL, optType}; }
DeviceConfig sliderPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, OPT_NULL}; }
DeviceConfig numPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, OPT_NULL}; }

/**
 * *******************************************************************
 * @brief   generate mqtt messages for home assistant auto discovery
 * @param   kmType
 * @param   name
 * @param   deviceClass
 * @param   component
 * @param   unit
 * @param   valueTemplate
 * @param   icon
 * @param   devType
 * @param   devCfg
 * @return  none
 * *******************************************************************/
void mqttHaConfig(KmType kmType, const char *name, const char *deviceClass, const char *component, const char *unit, const char *valueTemplate,
                  const char *icon, DeviceType devType, DeviceConfig devCfg) {

  if (strlen(discoveryPrefix) == 0 || strlen(statePrefix) == 0 || strlen(name) == 0) {
    return;
  }

  JsonDocument doc;

  char configTopic[256];
  sprintf(configTopic, "%s/%s/%s/%s/config", discoveryPrefix, component, deviceId, name);

  char stateTopic[256];
  switch (kmType) {
  case KM_CONFIG:
    sprintf(stateTopic, "%s/config/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;
  case KM_STATUS:
    sprintf(stateTopic, "%s/status/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;
  case KM_SENS:
    sprintf(stateTopic, "%s/sensor/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;
  case KM_OIL:
    sprintf(stateTopic, "%s/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;
  case KM_WIFI:
    sprintf(stateTopic, "%s/wifi", statePrefix);
    doc["stat_t"] = stateTopic;
    break;
  case KM_DEBUG:
    sprintf(stateTopic, "%s/debug", statePrefix);
    doc["stat_t"] = stateTopic;
    break;
  case KM_ALARM:
    sprintf(stateTopic, "%s/alarm/%s", statePrefix, name);
    doc["stat_t"] = stateTopic;
    break;
  case KM_SYSINFO:
    sprintf(stateTopic, "%s/sysinfo", statePrefix);
    doc["stat_t"] = stateTopic;
    break;
  default:
    break;
  }

  char friendlyName[64];
  replace_underscores(name, friendlyName, sizeof(friendlyName));
  doc["name"] = friendlyName;

  doc["uniq_id"] = name;

  if (deviceClass) {
    doc["dev_cla"] = deviceClass;
  }
  if (unit) {
    doc["unit_of_meas"] = unit;
  }
  if (valueTemplate) {
    doc["val_tpl"] = valueTemplate;
  }
  if (icon) {
    doc["icon"] = icon;
  }

  if (devType == TYP_BTN) {
    char cmdTopic[256];
    sprintf(cmdTopic, "%s/cmd/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
    doc["payload_press"] = "true";
  } else if (devType != TYP_TEXT) {
    doc["ent_cat"] = "config";
    char cmdTopic[256];
    sprintf(cmdTopic, "%s/setvalue/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
  }

  if (kmType == KM_DEBUG || kmType == KM_WIFI || kmType == KM_SYSINFO) {
    doc["ent_cat"] = "diagnostic";
  }

  if (devType == TYP_SLIDER) {
    doc["mode"] = "slider";
    doc["min"] = devCfg.min;
    doc["max"] = devCfg.max;
    doc["step"] = devCfg.step;
  }
  if (devType == TYP_NUM) {
    doc["mode"] = "box";
    doc["min"] = devCfg.min;
    doc["max"] = devCfg.max;
    doc["step"] = devCfg.step;
  }

  if (devType == TYP_OPT) {
    JsonArray opts = doc["options"].to<JsonArray>();

    switch (devCfg.optType) {
    case OPT_OP_MODE:
      for (int i = 0; i < 3; i++) {
        opts.add(cfgOptions.OPMODE[config.mqtt.lang][i]);
      }
      break;
    case OPT_RED_MODE:
      for (int i = 0; i < 4; i++) {
        opts.add(cfgOptions.REDUCT_MODE[config.mqtt.lang][i]);
      }
      break;
    case OPT_HC_PRG:
      for (int i = 0; i < 9; i++) {
        opts.add(cfgOptions.HC_PROGRAM[config.mqtt.lang][i]);
      }
      break;
    case OPT_WW_CRC:
      for (int i = 0; i < 8; i++) {
        opts.add(cfgOptions.CIRC_INTERVAL[config.mqtt.lang][i]);
      }
      break;
    case OPT_SUMMER:
      for (int i = 0; i < 23; i++) {
        opts.add(cfgOptions.SUMMER[config.mqtt.lang][i]);
      }
      break;
    default:
      break;
    }
  }

  char willTopic[256];
  snprintf(willTopic, sizeof(willTopic), "%s/status", statePrefix);
  doc["avty_t"] = willTopic;

  // device
  JsonObject deviceObj = doc["dev"].to<JsonObject>();
  deviceObj["name"] = deviceName;
  deviceObj["ids"] = deviceId;
  deviceObj["mf"] = "Buderus";
  deviceObj["mdl"] = "KM271";
  deviceObj["sw"] = swVersion;

  char jsonString[1024];
  serializeJson(doc, jsonString);

  if (resetMqttConfig) {
    mqttPublish(configTopic, "", false);
  } else {
    mqttPublish(configTopic, jsonString, false);
  }
}

/**
 * *******************************************************************
 * @brief   force mqttDiscovery Setup function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttDiscoverySendConfig() { sendMqttConfig = true; }

/**
 * *******************************************************************
 * @brief   force mqttDiscovery Reset
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttDiscoveryResetConfig() { resetMqttConfig = true; }

/**
 * *******************************************************************
 * @brief   mqttDiscovery Setup function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttDiscoverySetup() {

  // copy config values
  snprintf(discoveryPrefix, sizeof(discoveryPrefix), "%s", config.mqtt.ha_topic);
  snprintf(statePrefix, sizeof(statePrefix), "%s", config.mqtt.topic);
  snprintf(deviceName, sizeof(deviceName), "%s", config.mqtt.ha_device);
  snprintf(deviceId, sizeof(deviceId), "%s", config.mqtt.ha_device);
  snprintf(swVersion, sizeof(swVersion), "%s", VERSION);

  mqttHaConfig(KM_CMD_BTN, "restart", "restart", "button", NULL, NULL, "mdi:restart", TYP_BTN, textPar());

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_OPMODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_OP_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_REDUCTION_MODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_RED_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_PROGRAM[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_HC_PRG));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_FROST_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SUMMER_THRESHOLD[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:thermometer", TYP_OPT,
                 optPar(OPT_SUMMER));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_NIGHT_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_DAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HOLIDAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT),
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SWITCH_ON_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("0", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_INTERPR[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("30", "90", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HOLIDAY_DAYS[config.mqtt.lang], NULL, "number", "days", valueTmpl(VAL_SPLIT), "mdi:calendar", TYP_NUM,
                 numPar("0", "99", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_MAX_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TEMP_OFFSET[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HEATING_SYSTEM[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:heating-coil", TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_REMOTECTRL[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:remote", TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER01[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER02[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER03[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER04[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER05[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER06[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER07[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER08[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER09[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER10[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER11[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER12[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER13[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TIMER14[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_OPMODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_OP_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_REDUCTION_MODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_RED_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_PROGRAM[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_HC_PRG));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_FROST_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SUMMER_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("9", "31", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_NIGHT_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_DAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_HOLIDAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT),
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SWITCH_ON_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("0", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_INTERPR[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_SLIDER, sliderPar("30", "90", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_HOLIDAY_DAYS[config.mqtt.lang], NULL, "number", "days", valueTmpl(VAL_SPLIT), "mdi:calendar", TYP_NUM,
                 numPar("0", "99", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_MAX_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TEMP_OFFSET[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_HEATING_SYSTEM[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:heating-coil", TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_REMOTECTRL[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:remote", TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER01[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER02[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER03[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER04[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER05[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER06[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER07[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER08[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER09[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER10[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER11[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER12[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER13[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TIMER14[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:timer-outline", TYP_TEXT, textPar());
  }

  // hot-water config values
  if (config.km271.use_ww) {
    mqttHaConfig(KM_CONFIG, cfg_topics.WW_OPMODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_OP_MODE));
    mqttHaConfig(KM_CONFIG, cfg_topics.WW_PRIO[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:thermometer-chevron-up", TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_TEMP[config.mqtt.lang], "temperature", "number", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer", TYP_SLIDER,
                 sliderPar("30", "60", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_PROCESSING[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:water-thermometer-outline", TYP_TEXT,
                 textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_CIRCULATION[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:water-sync", TYP_OPT, optPar(OPT_WW_CRC));
  }

  // general config values
  mqttHaConfig(KM_CONFIG, cfg_topics.LANGUAGE[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:translate", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.SCREEN[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:monitor", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_TYP[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:gas-burner", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.BUILDING_TYP[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:home", TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.MAX_BOILER_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer-alert",
               TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.PUMP_LOGIC[config.mqtt.lang], "temperature", "sensor", "°C", valueTmpl(VAL_SPLIT), "mdi:thermometer", TYP_TEXT,
               textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.EXHAUST_THRESHOLD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_SPLIT), "mdi:thermometer-alert",
               TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_MIN_MOD[config.mqtt.lang], NULL, "sensor", "%", valueTmpl(VAL_SPLIT), "mdi:percent", TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_MOD_TIME[config.mqtt.lang], NULL, "sensor", "s", valueTmpl(VAL_SPLIT), "mdi:clock-outline", TYP_TEXT,
               textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.TIME_OFFSET[config.mqtt.lang], NULL, "sensor", "h", valueTmpl(VAL_SPLIT), "mdi:clock-outline", TYP_TEXT,
               textPar());

  // STATUS VALUES HC1
  if (config.km271.use_hc1) {
    mqttHaConfig(KM_STATUS, stat_topics.HC1_FLOW_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_FLOW_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_HEAT_CURVE1[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_HEAT_CURVE2[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_HEAT_CURVE3[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_MIXER[config.mqtt.lang], NULL, "sensor", "%", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OFF_TIME_OPT[config.mqtt.lang], NULL, "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_ON_TIME_OPT[config.mqtt.lang], NULL, "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_AUTOMATIC[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_FROST[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_HOLIDAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_MANUAL[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_OFFTIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_ONTIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_SCREED_DRY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV1_WW_PRIO[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_DAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_EXT_SENS_ERR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_FLOW_AT_MAX[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_FLOW_SENS_ERR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_NO_COM_REMOTE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_OV2_SUMMER[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_PUMP[config.mqtt.lang], NULL, "sensor", "%", NULL, "mdi:heat-pump-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_ROOM_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC1_ROOM_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  }
  // STATUS VALUES HC2
  if (config.km271.use_hc2) {
    mqttHaConfig(KM_STATUS, stat_topics.HC2_FLOW_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_FLOW_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_HEAT_CURVE1[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_HEAT_CURVE2[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_HEAT_CURVE3[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_MIXER[config.mqtt.lang], NULL, "sensor", "%", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OFF_TIME_OPT[config.mqtt.lang], NULL, "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_ON_TIME_OPT[config.mqtt.lang], NULL, "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_AUTOMATIC[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_FROST[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_HOLIDAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_MANUAL[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_OFFTIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_ONTIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_SCREED_DRY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV1_WW_PRIO[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_DAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_EXT_SENS_ERR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_FLOW_AT_MAX[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_FLOW_SENS_ERR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_NO_COM_REMOTE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_OV2_SUMMER[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_PUMP[config.mqtt.lang], NULL, "sensor", "%", NULL, "mdi:heat-pump-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_ROOM_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
                 textPar());
    mqttHaConfig(KM_STATUS, stat_topics.HC2_ROOM_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  }

  // STATUS VALUES WW
  if (config.km271.use_ww) {
    mqttHaConfig(KM_STATUS, stat_topics.WW_ONTIME_OPT[config.mqtt.lang], NULL, "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_PUMP_CHARGE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:heat-pump-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_PUMP_CIRC[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:heat-pump-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_PUMP_SOLAR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:heat-pump-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_AUTO[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_DESINFECT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_HOLIDAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_RELOAD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_WW_STAY_COLD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_ERR_ANODE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_ERR_DESINFECT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK),
                 "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV1_ERR_SENSOR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_DAY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_HOT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_LOAD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_MANUAL[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_OFF_TIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_ON_TIME_OPT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_PRIO[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
    mqttHaConfig(KM_STATUS, stat_topics.WW_OV2_RELOAD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:information-outline",
                 TYP_TEXT, textPar());
  }

  // STATUS VALUES BOILER/BURNER
  if (config.oilmeter.use_virtual_meter) {
    mqttHaConfig(KM_STATUS, stat_topics.BOILER_CONSUMPTION[config.mqtt.lang], NULL, "sensor", "L", NULL, "mdi:barrel-outline", TYP_TEXT, textPar());
  }
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_CONTROL[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:gas-burner", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_AUX_SENS[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_BURNER[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_EXT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_GAS_SENS[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_SAFETY[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_SENSOR[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ERR_STAY_COLD[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ERR_OK), "mdi:alert-circle-outline",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_LIFETIME_4[config.mqtt.lang], "duration", "sensor", "min", NULL, "mdi:clock-outline", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_ON_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_OFF_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_SETPOINT[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_ACTIVE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_GASTEST[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_PER_FREE[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_PER_HIGH[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner",
               TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_PROTECT[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_STAGE1[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.BOILER_STATE_STAGE1[config.mqtt.lang], NULL, "sensor", NULL, valueTmpl(VAL_ON_OFF), "mdi:gas-burner", TYP_TEXT,
               textPar());

  // general status values
  mqttHaConfig(KM_STATUS, stat_topics.OUTSIDE_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  mqttHaConfig(KM_STATUS, stat_topics.OUTSIDE_TEMP_DAMPED[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT,
               textPar());
  mqttHaConfig(KM_STATUS, stat_topics.EXHAUST_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());

  // optional Sensors
  if (config.sensor.ch1_enable) {
    char topic1[32];
    replace_whitespace(config.sensor.ch1_name, topic1, sizeof(topic1));
    mqttHaConfig(KM_SENS, topic1, "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  }
  if (config.sensor.ch2_enable) {
    char topic2[32];
    replace_whitespace(config.sensor.ch2_name, topic2, sizeof(topic2));
    mqttHaConfig(KM_SENS, topic2, "temperature", "sensor", "°C", NULL, "mdi:thermometer", TYP_TEXT, textPar());
  }

  // Oilcounter
  if (config.oilmeter.use_hardware_meter) {
    mqttHaConfig(KM_OIL, "oilcounter", NULL, "sensor", NULL, NULL, "mdi:barrel-outline", TYP_TEXT, textPar());
  }

  // Alarm
  if (config.km271.use_alarmMsg) {
    mqttHaConfig(KM_ALARM, errTopics.ERR_BUFF_1[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_ALARM, errTopics.ERR_BUFF_2[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_ALARM, errTopics.ERR_BUFF_3[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:alert-circle-outline", TYP_TEXT, textPar());
    mqttHaConfig(KM_ALARM, errTopics.ERR_BUFF_4[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:alert-circle-outline", TYP_TEXT, textPar());
  }

  // DEBUG INFO
  mqttHaConfig(KM_WIFI, "wifi_signal", NULL, "sensor", "%", "{{ value_json.signal }}", "mdi:signal", TYP_TEXT, textPar());
  mqttHaConfig(KM_WIFI, "wifi_rssi", NULL, "sensor", "dbm", "{{ value_json.rssi }}", "mdi:signal", TYP_TEXT, textPar());
  mqttHaConfig(KM_WIFI, "wifi_ip", NULL, "sensor", NULL, "{{ value_json.ip }}", "mdi:ip-outline", TYP_TEXT, textPar());

  mqttHaConfig(KM_DEBUG, "logmode", NULL, "sensor", NULL, "{{ value_json.logmode }}", "mdi:connection", TYP_TEXT, textPar());
  mqttHaConfig(KM_DEBUG, "sw_version", NULL, "sensor", NULL, "{{ value_json.sw_version }}", "mdi:github", TYP_TEXT, textPar());

  mqttHaConfig(KM_SYSINFO, "uptime", NULL, "sensor", NULL, "{{ value_json.uptime }}", "mdi:clock-outline", TYP_TEXT, textPar());
  mqttHaConfig(KM_SYSINFO, "restart_reason", NULL, "sensor", NULL, "{{ value_json.restart_reason }}", "mdi:information-outline", TYP_TEXT, textPar());
  mqttHaConfig(KM_SYSINFO, "heap", NULL, "sensor", "%", "{{ value_json.heap.split(' ')[0] }}", "mdi:memory", TYP_TEXT, textPar());
  mqttHaConfig(KM_SYSINFO, "flash", NULL, "sensor", "%", "{{ value_json.flash.split(' ')[0] }}", "mdi:harddisk", TYP_TEXT, textPar());
}

/**
 * *******************************************************************
 * @brief   mqttDiscovery Cyclic function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttDiscoveryCyclic() {

  if (sendMqttConfig) {
    mqttDiscoverySetup();
    sendMqttConfig = false;
  }
  if (resetMqttConfig) {
    mqttDiscoverySetup();
    resetMqttConfig = false;
  }
}