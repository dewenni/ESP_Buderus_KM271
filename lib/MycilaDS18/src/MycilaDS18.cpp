// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <MycilaDS18.h>

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

#define TAG "DS18"

#ifndef GPIO_IS_VALID_OUTPUT_GPIO
  #define GPIO_IS_VALID_OUTPUT_GPIO(gpio_num) ((gpio_num >= 0) && \
                                               (((1ULL << (gpio_num)) & SOC_GPIO_VALID_OUTPUT_GPIO_MASK) != 0))
#endif

static const char* err_desc[] = {"", "CRC ERROR", "BAD DATA", "TIMEOUT", "DRIVER NOT INITIALIZED"};

void Mycila::DS18::begin(const int8_t pin, uint8_t maxSearchCount) {
  if (_enabled)
    return;

  if (GPIO_IS_VALID_OUTPUT_GPIO(pin)) {
    _pin = (gpio_num_t)pin;
  } else {
    LOGE(TAG, "Disable DS18 Sensor: Invalid pin: %" PRId8, pin);
    _pin = GPIO_NUM_NC;
    return;
  }

  _oneWire = new OneWire32(_pin);

  LOGD(TAG, "Searching for DS18 sensor on pin: %" PRId8 "...", pin);
  while (maxSearchCount-- > 0 && !_oneWire->search(&_deviceAddress, 1)) {
    vTaskDelay(portTICK_PERIOD_MS);
  }

  if (!_deviceAddress) {
    LOGE(TAG, "No DS18 sensor found on pin: %" PRId8, pin);
    return;
  }
  _name = getModel();

  LOGD(TAG, "Found %s sensor at address 0x%llx on pin: %" PRId8 " (remaining search count: %d)", _name, _deviceAddress, _pin, maxSearchCount);

  _oneWire->request();

  LOGI(TAG, "%s 0x%llx @ pin %d enabled!", _name, _deviceAddress, _pin);
  _enabled = true;
}

void Mycila::DS18::end() {
  if (_enabled) {
    std::lock_guard<std::mutex> lock(_mutex);
    _enabled = false;
    delete _oneWire;
    _temperature = 0;
    _lastTime = 0;
    _pin = GPIO_NUM_NC;
    _deviceAddress = 0;
    LOGI(TAG, "%s 0x%llx @ pin %d disabled!", _name, _deviceAddress, _pin);
  }
}

bool Mycila::DS18::read() {
  if (!_enabled)
    return false;

  std::lock_guard<std::mutex> lock(_mutex);

  float read;
  uint8_t err = _oneWire->getTemp(_deviceAddress, read);

  // request new reading
  _oneWire->request();

  // process data when no error
  if (err) {
    LOGW(TAG, "%s 0x%llx @ pin %d: read error: %s", _name, _deviceAddress, _pin, err_desc[err]);
    return false;
  }

  // discard any invalid read
  if (isnan(read))
    return false;

  // read is valid, record the time
  _lastTime = millis();

  // make it on 2 decimals
  read = round(read * 100) / 100;

  const bool changed = abs(read - _temperature) >= MYCILA_DS18_RELEVANT_TEMPERATURE_CHANGE || !isValid();

  if (changed) {
    _temperature = read;
    LOGD(TAG, "%s 0x%llx @ pin %d: %.2f °C", _name, _deviceAddress, _pin, read);
  }

  if (_callback)
    _callback(_temperature, changed);

  return true;
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::DS18::toJson(const JsonObject& root) const {
  root["elapsed"] = getElapsedTime();
  root["enabled"] = _enabled;
  root["expired"] = isExpired();
  root["temp"] = getTemperature().value_or(0);
  root["time"] = _lastTime;
  root["valid"] = isValid();
}
#endif
