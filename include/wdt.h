#include <esp_task_wdt.h>
#include <message.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


class Watchdog {
private:
    esp_task_wdt_config_t twdt_config;
    bool isInitialized;
    TaskHandle_t loopTaskHandle; // Handle der Loop-Task

    Watchdog(uint32_t timeout = 10000, uint8_t idle_core_mask = 0b10, bool trigger_panic = true)
        : isInitialized(false), loopTaskHandle(nullptr) {
        twdt_config = {timeout_ms : timeout, idle_core_mask : idle_core_mask, trigger_panic : trigger_panic};
    }

public:
    static Watchdog& getInstance(uint32_t timeout = 10000, uint8_t idle_core_mask = 0b10, bool trigger_panic = true) {
        static Watchdog instance(timeout, idle_core_mask, trigger_panic);
        return instance;
    }

    void enable() {
    if (isInitialized) {
        MY_LOGW("WDT", "Watchdog timer is already initialized");
        return;
    }

    if (loopTaskHandle == nullptr) {
        loopTaskHandle = xTaskGetCurrentTaskHandle(); 
    }

    if (esp_task_wdt_status(loopTaskHandle) == ESP_OK) {
        esp_task_wdt_deinit();
    }

    if (esp_task_wdt_init(&twdt_config) == ESP_OK) {
        esp_task_wdt_add(loopTaskHandle);
        MY_LOGI("WDT", "Watchdog timer initialized for Loop Task");
        isInitialized = true;
    } else {
        MY_LOGE("WDT", "Failed to initialize Watchdog timer");
    }
}


    void disable() {
        if (!isInitialized) {
            MY_LOGW("WDT", "Watchdog timer is not initialized");
            return;
        }

        esp_task_wdt_delete(loopTaskHandle);
        esp_task_wdt_deinit();
        MY_LOGI("WDT", "Watchdog timer de-initialized");
        isInitialized = false;
    }

    bool isActive() const { return isInitialized; }
};
