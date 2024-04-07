#include <basics.h>
#include <mqtt.h>
#include <mqttDiscovery.h>

/* D E C L A R A T I O N S ****************************************************/
char discoveryPrefix[32] = {"homeassistant"};
char deviceName[32] = {"Heizung"};
muTimer cyclicTimer = muTimer();
s_cfg_topics cfg_topics;
s_stat_topics stat_topics;

bool initDone = false;

/* P R O T O T Y P E S ********************************************************/

void mqttDiscoveryConfig(const char *name, const char *deviceClass, const char *component, const char *unit, const char *valueTemplate,
                         const char *icon) {

  char stateTopic[128];
  sprintf(stateTopic, "%s/%s/%s/state", discoveryPrefix, component, name);

  JsonDocument doc;
  doc["state_topic"] = stateTopic;
  doc["name"] = name;
  doc["unique_id"] = name;
  if (deviceClass) {
    doc["device_class"] = deviceClass;
  }
  if (unit) {
    doc["unit_of_measurement"] = unit;
  }
  if (valueTemplate) {
    doc["value_template"] = valueTemplate;
  }
  if (icon) {
    doc["icon"] = icon;
  }

  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  identifiers.add(deviceName);

  device["manufacturer"] = "Buderus";
  device["model"] = "R2107";
  device["name"] = "ESP-Buderus-KM271";

  char configTopic[128];
  sprintf(configTopic, "%s/%s/%s/config", discoveryPrefix, component, name);

  char jsonString[512];
  serializeJson(doc, jsonString);

  mqttPublish(configTopic, jsonString, false);
}

void mqttDiscoveryState(const char *name, const char *component, const char *value) {

  char stateTopic[128];
  sprintf(stateTopic, "%s/%s/%s/state", discoveryPrefix, component, name);

  mqttPublish(stateTopic, value, false);
}

void mqttDiscoverySetup() {
  if (initDone)
    return;

  mqttDiscoveryConfig(cfg_topics.HC1_FROST_THRESHOLD[0], "temperature", "sensor", "°C", NULL, NULL);
  mqttDiscoveryConfig(cfg_topics.HC1_SUMMER_THRESHOLD[0], "temperature", "sensor", "°C", NULL, NULL);
  mqttDiscoveryConfig(cfg_topics.HC1_OPMODE[0], NULL, "sensor", NULL, NULL, "mdi:tune-variant");
  mqttDiscoveryConfig(stat_topics.HC1_ON_TIME_OPT[0], NULL, "sensor", "min", NULL, "mdi:timer-outline");
  mqttDiscoveryConfig(stat_topics.HC1_MIXER[0], NULL, "sensor", "%", NULL, "mdi:valve");
  initDone = true;
}

void mqttDiscoveryCyclic() {
  if (cyclicTimer.cycleTrigger(5000)) {
    mqttDiscoveryState(cfg_topics.HC1_FROST_THRESHOLD[0], "sensor", "-3");
    mqttDiscoveryState(cfg_topics.HC1_SUMMER_THRESHOLD[0], "sensor", "13");
    mqttDiscoveryState(cfg_topics.HC1_OPMODE[0], "sensor", "auto");
    mqttDiscoveryState(stat_topics.HC1_ON_TIME_OPT[0], "sensor", "34");
    mqttDiscoveryState(stat_topics.HC1_MIXER[0], "sensor", "50");
  }
}