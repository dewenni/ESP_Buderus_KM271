#include <basics.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
char discoveryPrefix[32] = {"homeassistant"}; // TODO: read from config
char statePrefix[32] = {"esp-km271-test"};    // TODO: read from variable
char deviceId[32] = {"esp-km271"};
char swVersion[32] = {"4.1.0"}; // TODO: read from variable

muTimer cyclicTimer = muTimer();
s_cfg_topics cfg_topics;
s_stat_topics stat_topics;
s_cfg_arrays cfgOptions;
bool initDone = false;

typedef enum { OPT_NULL, OPT_OP_MODE, OPT_RED_MODE, OPT_HC_PRG } OptType;

typedef enum { TYP_TEXT, TYP_SLIDER, TYP_NUM, TYP_OPT } DeviceType;

typedef enum { KM_CONFIG, KM_STATUS, KM_INFO, KM_WIFI, KM_ALARM, KM_DEBUG } KmType;

typedef struct {
  const char *min;
  const char *max;
  const char *step;
  OptType optType;
} DeviceConfig;

/* P R O T O T Y P E S ********************************************************/

DeviceConfig textPar() { return (DeviceConfig){NULL, NULL, NULL, OPT_NULL}; }
DeviceConfig optPar(OptType optType) { return (DeviceConfig){NULL, NULL, NULL, optType}; }
DeviceConfig sliderPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, OPT_NULL}; }
DeviceConfig numPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, OPT_NULL}; }

void mqttHaConfig(KmType kmType, const char *name, const char *deviceClass, const char *component, const char *unit, const char *valueTemplate,
                  const char *icon, DeviceType devType, DeviceConfig devCfg) {

  JsonDocument doc;

  char configTopic[128];
  sprintf(configTopic, "%s/%s/%s/%s/config", discoveryPrefix, component, deviceId, name);

  char stateTopic[128];
  switch (kmType) {
  case KM_CONFIG:
    sprintf(stateTopic, "%s/config/%s", statePrefix, name);
    break;
  case KM_STATUS:
    sprintf(stateTopic, "%s/status/%s", statePrefix, name);
    break;
  default:
    break;
  }
  doc["stat_t"] = stateTopic;

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

  if (devType != TYP_TEXT) {
    doc["ent_cat"] = "config";

    char cmdTopic[128];
    sprintf(cmdTopic, "%s/setvalue/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
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
    default:
      break;
    }
  }

  // device
  JsonObject deviceObj = doc["dev"].to<JsonObject>();
  deviceObj["name"] = "Logamatic";
  deviceObj["ids"] = deviceId;
  deviceObj["mf"] = "Buderus";
  deviceObj["mdl"] = "KM271";
  deviceObj["sw"] = swVersion;
  // deviceObj["cu"] = actual ip-address

  char jsonString[512];
  serializeJson(doc, jsonString);

  mqttPublish(configTopic, jsonString, false);
}

void mqttDiscoverySetup() {
  if (initDone)
    return;

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_OPMODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_OP_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_REDUCTION_MODE[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_RED_MODE));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_PROGRAM[config.mqtt.lang], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(OPT_HC_PRG));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_FROST_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SUMMER_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("9", "31", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_NIGHT_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_DAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HOLIDAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_SWITCH_ON_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("0", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_INTERPR[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("30", "90", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HOLIDAY_DAYS[config.mqtt.lang], NULL, "number", "days", "{{value.split(' ')[0]}}", "mdi:calendar", TYP_NUM,
                 numPar("0", "99", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_MAX_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC1_TEMP_OFFSET[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
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

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_FROST_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SUMMER_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("9", "31", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_NIGHT_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_DAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_HOLIDAY_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("10", "30", "0.5"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("-20", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_SWITCH_ON_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}",
                 "mdi:thermometer", TYP_SLIDER, sliderPar("0", "10", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_INTERPR[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("30", "90", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_HOLIDAY_DAYS[config.mqtt.lang], NULL, "number", "days", "{{value.split(' ')[0]}}", "mdi:calendar", TYP_NUM,
                 numPar("0", "99", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_MAX_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_TEXT, textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.HC2_TEMP_OFFSET[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
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

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_TEMP[config.mqtt.lang], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
                 TYP_SLIDER, sliderPar("30", "60", "1"));

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_PROCESSING[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:water-thermometer-outline", TYP_TEXT,
                 textPar());

    mqttHaConfig(KM_CONFIG, cfg_topics.WW_CIRCULATION[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:water-sync", TYP_TEXT, textPar());
  }

  // general config values
  mqttHaConfig(KM_CONFIG, cfg_topics.LANGUAGE[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:translate", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.SCREEN[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:monitor", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_TYP[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:gas-burner", TYP_TEXT, textPar());
  mqttHaConfig(KM_CONFIG, cfg_topics.BUILDING_TYP[config.mqtt.lang], NULL, "sensor", NULL, NULL, "mdi:home", TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.MAX_BOILER_TEMP[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}",
               "mdi:thermometer-alert", TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.PUMP_LOGIC[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer",
               TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.EXHAUST_THRESHOLD[config.mqtt.lang], "temperature", "sensor", "°C", "{{value.split(' ')[0]}}",
               "mdi:thermometer-alert", TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.EXHAUST_THRESHOLD[config.mqtt.lang], NULL, "sensor", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer-alert",
               TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_MIN_MOD[config.mqtt.lang], NULL, "sensor", "%", "{{value.split(' ')[0]}}", "mdi:percent", TYP_TEXT,
               textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.BURNER_MOD_TIME[config.mqtt.lang], "duration", "sensor", "s", "{{value.split(' ')[0]}}", "mdi:clock-outline",
               TYP_TEXT, textPar());

  mqttHaConfig(KM_CONFIG, cfg_topics.TIME_OFFSET[config.mqtt.lang], "duration", "sensor", "h", "{{value.split(' ')[0]}}", "mdi:clock-outline",
               TYP_TEXT, textPar());

  initDone = true;
}
