// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <MycilaMQTT.h>

#include <Arduino.h>
#include <esp_crt_bundle.h>

#include <algorithm>
#include <functional>

#ifdef MYCILA_LOGGER_SUPPORT
  #include <MycilaLogger.h>
extern Mycila::Logger logger;
  #define LOGD(tag, format, ...) logger.debug(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) logger.info(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) logger.warn(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) logger.error(tag, format, ##__VA_ARGS__)
#else
  #define LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
  #define LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
  #define LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
  #define LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#endif

#define TAG "MQTT"

void Mycila::MQTT::begin(const MQTT::Config& config) {
  if (_state != MQTT::State::MQTT_DISABLED)
    return;

  // copy config
  _config = config;

  if (_config.server.isEmpty() || _config.port <= 0) {
    LOGE(TAG, "MQTT disabled: Invalid server, port or base topic");
    return;
  }

  LOGI(TAG, "Enable MQTT...");

  const bool auth = !_config.username.isEmpty() && !_config.password.isEmpty();

#if ESP_IDF_VERSION_MAJOR == 5
  if (_config.certBundle) {
    esp_crt_bundle_set(_config.certBundle, _config.certBundleSize);
  }
  const esp_mqtt_client_config_t cfg = {
    .broker = {
      .address = {
        .uri = nullptr,
        .hostname = _config.server.c_str(),
        .transport = _config.secured ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
        .path = nullptr,
        .port = _config.port,
      },
      .verification = {
        .use_global_ca_store = false,
        .crt_bundle_attach = _config.secured && _config.certBundle ? esp_crt_bundle_attach : nullptr,
        .certificate = _config.secured && !_config.serverCert.isEmpty() ? _config.serverCert.c_str() : nullptr,
        .certificate_len = 0,
        .psk_hint_key = nullptr,
        .skip_cert_common_name_check = true,
        .alpn_protos = nullptr,
        .common_name = nullptr,
      },
    },
    .credentials = {
      .username = auth ? _config.username.c_str() : nullptr,
      .client_id = _config.clientId.c_str(),
      .set_null_client_id = false,
      .authentication = {
        .password = auth ? _config.password.c_str() : nullptr,
        .certificate = nullptr,
        .certificate_len = 0,
        .key = nullptr,
        .key_len = 0,
        .key_password = nullptr,
        .key_password_len = 0,
        .use_secure_element = false,
        .ds_data = nullptr,
      },
    },
    .session = {
      .last_will = {
        .topic = _config.willTopic.c_str(),
        .msg = "offline",
        .msg_len = 7,
        .qos = 0,
        .retain = true,
      },
      .disable_clean_session = !MYCILA_MQTT_CLEAN_SESSION,
      .keepalive = _config.keepAlive,
      .disable_keepalive = false,
      .protocol_ver = esp_mqtt_protocol_ver_t::MQTT_PROTOCOL_UNDEFINED,
      .message_retransmit_timeout = MYCILA_MQTT_RETRANSMIT_TIMEOUT * 1000,
    },
    .network = {
      .reconnect_timeout_ms = MYCILA_MQTT_RECONNECT_INTERVAL * 1000,
      .timeout_ms = MYCILA_MQTT_NETWORK_TIMEOUT * 1000,
      .refresh_connection_after_ms = 0,
      .disable_auto_reconnect = false,
      .transport = nullptr,
      .if_name = nullptr,
    },
    .task = {
      .priority = MYCILA_MQTT_TASK_PRIORITY,
      .stack_size = MYCILA_MQTT_STACK_SIZE,
    },
    .buffer = {
      .size = MYCILA_MQTT_BUFFER_SIZE,
      .out_size = MYCILA_MQTT_BUFFER_SIZE,
    },
    .outbox = {
      .limit = MYCILA_MQTT_OUTBOX_SIZE,
    },
  };
#else
  if (_config.certBundle) {
    arduino_esp_crt_bundle_set(_config.certBundle);
  }
  const esp_mqtt_client_config_t cfg = {
    .event_handle = nullptr,
    .event_loop_handle = nullptr,
    .host = _config.server.c_str(),
    .uri = nullptr,
    .port = _config.port,
    .set_null_client_id = false,
    .client_id = _config.clientId.c_str(),
    .username = auth ? _config.username.c_str() : nullptr,
    .password = auth ? _config.password.c_str() : nullptr,
    .lwt_topic = _config.willTopic.c_str(),
    .lwt_msg = "offline",
    .lwt_qos = 0,
    .lwt_retain = true,
    .lwt_msg_len = 7,
    .disable_clean_session = !MYCILA_MQTT_CLEAN_SESSION,
    .keepalive = _config.keepAlive,
    .disable_auto_reconnect = false,
    .user_context = nullptr,
    .task_prio = MYCILA_MQTT_TASK_PRIORITY,
    .task_stack = MYCILA_MQTT_STACK_SIZE,
    .buffer_size = MYCILA_MQTT_BUFFER_SIZE,
    .cert_pem = _config.secured && !_config.serverCert.isEmpty() ? _config.serverCert.c_str() : nullptr,
    .cert_len = 0,
    .client_cert_pem = nullptr,
    .client_cert_len = 0,
    .client_key_pem = nullptr,
    .client_key_len = 0,
    .transport = _config.secured ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
    .refresh_connection_after_ms = 0,
    .psk_hint_key = nullptr,
    .use_global_ca_store = false,
    .crt_bundle_attach = _config.secured && _config.certBundle ? arduino_esp_crt_bundle_attach : nullptr,
    .reconnect_timeout_ms = MYCILA_MQTT_RECONNECT_INTERVAL * 1000,
    .alpn_protos = nullptr,
    .clientkey_password = nullptr,
    .clientkey_password_len = 0,
    .protocol_ver = esp_mqtt_protocol_ver_t::MQTT_PROTOCOL_UNDEFINED,
    .out_buffer_size = MYCILA_MQTT_BUFFER_SIZE,
    .skip_cert_common_name_check = true,
    .use_secure_element = false,
    .ds_data = nullptr,
    .network_timeout_ms = MYCILA_MQTT_NETWORK_TIMEOUT * 1000,
    .disable_keepalive = false,
    .path = nullptr,
    .message_retransmit_timeout = MYCILA_MQTT_RETRANSMIT_TIMEOUT * 1000,
  };
#endif

  _lastError = nullptr;
  _mqttClient = esp_mqtt_client_init(&cfg);
  if (!_mqttClient) {
    LOGE(TAG, "Failed to create MQTT client");
    return;
  }
  ESP_ERROR_CHECK(esp_mqtt_client_register_event(_mqttClient, MQTT_EVENT_ANY, _mqttEventHandler, this));
  ESP_ERROR_CHECK(esp_mqtt_client_start(_mqttClient));
  _state = MQTT::State::MQTT_CONNECTING;
}

