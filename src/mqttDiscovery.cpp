#include <basics.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
char discoveryPrefix[32] = {"homeassistant"}; // TODO: read from config
char statePrefix[32] = {"esp-km271-test"};    // TODO: read from variable
char deviceId[32] = {"esp-km271"};
char logamaticVerion[32] = {"3.18"}; // TODO: read from variable
char swVerion[32] = {"4.1.0"};       // TODO: read from variable

muTimer cyclicTimer = muTimer();
s_cfg_topics cfg_topics;
s_stat_topics stat_topics;
s_cfg_arrays cfgOptions;
bool initDone = false;

typedef enum { TYP_TEXT, TYP_SLIDER, TYP_NUM, TYP_OPT } DeviceType;

typedef enum { KM_CONFIG, KM_STATUS, KM_INFO, KM_WIFI, KM_ALARM, KM_DEBUG } KmType;

typedef struct {
  const char *min;
  const char *max;
  const char *step;
  const char *options;
  int numOptions;
} DeviceConfig;

/* P R O T O T Y P E S ********************************************************/

DeviceConfig textPar() { return (DeviceConfig){NULL, NULL, NULL, NULL, 0}; }
DeviceConfig sliderPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, NULL, 0}; }
DeviceConfig numPar(const char *min, const char *max, const char *step) { return (DeviceConfig){min, max, step, NULL, 0}; }
DeviceConfig optPar(const char *options, int numOptions) { return (DeviceConfig){NULL, NULL, NULL, options, numOptions}; }

void mqttHaConfig(KmType kmType, const char *name, const char *deviceClass, const char *component, const char *unit, const char *valueTemplate,
                  const char *icon, DeviceType type, DeviceConfig config) {

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

  doc["name"] = name;

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

  if (type != TYP_TEXT) {
    doc["ent_cat"] = "config";

    char cmdTopic[128];
    sprintf(cmdTopic, "%s/setvalue/%s", statePrefix, name);
    doc["cmd_t"] = cmdTopic;
  }

  if (type == TYP_SLIDER) {
    doc["mode"] = "slider";
    doc["min"] = config.min;
    doc["max"] = config.max;
    doc["step"] = config.step;
  }
  if (type == TYP_NUM) {
    doc["mode"] = "box";
    doc["min"] = config.min;
    doc["max"] = config.max;
    doc["step"] = config.step;
  }

  if (type == TYP_OPT) {
    JsonArray opts = doc["options"].to<JsonArray>();
    for (int i = 0; i < config.numOptions; i++) {
      opts.add(cfgOptions.OPMODE[1][i]);
    }
  }

  // device
  JsonObject deviceObj = doc["dev"].to<JsonObject>();
  deviceObj["name"] = "ESP-Buderus-KM271";
  deviceObj["ids"] = deviceId;
  deviceObj["mf"] = "Buderus";
  deviceObj["mdl"] = "Logamatic";
  deviceObj["sw"] = logamaticVerion;
  // deviceObj["cu"] = actual ip-address

  char jsonString[512];
  serializeJson(doc, jsonString);

  mqttPublish(configTopic, jsonString, false);
}

void mqttDiscoverySetup() {
  if (initDone)
    return;

  mqttHaConfig(KM_CONFIG, cfg_topics.HC1_FROST_THRESHOLD[1], "temperature", "number", "°C", "{{value.split(' ')[0]}}", "mdi:thermometer", TYP_SLIDER,
               sliderPar("-20", "10", "1"));

  mqttHaConfig(KM_CONFIG, cfg_topics.HC1_HOLIDAY_DAYS[1], NULL, "number", "days", "{{value.split(' ')[0]}}", "mdi:calendar", TYP_NUM,
               numPar("0", "99", "1"));

  mqttHaConfig(KM_CONFIG, cfg_topics.HC1_OPMODE[1], NULL, "select", NULL, NULL, "mdi:cog", TYP_OPT, optPar(cfgOptions.OPMODE[1][0], 3));

  initDone = true;
}

void mqttDiscoveryCyclic() {}