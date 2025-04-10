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
static bool reboot = true; // flag for reboot
static char tmpMsg[300] = {'\0'};
static const char *TAG = "OIL";             // LOG TAG
static bool errTriggerGpioNotSet = false;   // flag for error message
static bool errPulsePerLiterNotSet = false; // flag for error message

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
 * @brief   Set new value for Oilcounter and safe to FLASH
 * @param   setvalue  new setvalue
 * @return  none
 * *******************************************************************/
void cmdSetOilmeter(long setvalue) {

  data.oilcounter = setvalue;
  cmdStoreOilmeter();

  snprintf(tmpMsg, sizeof(tmpMsg), "oilcounter was set to: %ld = %.2f L", data.oilcounter, ((float)data.oilcounter) / 100.0);
  km271Msg(KM_TYP_MESSAGE, tmpMsg, "");

  sendOilmeter();
}

/**
 * *******************************************************************
 * @brief   store actual Oilcountervalue to FLASH
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

  nvs_set_i32(nvs_handle, "oilcounter", data.oilcounter);
  nvs_commit(nvs_handle);
  nvs_close(nvs_handle);
}

/**
 * *******************************************************************
 * @brief   load Oilcountervalue from FLASH
 * @param   none
 * @return  none
 * *******************************************************************/
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
}

/**
 * *******************************************************************
 * @brief   Cyclic Oilcounter function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicOilmeter() {
  // LED indication: toggle onboard LED with every impulse
  if (config.gpio.led_oilcounter != -1) {
    digitalWrite(config.gpio.led_oilcounter, digitalRead(config.gpio.trigger_oilcounter));
  }

  if (config.gpio.trigger_oilcounter == -1) {
    if (!errTriggerGpioNotSet) {
      ESP_LOGE(TAG, "trigger_oilcounter is not set");
      errTriggerGpioNotSet = true;
    }
    return;
  }
  if (config.oilmeter.pulse_per_liter == 0) {
    if (!errPulsePerLiterNotSet) {
      ESP_LOGE(TAG, "pulse_per_liter is not set");
      errPulsePerLiterNotSet = true;
    }
    return;
  }

  errTriggerGpioNotSet = false;
  errPulsePerLiterNotSet = false;

  // Debounce the input trigger using a timer function
  bool statusTrigger = oilTrigger.delayOnOffTrigger(!digitalRead(config.gpio.trigger_oilcounter), 0, OILTRIGGER_TIME) == 1;

  // Static accumulator for the fractional part (the remaining hundredths per liter)
  static int oil_remainder = 0;

  if (statusTrigger && !reboot) {
    // Objective: 1 liter = 100 hundredths, so each impulse should add (100 / pulse_per_liter) hundredths.
    // Since (100 / pulse_per_liter) may not be an integer,
    // we add the integer part directly and accumulate the remainder.
    int baseIncrement = 100 / config.oilmeter.pulse_per_liter;      // Integer part increment
    int remainderIncrement = 100 % config.oilmeter.pulse_per_liter; // Remainder to accumulate

    data.oilcounter += baseIncrement;
    oil_remainder += remainderIncrement;

    // When the accumulated remainder reaches or exceeds the threshold,
    // add one additional hundredth to the counter and subtract the threshold.
    if (oil_remainder >= config.oilmeter.pulse_per_liter) {
      data.oilcounter += 1;
      oil_remainder -= config.oilmeter.pulse_per_liter;
      sendOilmeter(); // Send new counter value via MQTT
    }
  }

  if ((data.oilcounter % 50) == 0) { // store new value in flash every (0.5 liter)
    cmdStoreOilmeter();
  }

  // Send cyclic information periodically
  if (oilCyclicInfo.cycleTrigger(OILCYCLICINFO_TIME)) {
    sendOilmeter();
  }

  reboot = false; // Reset the reboot flag
}