void Mycila::MQTT::end() {
  if (_state == MQTT::State::MQTT_DISABLED)
    return;

  LOGI(TAG, "Disable MQTT...");
  esp_mqtt_client_publish(_mqttClient, _config.willTopic.c_str(), "offline", 7, 0, true);
  _state = MQTT::State::MQTT_DISABLED;
  esp_mqtt_client_disconnect(_mqttClient);
  esp_mqtt_client_stop(_mqttClient);
  esp_mqtt_client_destroy(_mqttClient);
  _mqttClient = nullptr;
}

bool Mycila::MQTT::publish(const char* topic, const char* payload, bool retain) {
  if (!isConnected())
    return false;
  if (_async)
    return esp_mqtt_client_enqueue(_mqttClient, topic, payload, 0, 0, retain, true) >= 0;
  else
    return esp_mqtt_client_publish(_mqttClient, topic, payload, 0, 0, retain) >= 0;
}

void Mycila::MQTT::subscribe(const char* topic, MQTT::MessageCallback callback) {
  _listeners.push_back({topic, callback});
  if (isConnected()) {
    LOGD(TAG, "Subscribing to: %s...", topic);
    esp_mqtt_client_subscribe(_mqttClient, topic, 0);
  }
}

void Mycila::MQTT::unsubscribe(const char* topic) {
  LOGD(TAG, "Unsubscribing from: %s...", topic);
  esp_mqtt_client_unsubscribe(_mqttClient, topic);
  remove_if(_listeners.begin(), _listeners.end(), [topic](const MQTTMessageListener& listener) {
    return listener.topic.equals(topic);
  });
}

void Mycila::MQTT::_mqttEventHandler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t mqttClient = event->client;
  Mycila::MQTT* mqtt = (Mycila::MQTT*)event_handler_arg;
  switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_ERROR:
      switch (event->error_handle->error_type) {
        case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
#ifdef MYCILA_MQTT_DEBUG
          LOGD(TAG, "MQTT_EVENT_ERROR: Connection refused");
#endif
          mqtt->_lastError = "Connection refused";
          break;
        case MQTT_ERROR_TYPE_TCP_TRANSPORT:
#ifdef MYCILA_MQTT_DEBUG
          LOGD(TAG, "MQTT_EVENT_ERROR: TCP transport error: %s", strerror(event->error_handle->esp_transport_sock_errno));
#endif
          mqtt->_lastError = "TCP transport error";
          break;
        default:
#ifdef MYCILA_MQTT_DEBUG
          LOGD(TAG, "MQTT_EVENT_ERROR: Unknown error");
#endif
          mqtt->_lastError = "Unknown error";
          break;
      }
      break;
    case MQTT_EVENT_CONNECTED:
      mqtt->_state = MQTT::State::MQTT_CONNECTED;
      mqtt->publish(mqtt->_config.willTopic.c_str(), "online", true);
