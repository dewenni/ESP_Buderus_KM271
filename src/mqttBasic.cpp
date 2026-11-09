// Shared MQTT transport for ESP_Buderus_KM271 and ESP32-Jarolift-Controller.
// Keep this file and mqttBasic.h identical in both projects.
#include <mqttBasic.h>
#include <mqttCustom.h>
#include <basics.h>
#include <WiFiMgn.h>
#include <AsyncMqttClient.h>
#include <atomic>
#include <queue>

static constexpr size_t MAX_MQTT_CMD = 20;
static constexpr uint32_t MQTT_CONNECT_TIMEOUT = 30000;
static const char *TAG = "MQTT";
static AsyncMqttClient mqtt_client;
static bool setupDone = false;
static char lastError[64] = "---";
static int mqtt_retry = 0;
static uint32_t mqttConnectStartedAt = 0;
static uint32_t mqttRetryStartedAt = 0;
static std::atomic<bool> mqttTransportDisconnected{true};
static std::atomic<bool> mqttReconnectRequested{false};
static std::atomic<bool> mqttConnectedPending{false};
static s_cfg_mqtt mqttConnectionConfig;
static char mqttClientId[sizeof(config.wifi.hostname)];
static char mqttWillTopic[256];
static std::queue<s_MqttMessage> mqttCmdQueue;
static SemaphoreHandle_t mqttQueueMutex = nullptr;

// AsyncTCP receives commands on a different task. Use a mutex rather than a
// critical section: std::queue may allocate/free heap memory while it is locked.
static void addMqttCmd(const s_MqttMessage &message) {
  xSemaphoreTake(mqttQueueMutex, portMAX_DELAY);
  bool full = mqttCmdQueue.size() >= MAX_MQTT_CMD;
  if (!full) {
    mqttCmdQueue.push(message);
  }
  xSemaphoreGive(mqttQueueMutex);
  if (full) {
    ESP_LOGW(TAG, "MQTT command queue full; command dropped");
  } else {
    ESP_LOGI(TAG, "msg received | topic: %s | payload: %s", message.topic, message.payload);
  }
}

static void processNextMqttMessage() {
  s_MqttMessage message;
  xSemaphoreTake(mqttQueueMutex, portMAX_DELAY);
  bool available = !mqttCmdQueue.empty();
  if (available) {
    message = mqttCmdQueue.front();
    mqttCmdQueue.pop();
  }
  xSemaphoreGive(mqttQueueMutex);
  if (available) {
    processMqttMessage(message);
  }
}

// AsyncMqttClient can split one MQTT payload across several TCP callbacks.
// Queue complete commands only; never interpret a truncated fragment as a command.
static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                          size_t len, size_t index, size_t total) {
  static s_MqttMessage incoming{};
  static size_t received = 0, expected = 0;
  static bool receiving = false;

  if (index == 0) {
    receiving = false;
    received = 0;
    if (topic != nullptr && strnlen(topic, sizeof(incoming.topic)) < sizeof(incoming.topic) &&
        total < sizeof(incoming.payload)) {
      snprintf(incoming.topic, sizeof(incoming.topic), "%s", topic);
      expected = total;
      receiving = true;
    }
  }
  if (!receiving || topic == nullptr || strcmp(topic, incoming.topic) != 0 || total != expected ||
      index != received || index > total || len > total - index ||
      (len > 0 && (payload == nullptr || memchr(payload, '\0', len) != nullptr))) {
    receiving = false;
    ESP_LOGW(TAG, "Invalid or oversized MQTT command dropped");
    return;
  }
  if (len > 0) {
    memcpy(incoming.payload + index, payload, len);
  }
  received += len;
  if (received == expected) {
    incoming.payload[received] = '\0';
    incoming.len = received;
    receiving = false;
    addMqttCmd(incoming);
  }
}

static void onMqttConnect(bool sessionPresent) {
  ESP_LOGI(TAG, "MQTT connected");
  // Defer subscriptions and device work to the loop task.
  mqttConnectedPending.store(true);
}

static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

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
  mqttConnectedPending.store(false);
  mqttTransportDisconnected.store(true);
}

bool mqttIsConnected() { return mqtt_client.connected(); }

const char *mqttGetLastError() { return lastError; }

void mqttReconnect() {
  if (config.mqtt.enable && !setupMode) {
    mqttReconnectRequested = true;
  }
}

bool mqttIsReconnecting() { return mqttReconnectRequested; }

bool mqttIsConnecting() {
  return mqttReconnectRequested || (!mqttTransportDisconnected.load() && !mqttIsConnected());
}

const s_cfg_mqtt &mqttGetConnectionConfig() { return mqttConnectionConfig; }

void mqttPublish(const char *topic, const char *payload, boolean retained) {
  mqtt_client.publish(topic, 0, retained, payload);
}

void mqttSubscribe(const char *topic) { mqtt_client.subscribe(topic, 0); }

const char *addTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s%s", config.mqtt.topic, suffix);
  return newTopic;
}

const char *addCfgCmdTopic(const char *suffix) {
  static char newTopic[256];
  snprintf(newTopic, sizeof(newTopic), "%s/setvalue/%s", config.mqtt.topic, suffix);
  return newTopic;
}

