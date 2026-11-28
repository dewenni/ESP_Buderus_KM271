// The host compiles the complete production discovery file with real ArduinoJson.
// Only configuration, timer ticks and MQTT publication are replaced locally.
#include "discovery.cpp"
#include <iostream>
#include <string>
#include <vector>

struct Publication { std::string topic, payload; bool retained; };
static std::vector<Publication> publications;
void mqttPublish(const char *topic, const char *payload, bool retained) {
  publications.push_back({topic, payload, retained});
}

static void report(const char *phase) {
  const uint8_t lang = config.mqtt.lang;
  const char *names[] = {
    KM_CFG_TOPIC::HC1_MAX_TEMP[lang], KM_CFG_TOPIC::HC1_TEMP_OFFSET[lang], KM_CFG_TOPIC::HC1_HEATING_SYSTEM[lang],
    KM_CFG_TOPIC::HC2_MAX_TEMP[lang], KM_CFG_TOPIC::HC2_TEMP_OFFSET[lang], KM_CFG_TOPIC::HC2_HEATING_SYSTEM[lang],
    KM_CFG_TOPIC::WW_PROCESSING[lang], KM_CFG_TOPIC::WW_PRIO[lang],
  };
  const char *kinds[] = {"max", "offset", "hc1_system", "max", "offset", "hc2_system", "on_off", "on_off"};
  JsonDocument doc;
  doc["phase"] = phase;
  JsonArray messages = doc["messages"].to<JsonArray>();
  for (const auto &publication : publications) {
    JsonObject message = messages.add<JsonObject>();
    message["topic"] = publication.topic;
    message["payload"] = publication.payload;
    message["retained"] = publication.retained;
    const std::string prefix = publication.topic.substr(0, publication.topic.size() - 7);
    const std::string name = prefix.substr(prefix.find_last_of('/') + 1);
    message["name"] = name;
    message["migrated"] = false;
    for (unsigned i = 0; i < 8; ++i) {
      if (name != names[i]) continue;
      message["migrated"] = true;
      message["kind"] = kinds[i];
      message["hc2"] = i >= 3 && i <= 5;
    }
  }
  serializeJson(doc, std::cout);
  std::cout << '\n';
  publications.clear();
}

static void cycles(unsigned count) {
  for (unsigned i = 0; i < count; ++i) mqttDiscoveryCyclic();
}

int main() {
  for (uint8_t lang = 0; lang < 2; ++lang) {
    config.mqtt.lang = lang;
    config.mqtt.ha_topic = "homeassistant";
    config.km271.use_hc1 = config.km271.use_hc2 = config.km271.use_ww = true;
    configValues.hc2_heating_system = 1;
    sendMqttConfig = resetMqttConfig = false;
    mqttSendCnt = 0;
    mqttSendHaCfg_KmCfg(); report("baseline");
    mqttSendHaCfg_KmCfg(); report("repeat");
    configValues.hc2_heating_system = 3;
    cycles(4); report("underfloor");
    configValues.hc2_heating_system = 1;
    cycles(4); report("radiator");
    mqttDiscoverySendConfig();
    cycles(1);
    configValues.hc2_heating_system = 3;
    cycles(7); report("mid_pass_change");
    mqttDiscoveryResetConfig();
    cycles(4); report("reset");
    config.km271.use_hc1 = config.km271.use_hc2 = config.km271.use_ww = false;
    mqttSendHaCfg_KmCfg(); report("disabled");
    config.mqtt.ha_topic = "";
    mqttSendHaCfg_KmCfg(); report("empty_prefix");
  }
}