#ifdef MYCILA_MQTT_DEBUG
      LOGD(TAG, "MQTT_EVENT_CONNECTED: Subscribing to %u topics...", mqtt->_listeners.size());
#endif
      for (auto& _listener : mqtt->_listeners) {
        String t = _listener.topic;
        esp_mqtt_client_subscribe(mqttClient, t.c_str(), 0);
#ifdef MYCILA_MQTT_DEBUG
        LOGD(TAG, "MQTT_EVENT_CONNECTED: %s", t.c_str());
#endif
      }
      if (mqtt->_onConnect)
        mqtt->_onConnect();
      break;
    case MQTT_EVENT_DISCONNECTED:
#ifdef MYCILA_MQTT_DEBUG
      LOGD(TAG, "MQTT_EVENT_DISCONNECTED");
#endif
      mqtt->_state = MQTT::State::MQTT_DISCONNECTED;
      break;
    case MQTT_EVENT_SUBSCRIBED:
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      break;
    case MQTT_EVENT_DATA: {
      String topic;
      topic.reserve(event->topic_len + 1);
      topic.concat((const char*)event->topic, event->topic_len);
      String data;
      data.reserve(event->data_len + 1);
      data.concat((const char*)event->data, event->data_len);
#ifdef MYCILA_MQTT_DEBUG
      LOGD(TAG, "MQTT_EVENT_DATA: %s %s", topic.c_str(), data.c_str());
#endif
      for (auto& listener : mqtt->_listeners)
        if (_topicMatches(listener.topic.c_str(), topic.c_str()))
          listener.callback(topic, data);
      break;
    }
    case MQTT_EVENT_BEFORE_CONNECT:
#ifdef MYCILA_MQTT_DEBUG
      LOGD(TAG, "MQTT_EVENT_BEFORE_CONNECT");
#endif
      mqtt->_state = MQTT::State::MQTT_CONNECTING;
      break;
    case MQTT_EVENT_DELETED:
// see OUTBOX_EXPIRED_TIMEOUT_MS and MQTT_REPORT_DELETED_MESSAGES
#ifdef MYCILA_MQTT_DEBUG
      LOGD(TAG, "MQTT_EVENT_DELETED: %d", event->msg_id);
#endif
      break;
    default:
      break;
  }
}

bool Mycila::MQTT::_topicMatches(const char* sub, const char* topic) {
  // LOGD(TAG, "Match: %s vs %s ?", sub, topic);
  size_t spos;

  if (!sub || !topic || sub[0] == 0 || topic[0] == 0)
    return false;

  if ((sub[0] == '$' && topic[0] != '$') || (topic[0] == '$' && sub[0] != '$'))
    return false;

  spos = 0;

  while (sub[0] != 0) {
    if (topic[0] == '+' || topic[0] == '#')
      return false;

    if (sub[0] != topic[0] || topic[0] == 0) { /* Check for wildcard matches */
      if (sub[0] == '+') {
        /* Check for bad "+foo" or "a/+foo" subscription */
        if (spos > 0 && sub[-1] != '/')
          return false;

        /* Check for bad "foo+" or "foo+/a" subscription */
        if (sub[1] != 0 && sub[1] != '/')
          return false;

        spos++;
        sub++;
        while (topic[0] != 0 && topic[0] != '/') {
          if (topic[0] == '+' || topic[0] == '#')
            return false;
          topic++;
        }
        if (topic[0] == 0 && sub[0] == 0)
          return true;
      } else if (sub[0] == '#') {
        /* Check for bad "foo#" subscription */
        if (spos > 0 && sub[-1] != '/')
          return false;

        /* Check for # not the final character of the sub, e.g. "#foo" */
        if (sub[1] != 0)
          return false;
        else {
          while (topic[0] != 0) {
            if (topic[0] == '+' || topic[0] == '#')
              return false;
            topic++;
          }
          return true;
        }
      } else {
        /* Check for e.g. foo/bar matching foo/+/# */
        if (topic[0] == 0 && spos > 0 && sub[-1] == '+' && sub[0] == '/' && sub[1] == '#')
          return true;

        /* There is no match at this point, but is the sub invalid? */
        while (sub[0] != 0) {
          if (sub[0] == '#' && sub[1] != 0)
            return false;
          spos++;
          sub++;
        }

        /* Valid input, but no match */
        return false;
      }
    } else {
      /* sub[spos] == topic[tpos] */
      if (topic[1] == 0) {
        /* Check for e.g. foo matching foo/# */
        if (sub[1] == '/' && sub[2] == '#' && sub[3] == 0)
          return true;
      }
      spos++;
      sub++;
      topic++;
      if (sub[0] == 0 && topic[0] == 0)
        return true;
      else if (topic[0] == 0 && sub[0] == '+' && sub[1] == 0) {
        if (spos > 0 && sub[-1] != '/')
          return false;
        spos++;
        sub++;
        return true;
      }
    }
  }
  return false;
}