static void mqttApplySettings() {
  // AsyncMqttClient retains pointers: keep this connection's parameters alive
  // and unchanged while the settings form is being edited.
  mqttConnectionConfig = config.mqtt;
  snprintf(mqttClientId, sizeof(mqttClientId), "%s", config.wifi.hostname);
  snprintf(mqttWillTopic, sizeof(mqttWillTopic), "%s/status", mqttConnectionConfig.topic);
  mqtt_client.setServer(mqttConnectionConfig.server, mqttConnectionConfig.port);
  mqtt_client.setClientId(mqttClientId);
  mqtt_client.setCredentials(mqttConnectionConfig.user[0] ? mqttConnectionConfig.user : nullptr,
                             mqttConnectionConfig.user[0] ? mqttConnectionConfig.password : nullptr);
  mqtt_client.setWill(mqttWillTopic, 0, true, "offline");
}

static void mqttConnect() {
  if (mqttTransportDisconnected.load()) {
    mqttApplySettings();
    mqttConnectStartedAt = millis();
    mqttRetryStartedAt = mqttConnectStartedAt;
    mqttTransportDisconnected.store(false);
    ESP_LOGI(TAG, "MQTT - connection attempt (backoff level %i)", mqtt_retry);
    mqtt_client.connect();
  }
}

static void mqttDisconnect() {
  mqtt_client.disconnect(true);
  // Each connection uses a clean session. Do not carry queued outgoing packets
  // from an abandoned connection/test into the next one.
  mqtt_client.clearQueue();
}

void mqttSetup() {
  if (setupDone) {
    return;
  }
  mqttQueueMutex = xSemaphoreCreateMutex();
  if (mqttQueueMutex == nullptr) {
    ESP_LOGE(TAG, "MQTT setup failed: cannot create command queue mutex");
    return;
  }
  mqtt_client.onConnect(onMqttConnect);
  mqtt_client.onDisconnect(onMqttDisconnect);
  mqtt_client.onMessage(onMqttMessage);
  mqttApplySettings();
  mqtt_client.setKeepAlive(10);
  mqtt_client.setCleanSession(true);
  setupDone = true;
  ESP_LOGI(TAG, "MQTT setup done!");
}

void mqttCyclic() {
  if (EspSysUtil::OTA::getInstance().isActive()) {
    // Pause command execution and connection deadlines during an OTA update.
    mqttConnectStartedAt = mqttRetryStartedAt = millis();
    return;
  }
  if (config.mqtt.enable && !setupMode && !setupDone && (eth.connected || wifi.connected)) {
    mqttSetup();
  }
  if (!setupDone) {
    return;
  }

  processNextMqttMessage();
  if (!config.mqtt.enable || setupMode || !(wifi.connected || eth.connected)) {
    if (!mqttTransportDisconnected.load()) {
      ESP_LOGI(TAG, "MQTT - closing connection: MQTT disabled or network unavailable");
      mqttDisconnect();
    }
    mqtt_retry = 0;
    return;
  }

  if (mqttReconnectRequested.exchange(false)) {
    mqttDisconnect();
    mqtt_retry = 0;
    snprintf(lastError, sizeof(lastError), "---");
    ESP_LOGI(TAG, "MQTT connection test requested");
  }

  if (mqtt_client.connected()) {
    // Keep reconnect bookkeeping on the loop task, not in AsyncTCP callbacks.
    mqtt_retry = 0;
  } else if (!mqttTransportDisconnected.load() &&
             static_cast<uint32_t>(millis() - mqttConnectStartedAt) >= MQTT_CONNECT_TIMEOUT) {
    ESP_LOGW(TAG, "MQTT - connection timed out after %lu ms; resetting MQTT transport", (unsigned long)MQTT_CONNECT_TIMEOUT);
    mqttDisconnect();
    mqttRetryStartedAt = millis();
    snprintf(lastError, sizeof(lastError), "CONNECT TIMEOUT");
  }

  // Automatic reconnect to the mqtt broker if the connection is lost. This
  // never reboots the device: an unreachable broker (Home Assistant being
  // updated, the broker container restarting, a mistyped server address) says
  // nothing about this device's health, and restarting would drop the decoded
  // boiler state, both log ring buffers and the NTP sync without fixing
  // anything. The old behaviour rebooted after ~150 s and kept doing so for
  // the whole outage, which also made it very hard to push an OTA update to
  // correct the setting that caused it. Retries continue indefinitely with a
  // backoff capped at MQTT_RECONNECT * MQTT_RETRY_MAX.
  if (!mqtt_client.connected() && mqttTransportDisconnected.load()) {
    if (mqtt_retry == 0) {
      mqtt_retry++;
      mqttConnect();
    } else if (static_cast<uint32_t>(millis() - mqttRetryStartedAt) >= MQTT_RECONNECT * mqtt_retry) {
      if (mqtt_retry < MQTT_RETRY_MAX) {
        mqtt_retry++; // capped, so the backoff stops growing and cannot overflow
      }
      mqttConnect();
    }
  }

  if (mqtt_client.connected() && mqttConnectedPending.exchange(false)) {
    mqttCustomConnected();
  }
  mqttCustomCyclic();
}
