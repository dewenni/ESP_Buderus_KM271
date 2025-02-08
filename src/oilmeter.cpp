//*****************************************************************************
//
// Title      : optional handling of Oil Meter to measure oil consumtion
// hardware   : Braun HZ5DR / HZ5R / HZ5 (modified with reed contact)
// Remark     : if you dont want to use this, you can disable this in config.h
//*****************************************************************************
#include <EEPROM.h>
#include <basics.h>
#include <config.h>
#include <message.h>
#include <mqtt.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <oilmeter.h>

/* V A R I A B L E S ********************************************************/
static int writeCounter = 0; // counter for write to NVS
static bool reboot = true;   // flag for reboot
static char tmpMsg[300] = {'\0'};
static const char *TAG = "OIL"; // LOG TAG

#define NVS_NAMESPACE "oilmeter_data"
#define OILTRIGGER_TIME 1000       // 1.000 = 1sec
#define OILCYCLICINFO_TIME 3600000 // 360.000 = 1 hour

muTimer oilTrigger = muTimer();    // timer for debouncing trigger input
muTimer oilCyclicInfo = muTimer(); // timer for cyclic information

/**
 * *******************************************************************
 * @brief   Publish actual Oilmeter values via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendOilmeter() {
  // publish actual value
  mqttPublish(addTopic("/oilcounter"), EspStrUtil::intToString(data.oilcounter), true);
}

/**
 * *******************************************************************
 * @brief   get actual Oilmeter value
 * @param   none
 * @return  none
 * *******************************************************************/
long getOilmeter() { return data.oilcounter; }

/**
 * *******************************************************************
 * @brief   Set new value for Oilcounter and safe to EEPROM
 * @param   setvalue  new setvalue
 * @return  none
 * *******************************************************************/
void cmdSetOilmeter(long setvalue) {

  data.oilcounter = setvalue;
  cmdStoreOilmeter();

  snprintf(tmpMsg, sizeof(tmpMsg), "oilcounter was set to: %ld", data.oilcounter);
  km271Msg(KM_TYP_MESSAGE, tmpMsg, "");

  sendOilmeter();
}

/**
 * *******************************************************************
 * @brief   store actual Oilcountervalue to EEPROM
 * @param   none
 * @return  none
 * *******************************************************************/
void cmdStoreOilmeter() {

  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return;
  }

  nvs_set_u32(nvs_handle, "oilcounter", data.oilcounter);
  nvs_commit(nvs_handle);
  nvs_close(nvs_handle);
}

// ####################################################################
//  get Device Counter
// ####################################################################
void cmdLoadOilmeter() {
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS");
    return;
  }

  err = nvs_get_i32(nvs_handle, "oilcounter", &data.oilcounter);

  if (err == ESP_ERR_NVS_NOT_FOUND) {
    // try to read from EEPROM if not found in NVS (migration)
    EEPROM.get(0, data);

    // save to NVS for future use
    nvs_set_i32(nvs_handle, "oilcounter", data.oilcounter);
    nvs_commit(nvs_handle);
    ESP_LOGI(TAG, "Migrated oilcounter=%lu from EEPROM to NVS", data.oilcounter);
  }

  nvs_close(nvs_handle);
  return;
}

/**
 * *******************************************************************
 * @brief   Basic Setup for Oilcounter components
 * @param   none
 * @return  none
 * *******************************************************************/
void setupOilmeter() {

  cmdLoadOilmeter(); // load oilcounter from NVS

  ESP_LOGI(TAG, "restored value from Flash: %lu", data.oilcounter);

  snprintf(tmpMsg, sizeof(tmpMsg), "oilcounter was set to: %lu", data.oilcounter);
  km271Msg(KM_TYP_MESSAGE, tmpMsg, "");
}

/**
 * *******************************************************************
 * @brief   Cyclic Oilcounter function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicOilmeter() {
  if (config.gpio.led_oilcounter != -1) {
    digitalWrite(config.gpio.led_oilcounter, digitalRead(config.gpio.trigger_oilcounter)); // signal for every incoming impulse with Onboard LED
  }

  if (config.gpio.trigger_oilcounter != -1) {
    // debounce input trigger with timer function
    bool statusTrigger = oilTrigger.delayOnOffTrigger(!digitalRead(config.gpio.trigger_oilcounter), 0, OILTRIGGER_TIME) == 1;
    if (statusTrigger && !reboot) {
      data.oilcounter = data.oilcounter + 2; // one impulse = 0,02 litre
      writeCounter++;                        // increase counter for writing to EEPROM
      sendOilmeter();                        // send new Countervalue via MQTT
    }
  }

  if (writeCounter >= 25) // save value every 25 counts = 0,5 liter)
  {
    writeCounter = 0;
    cmdStoreOilmeter(); // store new value in flash
  }

  // send cyclic infos
  if (oilCyclicInfo.cycleTrigger(OILCYCLICINFO_TIME)) // send actual information every x seconds
  {
    sendOilmeter(); // send new Countervalue via MQTT
  }

  reboot = false; // reset reboot flag
}
