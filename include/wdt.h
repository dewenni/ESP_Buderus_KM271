#pragma once

#include <esp_task_wdt.h>
#include <message.h>

#define WDT_TIMEOUT 10000

static const char *WDT_TAG = "WDT"; // LOG TAG

void enableWdt() {

  esp_task_wdt_deinit();
  esp_task_wdt_config_t twdt_config{timeout_ms : WDT_TIMEOUT, idle_core_mask : 0b10, trigger_panic : true};

  if (esp_task_wdt_init(&twdt_config) == ESP_OK) {
    esp_task_wdt_add(NULL);
    MY_LOGI(WDT_TAG, "Watchdog timer initialized");

  } else {
    MY_LOGE(WDT_TAG, "Failed to initialize Watchdog timer");
  }
}

void disableWdt() {
  esp_task_wdt_deinit();
  esp_task_wdt_delete(NULL);
  MY_LOGI(WDT_TAG, "Watchdog timer de-initialized");
}