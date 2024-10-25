// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include <mqtt_client.h>

#include <WString.h>
#include <esp32-hal-log.h>
#include <functional>
#include <vector>

#define MYCILA_MQTT_VERSION          "4.2.2"
#define MYCILA_MQTT_VERSION_MAJOR    4
#define MYCILA_MQTT_VERSION_MINOR    2
#define MYCILA_MQTT_VERSION_REVISION 2

#ifndef MYCILA_MQTT_RECONNECT_INTERVAL
  #define MYCILA_MQTT_RECONNECT_INTERVAL 5
#endif

#ifndef MYCILA_MQTT_CLEAN_SESSION
  #define MYCILA_MQTT_CLEAN_SESSION false
#endif

#ifndef MYCILA_MQTT_TASK_PRIORITY
  #define MYCILA_MQTT_TASK_PRIORITY 5
#endif

#ifndef MYCILA_MQTT_STACK_SIZE
  #define MYCILA_MQTT_STACK_SIZE 4096
#endif

#ifndef MYCILA_MQTT_BUFFER_SIZE
  #define MYCILA_MQTT_BUFFER_SIZE 1024
#endif

#ifndef MYCILA_MQTT_NETWORK_TIMEOUT
  #define MYCILA_MQTT_NETWORK_TIMEOUT 5
#endif

#ifndef MYCILA_MQTT_RETRANSMIT_TIMEOUT
  #define MYCILA_MQTT_RETRANSMIT_TIMEOUT 1
#endif

#ifndef MYCILA_MQTT_OUTBOX_SIZE
  #define MYCILA_MQTT_OUTBOX_SIZE 0
#endif

#ifndef MYCILA_MQTT_KEEPALIVE
  #define MYCILA_MQTT_KEEPALIVE 60
#endif

#define MYCILA_MQTT_TASK_NAME "mqtt_task"

namespace Mycila {
  class MQTT {
    public:
      enum class State {
        // CONNECTED -> DISABLED
        // DISCONNECTED -> DISABLED
        // PUBLISHING -> DISABLED
        MQTT_DISABLED,
        // DISABLED -> CONNECTING
        MQTT_CONNECTING,
        // CONNECTING -> CONNECTED
        MQTT_CONNECTED,
        // CONNECTED -> DISCONNECTED
        // PUBLISHING -> DISCONNECTED
        MQTT_DISCONNECTED,
      };

      typedef std::function<void(const String& topic, const String& payload)> MessageCallback;
      typedef std::function<void()> ConnectedCallback;

      typedef struct
      {
          String topic;
          MessageCallback callback;
      } MQTTMessageListener;

      typedef struct {
          String server = emptyString;
          uint16_t port = 1883;
          bool secured = false;
          const uint8_t* certBundle = nullptr;
          size_t certBundleSize = 0;
          String serverCert = emptyString;
          String username = emptyString;
          String password = emptyString;
          String clientId = emptyString;
          String willTopic = emptyString;
          uint16_t keepAlive = MYCILA_MQTT_KEEPALIVE;
      } Config;

      ~MQTT() { end(); }

      void begin(const Config& config);
      void end();

      void setAsync(bool async) { _async = async; }
      bool isAsync() { return _async; }

      void subscribe(const char* topic, MessageCallback callback);
      void subscribe(const String& topic, MessageCallback callback) { subscribe(topic.c_str(), callback); }

      void unsubscribe(const char* topic);
      void unsubscribe(const String& topic) { unsubscribe(topic.c_str()); }

      void onConnect(ConnectedCallback callback) { _onConnect = callback; }

      bool publish(const char* topic, const char* payload, bool retain = false);
      inline bool publish(const String& topic, const String& payload, bool retain = false) {
        return publish(topic.c_str(), payload.c_str(), retain);
      }
      inline bool publish(const char* topic, const String& payload, bool retain = false) {
        return publish(topic, payload.c_str(), retain);
      }

      bool isEnabled() { return _state != State::MQTT_DISABLED; }
      bool isConnected() { return _state == State::MQTT_CONNECTED; }
      const char* getLastError() { return _lastError; }

    private:
      esp_mqtt_client_handle_t _mqttClient = nullptr;
      State _state = State::MQTT_DISABLED;
      ConnectedCallback _onConnect = nullptr;
      std::vector<MQTTMessageListener> _listeners;
      Config _config;
      const char* _lastError = nullptr;
      bool _async = false;

    private:
      static void _mqttEventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
      static bool _topicMatches(const char* subscribed, const char* topic);
  };
} // namespace Mycila
