#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <message.h>

class Watchdog {
private:
  esp_task_wdt_config_t twdt_config;
  bool isInitialized;
  TaskHandle_t loopTaskHandle; // Handle der Loop-Task

  Watchdog(uint32_t timeout = 10000, uint8_t idle_core_mask = 0b10, bool trigger_panic = true) : isInitialized(false), loopTaskHandle(nullptr) {
    twdt_config = {timeout_ms : timeout, idle_core_mask : idle_core_mask, trigger_panic : trigger_panic};
  }

public:
  static Watchdog &getInstance(uint32_t timeout = 10000, uint8_t idle_core_mask = 0b10, bool trigger_panic = true) {
    static Watchdog instance(timeout, idle_core_mask, trigger_panic);
    return instance;
  }

  void enable() {
    MY_LOGI("WDT", "enable cmd");

    if (isInitialized) {
      MY_LOGW("WDT", "Watchdog timer is already initialized");
      return;
    }

    if (loopTaskHandle == nullptr) {
      loopTaskHandle = xTaskGetCurrentTaskHandle();
    }

    if (esp_task_wdt_reconfigure(&twdt_config) == ESP_OK) {
      esp_task_wdt_add(loopTaskHandle);
      MY_LOGI("WDT", "Watchdog timer initialized for Loop Task");
      isInitialized = true;
    } else {
      MY_LOGE("WDT", "Failed to initialize Watchdog timer");
    }
  }

  void disable() {
    MY_LOGI("WDT", "disable cmd");
    
    if (!isInitialized) {
      MY_LOGW("WDT", "Watchdog timer is not initialized");
      return;
    }

    if (loopTaskHandle == nullptr) {
      loopTaskHandle = xTaskGetCurrentTaskHandle();
    }

    esp_task_wdt_delete(loopTaskHandle);
    MY_LOGI("WDT", "Watchdog timer de-initialized");
    isInitialized = false;
  }

  bool isActive() const { return isInitialized; }
};
