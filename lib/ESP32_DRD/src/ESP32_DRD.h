#pragma once

#ifndef ESP32_DRD_H
#define ESP32_DRD_H

#include <Arduino.h>
#include <LittleFS.h>

#define DRD_FILENAME "/drd.dat"

#define DOUBLERESETDETECTOR_FLAG_SET 0xD0D01234
#define DOUBLERESETDETECTOR_FLAG_CLEAR 0xD0D04321



class DRD32 {
public:
  DRD32(int timeout) {

    // LittleFS
    if (!LittleFS.begin()) {
      ESP_LOGD(TAG, "LittleFS failed!");
    }

    this->timeout = timeout * 1000;
    doubleResetDetected = false;
    waitingForDoubleReset = false;
  };

  bool detectDoubleReset() {
    doubleResetDetected = detectRecentlyResetFlag();

    if (doubleResetDetected) {
      ESP_LOGI(TAG, "doubleResetDetected");
      clearRecentlyResetFlag();
    } else {
      ESP_LOGI(TAG, "No doubleResetDetected");

      setRecentlyResetFlag();
      waitingForDoubleReset = true;
    }

    return doubleResetDetected;
  };

  bool waitingForDRD() { return waitingForDoubleReset; }

  void loop() {
    if (waitingForDoubleReset && millis() > timeout) {
      ESP_LOGD(TAG, "Stop doubleResetDetecting");
      stop();
    }
  };

  void stop() {
    clearRecentlyResetFlag();
    waitingForDoubleReset = false;
  };

  bool doubleResetDetected;

private:
  const char *TAG = "DRD";
  uint32_t DOUBLERESETDETECTOR_FLAG;
  unsigned long timeout;
  int address;
  bool waitingForDoubleReset;

  bool detectRecentlyResetFlag() {
    if (LittleFS.exists(DRD_FILENAME)) {
      // if config file exists, load
      File file = LittleFS.open(DRD_FILENAME, "r");
      if (!file) {
        ESP_LOGD(TAG, "Loading config file failed");
      }

      file.readBytes((char *)&DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
      doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG;
      ESP_LOGD(TAG, "LittleFS Flag read = 0x%lX\n", DOUBLERESETDETECTOR_FLAG);
      file.close();
    }

    doubleResetDetected = (doubleResetDetectorFlag == DOUBLERESETDETECTOR_FLAG_SET);
    return doubleResetDetected;
  };

  void setRecentlyResetFlag() {
    doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG_SET;
    DOUBLERESETDETECTOR_FLAG = DOUBLERESETDETECTOR_FLAG_SET;
    File file = LittleFS.open(DRD_FILENAME, "w");
    ESP_LOGD(TAG, "Saving config file...");
    if (file) {
      file.write((uint8_t *)&DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
      file.close();
      ESP_LOGD(TAG, "Saving config file OK");
    } else {
      ESP_LOGD(TAG, "Saving config file failed");
    }
  };

  void clearRecentlyResetFlag() {
    doubleResetDetectorFlag = DOUBLERESETDETECTOR_FLAG_CLEAR;
    DOUBLERESETDETECTOR_FLAG = DOUBLERESETDETECTOR_FLAG_CLEAR;

    // LittleFS / SPIFFS code
    File file = LittleFS.open(DRD_FILENAME, "w");
    ESP_LOGD(TAG, "Saving config file...");

    if (file) {
      file.write((uint8_t *)&DOUBLERESETDETECTOR_FLAG, sizeof(DOUBLERESETDETECTOR_FLAG));
      file.close();
      ESP_LOGD(TAG, "Saving config file OK");
    } else {
      ESP_LOGD(TAG, "Saving config file failed");
    }
  };
  uint32_t doubleResetDetectorFlag;
};
#endif