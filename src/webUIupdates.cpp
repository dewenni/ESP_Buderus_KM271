
#include <language.h>
#include <main.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

/* S E T T I N G S ****************************************************/
#define WEBUI_FAST_REFRESH_TIME_MS 20
#define WEBUI_SLOW_REFRESH_TIME_MS 1000
#define WEBUI_JSON_REFRESH_TIME_MS 50

/* P R O T O T Y P E S ********************************************************/
void updateKm271ConfigElementsAll();
void updateKm271StatusElementsAll();
void updateOilmeterElements(bool init);
void updateSensorElements();
void updateSystemInfoElements();

/* D E C L A R A T I O N S ****************************************************/
muTimer refreshTimer1 = muTimer();    // timer to refresh other values
muTimer refreshTimer2 = muTimer();    // timer to refresh other values
muTimer refreshTimer3 = muTimer();    // timer to refresh other values
muTimer gitVersionTimer1 = muTimer(); // timer to refresh other values
muTimer gitVersionTimer2 = muTimer(); // timer to refresh other values

s_km271_status kmStatusCpy;
s_km271_status *pkmStatus = km271GetStatusValueAdr();
s_km271_config_str *pkmConfigStr = km271GetConfigStringsAdr();
s_km271_config_num *pkmConfigNum = km271GetConfigValueAdr();
s_km271_alarm_str *pkmAlarmStr = km271GetAlarmMsgAdr();

char tmpMessage[300] = {'\0'};
unsigned int KmCfgHash[kmConfig_Hash_SIZE] = {0};
unsigned int KmAlarmHash[4] = {0};
bool refreshRequest = false;
bool km271RefreshActiveOld = false;
int UpdateCntFast = 0;
int UpdateCntSlow = 0;
int UpdateCntRefresh = 0;
s_lang LANG;
JsonDocument jsonDoc;

// convert minutes to human readable structure
timeComponents convertMinutes(int totalMinutes) {
  int minutesPerYear = 525600; // 365 days * 24 hours * 60 minutes
  int minutesPerDay = 1440;    // 24 hours * 60 minutes
  int minutesPerHour = 60;

  timeComponents result;

  result.years = totalMinutes / minutesPerYear;
  int remainingMinutes = totalMinutes % minutesPerYear;

  result.days = remainingMinutes / minutesPerDay;
  remainingMinutes %= minutesPerDay;

  result.hours = remainingMinutes / minutesPerHour;
  remainingMinutes %= minutesPerHour;

  result.minutes = remainingMinutes;

  return result;
}

// ---------------------------------------------------------------------------------------------------------------------
// custom json function to update many elements with minimized message size
// elementID, elementType and elementValue are combined in one Key-Value-Pair
//
// Example:
// "p09_wifi_ip#l":"192.168.178.193"
// JSON "key" is a combination of webUI elementID and the kind of the webUI element (innterHTML, value, input, etc...)
// "p09_wifi_ip#l":"192.168.178.193" => elementID=p09_wifi_ip, elementType=label, elementValue=192.168.178.193
// handling on client side with js-function updateJSON(data)
// ---------------------------------------------------------------------------------------------------------------------

// initialize JSON-Buffer
void initJsonBuffer(JsonDocument &jsonBuf) {
  jsonBuf.clear();
  jsonBuf["type"] = "updateJSON";
}

// add JSON Element to JSON-Buffer
void addJsonElement(JsonDocument &jsonBuf, const char *elementID, const char *typSuffix, const char *value) {
  char key[128];
  snprintf(key, sizeof(key), "%s#%s", elementID, typSuffix);
  jsonBuf[key].set((char *)value); // make sure value is handled as a copy not as pointer
};

void addJsonLabelInt(JsonDocument &jsonBuf, const char *elementID, intmax_t value) { addJsonElement(jsonBuf, elementID, "l", int32ToString(value)); };

void addJsonLabelTxt(JsonDocument &jsonBuf, const char *elementID, const char *value) { addJsonElement(jsonBuf, elementID, "l", value); };

void addJsonValueTxt(JsonDocument &jsonBuf, const char *elementID, const char *value) { addJsonElement(jsonBuf, elementID, "v", value); };

void addJsonValueInt(JsonDocument &jsonBuf, const char *elementID, intmax_t value) { addJsonElement(jsonBuf, elementID, "v", int32ToString(value)); };

void addJsonValueFlt(JsonDocument &jsonBuf, const char *elementID, float value) { addJsonElement(jsonBuf, elementID, "v", floatToString(value)); };

void addJsonValueFlt8(JsonDocument &jsonBuf, const char *elementID, float value) { addJsonElement(jsonBuf, elementID, "v", floatToString8(value)); };

void addJsonState(JsonDocument &jsonBuf, const char *elementID, bool value) { addJsonElement(jsonBuf, elementID, "c", (value ? "true" : "false")); };

void addJsonIcon(JsonDocument &jsonBuf, const char *elementID, const char *value) { addJsonElement(jsonBuf, elementID, "i", value); };

// ----------------------------------------------------------------

/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAllElements() {

  refreshRequest = true; // start combined json refresh

  // reset hash values to force updates
  memset((void *)KmAlarmHash, 0, sizeof(KmAlarmHash));

  updateOilmeterElements(true);
  updateWebLanguage(LANG.CODE[config.lang]);
  showElementClass("simModeBar", config.sim.enable);

  if (setupMode) {
    showElementClass("setupModeBar", true);
  }
}

/**
 * *******************************************************************
 * @brief   update Sensor informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSensorElements() {

  if (config.sensor.ch1_enable) {
    updateWebText("p01_sens1_name", config.sensor.ch1_name, false);
    updateWebTextInt("p01_sens1_value", sensor.ch1_temp, false);
    updateWebText("p01_sens1_description", config.sensor.ch1_description, false);
  }

  if (config.sensor.ch2_enable) {
    updateWebText("p01_sens2_name", config.sensor.ch2_name, false);
    updateWebTextInt("p01_sens2_value", sensor.ch2_temp, false);
    updateWebText("p01_sens2_description", config.sensor.ch2_description, false);
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateOilmeterElements(bool init) {

  static long oilcounter, oilcounterOld;
  static double oilcounterVirtOld = 0.0;

  if (config.oilmeter.use_hardware_meter) {
    oilcounter = getOilmeter();
    if (init || oilcounter != oilcounterOld) {
      oilcounterOld = oilcounter;

      // Oilmeter value in controlTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f", float(oilcounter) / 100);
      updateWebText("p02_oilmeter_act", tmpMessage, false);
      snprintf(tmpMessage, sizeof(tmpMessage), "%lu", (oilcounter));
      updateWebText("p02_oilmeter_set", tmpMessage, true);

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(oilcounter) / 100);
      updateWebText("p01_hw_oilmeter", tmpMessage, false);
    }
  }
  if (config.oilmeter.use_virtual_meter) {
    if (init || pkmStatus->BurnerCalcOilConsumption != oilcounterVirtOld) {
      oilcounterVirtOld = pkmStatus->BurnerCalcOilConsumption;

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(pkmStatus->BurnerCalcOilConsumption));
      updateWebText("p01_v_oilmeter", tmpMessage, false);
    }
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateGpioSettings() {

  initJsonBuffer(jsonDoc);

  addJsonValueInt(jsonDoc, "cfg_gpio_km271_RX", config.gpio.km271_RX);
  addJsonValueInt(jsonDoc, "cfg_gpio_km271_TX", config.gpio.km271_TX);
  addJsonValueInt(jsonDoc, "cfg_gpio_led_heartbeat", config.gpio.led_heartbeat);
  addJsonValueInt(jsonDoc, "cfg_gpio_led_logmode", config.gpio.led_logmode);
  addJsonValueInt(jsonDoc, "cfg_gpio_led_wifi", config.gpio.led_wifi);
  addJsonValueInt(jsonDoc, "cfg_gpio_led_oilcounter", config.gpio.led_oilcounter);
  addJsonValueInt(jsonDoc, "cfg_gpio_trigger_oilcounter", config.gpio.trigger_oilcounter);

  updateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  initJsonBuffer(jsonDoc);

  // Network information
  addJsonLabelTxt(jsonDoc, "p09_wifi_ip", wifi.ipAddress);
  snprintf(tmpMessage, sizeof(tmpMessage), "%i %%", wifi.signal);
  addJsonLabelTxt(jsonDoc, "p09_wifi_signal", tmpMessage);
  snprintf(tmpMessage, sizeof(tmpMessage), "%ld dbm", wifi.rssi);
  addJsonLabelTxt(jsonDoc, "p09_wifi_rssi", tmpMessage);

  if (!WiFi.isConnected()) {
    addJsonIcon(jsonDoc, "p00_wifi_icon", "i_wifi_nok");
  } else if (wifi.rssi < -80) {
    addJsonIcon(jsonDoc, "p00_wifi_icon", "i_wifi_1");
  } else if (wifi.rssi < -70) {
    addJsonIcon(jsonDoc, "p00_wifi_icon", "i_wifi_2");
  } else if (wifi.rssi < -60) {
    addJsonIcon(jsonDoc, "p00_wifi_icon", "i_wifi_3");
  } else {
    addJsonIcon(jsonDoc, "p00_wifi_icon", "i_wifi_4");
  }

  addJsonLabelTxt(jsonDoc, "p09_eth_ip", strlen(eth.ipAddress) ? eth.ipAddress : "-.-.-.-");
  addJsonLabelTxt(jsonDoc, "p09_eth_status", eth.connected ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (eth.connected) {
    addJsonIcon(jsonDoc, "p00_eth_icon", "i_eth");
    snprintf(tmpMessage, sizeof(tmpMessage), "%d Mbps", eth.linkSpeed);
    addJsonLabelTxt(jsonDoc, "p09_eth_link_speed", tmpMessage);
    addJsonLabelTxt(jsonDoc, "p09_eth_full_duplex", eth.fullDuplex ? WEB_TXT::FULL_DUPLEX[config.lang] : "---");

  } else {
    addJsonIcon(jsonDoc, "p00_eth_icon", "");
    addJsonLabelTxt(jsonDoc, "p09_eth_link_speed", "---");
    addJsonLabelTxt(jsonDoc, "p09_eth_full_duplex", "---");
  }

  // MQTT Status
  addJsonLabelTxt(jsonDoc, "p09_mqtt_status", mqttIsEnabled() ? WEB_TXT::ACTIVE[config.lang] : WEB_TXT::INACTIVE[config.lang]);
  addJsonLabelTxt(jsonDoc, "p09_mqtt_connection", mqttIsConnected() ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (mqttGetLastError() != nullptr) {
    addJsonLabelTxt(jsonDoc, "p09_mqtt_last_err", mqttGetLastError());
  } else {
    addJsonLabelTxt(jsonDoc, "p09_mqtt_last_err", "---");
  }

  // ESP informations
  addJsonLabelInt(jsonDoc, "p09_esp_flash_usage", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace());
  addJsonLabelInt(jsonDoc, "p09_esp_heap_usage", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize());
  addJsonLabelInt(jsonDoc, "p09_esp_maxallocheap", (float)ESP.getMaxAllocHeap() / 1000.0);
  addJsonLabelInt(jsonDoc, "p09_esp_minfreeheap", (float)ESP.getMinFreeHeap() / 1000.0);

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  addJsonLabelTxt(jsonDoc, "p09_uptime", uptimeStr);

  // actual date and time
  addJsonValueTxt(jsonDoc, "p12_ntp_date", getDateStringWeb());
  addJsonValueTxt(jsonDoc, "p12_ntp_time", getTimeString());

  updateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElementsStatic() {

  initJsonBuffer(jsonDoc);

  // Version informations
  addJsonLabelTxt(jsonDoc, "p00_version", VERSION);
  addJsonLabelTxt(jsonDoc, "p09_sw_version", VERSION);
  addJsonLabelTxt(jsonDoc, "p00_dialog_version", VERSION);

  getBuildDateTime(tmpMessage);
  addJsonLabelTxt(jsonDoc, "p09_sw_date", tmpMessage);

  // restart reason
  char restartReason[64];
  getRestartReason(restartReason, sizeof(restartReason));
  addJsonLabelTxt(jsonDoc, "p09_restart_reason", restartReason);

  updateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update Alarm messages of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271AlarmElements() {

  if (strDiff(&KmAlarmHash[0], pkmAlarmStr->alarm1)) {
    updateWebText("p08_err_msg1", pkmAlarmStr->alarm1, false);
  }
  if (strDiff(&KmAlarmHash[1], pkmAlarmStr->alarm2)) {
    updateWebText("p08_err_msg2", pkmAlarmStr->alarm2, false);
  }
  if (strDiff(&KmAlarmHash[2], pkmAlarmStr->alarm3)) {
    updateWebText("p08_err_msg3", pkmAlarmStr->alarm3, false);
  }
  if (strDiff(&KmAlarmHash[3], pkmAlarmStr->alarm4)) {
    updateWebText("p08_err_msg4", pkmAlarmStr->alarm4, false);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271ConfigElementsAll() {

  initJsonBuffer(jsonDoc);

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {
    addJsonValueInt(jsonDoc, "p02_hc1_frost_prot_th", pkmConfigNum->hc1_frost_protection_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc1_frost_prot_th_txt", pkmConfigNum->hc1_frost_protection_threshold);
    addJsonLabelTxt(jsonDoc, "p03_hc1_frost_th", pkmConfigStr->hc1_frost_protection_threshold);
    addJsonValueInt(jsonDoc, "p02_hc1_summer_th", pkmConfigNum->hc1_summer_mode_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc1_summer_th_txt", pkmConfigNum->hc1_summer_mode_threshold);
    addJsonLabelTxt(jsonDoc, "p03_hc1_summer_th", pkmConfigStr->hc1_summer_mode_threshold);
    addJsonLabelTxt(jsonDoc, "p03_hc1_night_temp", pkmConfigStr->hc1_night_temp);
    addJsonLabelTxt(jsonDoc, "p03_hc1_day_temp", pkmConfigStr->hc1_day_temp);
    addJsonState(jsonDoc, "p02_hc1_opmode_night", pkmConfigNum->hc1_operation_mode == 0 ? true : false);
    addJsonState(jsonDoc, "p02_hc1_opmode_day", pkmConfigNum->hc1_operation_mode == 1 ? true : false);
    addJsonState(jsonDoc, "p02_hc1_opmode_auto", pkmConfigNum->hc1_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonDoc, "p03_hc1_op_mode", pkmConfigStr->hc1_operation_mode);
    addJsonLabelTxt(jsonDoc, "p03_hc1_hday_temp", pkmConfigStr->hc1_holiday_temp);
    addJsonLabelTxt(jsonDoc, "p03_hc1_max_temp", pkmConfigStr->hc1_max_temp);
    addJsonValueInt(jsonDoc, "p02_hc1_interp", pkmConfigNum->hc1_interpretation);
    addJsonLabelInt(jsonDoc, "p02_hc1_interp_txt", pkmConfigNum->hc1_interpretation);
    addJsonLabelTxt(jsonDoc, "p03_hc1_interp", pkmConfigStr->hc1_interpretation);
    addJsonLabelTxt(jsonDoc, "p03_hc1_sw_on_temp", pkmConfigStr->hc1_switch_on_temperature);
    addJsonValueInt(jsonDoc, "p02_hc1_sw_off_th", pkmConfigNum->hc1_switch_off_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc1_sw_off_th_txt", pkmConfigNum->hc1_switch_off_threshold);
    addJsonLabelTxt(jsonDoc, "p03_hc1_sw_off_th", pkmConfigStr->hc1_switch_off_threshold);
    addJsonLabelTxt(jsonDoc, "p03_hc1_red_mode", pkmConfigStr->hc1_reduction_mode);
    addJsonValueInt(jsonDoc, "p02_hc1_reduct_mode", pkmConfigNum->hc1_reduction_mode);
    addJsonLabelTxt(jsonDoc, "p03_hc1_heatsys", pkmConfigStr->hc1_heating_system);
    addJsonLabelTxt(jsonDoc, "p03_hc1_temp_offset", pkmConfigStr->hc1_temp_offset);
    addJsonLabelTxt(jsonDoc, "p03_hc1_remotecontrol", pkmConfigStr->hc1_remotecontrol);
    addJsonValueInt(jsonDoc, "p02_hc1_prg", pkmConfigNum->hc1_program);
    addJsonValueInt(jsonDoc, "p02_hc1_holiday_days", pkmConfigNum->hc1_holiday_days);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t01", pkmConfigStr->hc1_timer01);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t02", pkmConfigStr->hc1_timer02);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t03", pkmConfigStr->hc1_timer03);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t04", pkmConfigStr->hc1_timer04);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t05", pkmConfigStr->hc1_timer05);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t06", pkmConfigStr->hc1_timer06);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t07", pkmConfigStr->hc1_timer07);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t08", pkmConfigStr->hc1_timer08);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t09", pkmConfigStr->hc1_timer09);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t10", pkmConfigStr->hc1_timer10);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t11", pkmConfigStr->hc1_timer11);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t12", pkmConfigStr->hc1_timer12);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t13", pkmConfigStr->hc1_timer13);
    addJsonLabelTxt(jsonDoc, "p03_hc1_t14", pkmConfigStr->hc1_timer14);
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    addJsonValueInt(jsonDoc, "p02_hc2_frost_prot_th", pkmConfigNum->hc2_frost_protection_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc2_frost_prot_th_txt", pkmConfigNum->hc2_frost_protection_threshold);
    addJsonLabelTxt(jsonDoc, "p04_hc2_frost_th", pkmConfigStr->hc2_frost_protection_threshold);
    addJsonValueInt(jsonDoc, "p02_hc2_summer_th", pkmConfigNum->hc2_summer_mode_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc2_summer_th_txt", pkmConfigNum->hc2_summer_mode_threshold);
    addJsonLabelTxt(jsonDoc, "p04_hc2_summer_th", pkmConfigStr->hc2_summer_mode_threshold);
    addJsonLabelTxt(jsonDoc, "p04_hc2_night_temp", pkmConfigStr->hc2_night_temp);
    addJsonLabelTxt(jsonDoc, "p04_hc2_day_temp", pkmConfigStr->hc2_day_temp);
    addJsonState(jsonDoc, "p02_hc2_opmode_night", pkmConfigNum->hc2_operation_mode == 0 ? true : false);
    addJsonState(jsonDoc, "p02_hc2_opmode_day", pkmConfigNum->hc2_operation_mode == 1 ? true : false);
    addJsonState(jsonDoc, "p02_hc2_opmode_auto", pkmConfigNum->hc2_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonDoc, "p04_hc2_op_mode", pkmConfigStr->hc2_operation_mode);
    addJsonLabelTxt(jsonDoc, "p04_hc2_hday_temp", pkmConfigStr->hc2_holiday_temp);
    addJsonLabelTxt(jsonDoc, "p04_hc2_max_temp", pkmConfigStr->hc2_max_temp);
    addJsonValueInt(jsonDoc, "p02_hc2_interp", pkmConfigNum->hc2_interpretation);
    addJsonLabelInt(jsonDoc, "p02_hc2_interp_txt", pkmConfigNum->hc2_interpretation);
    addJsonLabelTxt(jsonDoc, "p04_hc2_interp", pkmConfigStr->hc2_interpretation);
    addJsonLabelTxt(jsonDoc, "p04_hc2_sw_on_temp", pkmConfigStr->hc2_switch_on_temperature);
    addJsonValueInt(jsonDoc, "p02_hc2_sw_off_th", pkmConfigNum->hc2_switch_off_threshold);
    addJsonLabelInt(jsonDoc, "p02_hc2_sw_off_th_txt", pkmConfigNum->hc2_switch_off_threshold);
    addJsonLabelTxt(jsonDoc, "p04_hc2_sw_off_th", pkmConfigStr->hc2_switch_off_threshold);
    addJsonLabelTxt(jsonDoc, "p04_hc2_red_mode", pkmConfigStr->hc2_reduction_mode);
    addJsonValueInt(jsonDoc, "p02_hc2_reduct_mode", pkmConfigNum->hc2_reduction_mode);
    addJsonLabelTxt(jsonDoc, "p04_hc2_heatsys", pkmConfigStr->hc2_heating_system);
    addJsonLabelTxt(jsonDoc, "p04_hc2_temp_offset", pkmConfigStr->hc2_temp_offset);
    addJsonLabelTxt(jsonDoc, "p04_hc2_remotecontrol", pkmConfigStr->hc2_remotecontrol);
    addJsonValueInt(jsonDoc, "p02_hc2_prg", pkmConfigNum->hc2_program);
    addJsonValueInt(jsonDoc, "p02_hc2_holiday_days", pkmConfigNum->hc2_holiday_days);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t01", pkmConfigStr->hc2_timer01);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t02", pkmConfigStr->hc2_timer02);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t03", pkmConfigStr->hc2_timer03);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t04", pkmConfigStr->hc2_timer04);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t05", pkmConfigStr->hc2_timer05);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t06", pkmConfigStr->hc2_timer06);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t07", pkmConfigStr->hc2_timer07);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t08", pkmConfigStr->hc2_timer08);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t09", pkmConfigStr->hc2_timer09);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t10", pkmConfigStr->hc2_timer10);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t11", pkmConfigStr->hc2_timer11);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t12", pkmConfigStr->hc2_timer12);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t13", pkmConfigStr->hc2_timer13);
    addJsonLabelTxt(jsonDoc, "p04_hc2_t14", pkmConfigStr->hc2_timer14);
  }

  // hot-water config values
  if (config.km271.use_ww) {
    addJsonLabelTxt(jsonDoc, "p05_hw_prio", pkmConfigStr->ww_priority);
    addJsonValueInt(jsonDoc, "p02_ww_temp", pkmConfigNum->ww_temp);
    addJsonLabelInt(jsonDoc, "p02_ww_temp_txt", pkmConfigNum->ww_temp);
    addJsonLabelTxt(jsonDoc, "p05_hw_temp", pkmConfigStr->ww_temp);
    addJsonState(jsonDoc, "p02_ww_opmode_night", pkmConfigNum->ww_operation_mode == 0 ? true : false);
    addJsonState(jsonDoc, "p02_ww_opmode_day", pkmConfigNum->ww_operation_mode == 1 ? true : false);
    addJsonState(jsonDoc, "p02_ww_opmode_auto", pkmConfigNum->ww_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonDoc, "p05_hw_op_mode", pkmConfigStr->ww_operation_mode);
    addJsonLabelTxt(jsonDoc, "p05_hw_proccess", pkmConfigStr->ww_processing);
    addJsonValueInt(jsonDoc, "p02_ww_circ", pkmConfigNum->ww_circulation);
    addJsonLabelInt(jsonDoc, "p02_ww_circ_txt", pkmConfigNum->ww_circulation);
    addJsonLabelTxt(jsonDoc, "p05_hw_circ", pkmConfigStr->ww_circulation);
  }

  // general config values
  addJsonLabelTxt(jsonDoc, "p07_language", pkmConfigStr->language);
  addJsonLabelTxt(jsonDoc, "p07_display", pkmConfigStr->display);
  addJsonLabelTxt(jsonDoc, "p07_burner_type", pkmConfigStr->burner_type);
  addJsonLabelTxt(jsonDoc, "p06_max_boil_temp", pkmConfigStr->max_boiler_temperature);
  addJsonLabelTxt(jsonDoc, "p07_pump_logic_temp", pkmConfigStr->pump_logic_temp);
  addJsonLabelTxt(jsonDoc, "p07_exh_gas_temp_th", pkmConfigStr->exhaust_gas_temperature_threshold);
  addJsonLabelTxt(jsonDoc, "p07_burner_min_mod", pkmConfigStr->burner_min_modulation);
  addJsonLabelTxt(jsonDoc, "p07_burner_mod_run", pkmConfigStr->burner_modulation_runtime);
  addJsonLabelTxt(jsonDoc, "p07_building_type", pkmConfigStr->building_type);
  addJsonLabelTxt(jsonDoc, "p07_time_offset", pkmConfigStr->time_offset);

  updateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271ConfigElements() {

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {
    if (strDiff(&KmCfgHash[hc1_frost_protection_threshold], pkmConfigStr->hc1_frost_protection_threshold)) {
      updateWebValueInt("p02_hc1_frost_prot_th", pkmConfigNum->hc1_frost_protection_threshold);
      updateWebTextInt("p02_hc1_frost_prot_th_txt", pkmConfigNum->hc1_frost_protection_threshold, false);
      updateWebText("p03_hc1_frost_th", pkmConfigStr->hc1_frost_protection_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_summer_mode_threshold], pkmConfigStr->hc1_summer_mode_threshold)) {
      updateWebValueInt("p02_hc1_summer_th", pkmConfigNum->hc1_summer_mode_threshold);
      updateWebTextInt("p02_hc1_summer_th_txt", pkmConfigNum->hc1_summer_mode_threshold, false);
      updateWebText("p03_hc1_summer_th", pkmConfigStr->hc1_summer_mode_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_night_temp], pkmConfigStr->hc1_night_temp)) {
      updateWebText("p03_hc1_night_temp", pkmConfigStr->hc1_night_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_day_temp], pkmConfigStr->hc1_day_temp)) {
      updateWebText("p03_hc1_day_temp", pkmConfigStr->hc1_day_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_operation_mode], pkmConfigStr->hc1_operation_mode)) {
      updateWebState("p02_hc1_opmode_night", pkmConfigNum->hc1_operation_mode == 0 ? true : false);
      updateWebState("p02_hc1_opmode_day", pkmConfigNum->hc1_operation_mode == 1 ? true : false);
      updateWebState("p02_hc1_opmode_auto", pkmConfigNum->hc1_operation_mode == 2 ? true : false);
      updateWebText("p03_hc1_op_mode", pkmConfigStr->hc1_operation_mode, false);
    } else if (strDiff(&KmCfgHash[hc1_holiday_temp], pkmConfigStr->hc1_holiday_temp)) {
      updateWebText("p03_hc1_hday_temp", pkmConfigStr->hc1_holiday_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_max_temp], pkmConfigStr->hc1_max_temp)) {
      updateWebText("p03_hc1_max_temp", pkmConfigStr->hc1_max_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_interpretation], pkmConfigStr->hc1_interpretation)) {
      updateWebValueInt("p02_hc1_interp", pkmConfigNum->hc1_interpretation);
      updateWebTextInt("p02_hc1_interp_txt", pkmConfigNum->hc1_interpretation, false);
      updateWebText("p03_hc1_interp", pkmConfigStr->hc1_interpretation, false);
    } else if (strDiff(&KmCfgHash[hc1_switch_on_temperature], pkmConfigStr->hc1_switch_on_temperature)) {
      updateWebText("p03_hc1_sw_on_temp", pkmConfigStr->hc1_switch_on_temperature, false);
    } else if (strDiff(&KmCfgHash[hc1_switch_off_threshold], pkmConfigStr->hc1_switch_off_threshold)) {
      updateWebValueInt("p02_hc1_sw_off_th", pkmConfigNum->hc1_switch_off_threshold);
      updateWebTextInt("p02_hc1_sw_off_th_txt", pkmConfigNum->hc1_switch_off_threshold, false);
      updateWebText("p03_hc1_sw_off_th", pkmConfigStr->hc1_switch_off_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_reduction_mode], pkmConfigStr->hc1_reduction_mode)) {
      updateWebText("p03_hc1_red_mode", pkmConfigStr->hc1_reduction_mode, false);
      updateWebValueInt("p02_hc1_reduct_mode", pkmConfigNum->hc1_reduction_mode);
    } else if (strDiff(&KmCfgHash[hc1_heating_system], pkmConfigStr->hc1_heating_system)) {
      updateWebText("p03_hc1_heatsys", pkmConfigStr->hc1_heating_system, false);
    } else if (strDiff(&KmCfgHash[hc1_temp_offset], pkmConfigStr->hc1_temp_offset)) {
      updateWebText("p03_hc1_temp_offset", pkmConfigStr->hc1_temp_offset, false);
    } else if (strDiff(&KmCfgHash[hc1_remotecontrol], pkmConfigStr->hc1_remotecontrol)) {
      updateWebText("p03_hc1_remotecontrol", pkmConfigStr->hc1_remotecontrol, false);
    } else if (strDiff(&KmCfgHash[hc1_program], pkmConfigStr->hc1_program)) {
      updateWebValueInt("p02_hc1_prg", pkmConfigNum->hc1_program);
    } else if (strDiff(&KmCfgHash[hc1_holiday_days], pkmConfigStr->hc1_holiday_days)) {
      updateWebValueInt("p02_hc1_holiday_days", pkmConfigNum->hc1_holiday_days);
    } else if (strDiff(&KmCfgHash[hc1_timer01], pkmConfigStr->hc1_timer01)) {
      updateWebText("p03_hc1_t01", pkmConfigStr->hc1_timer01, false);
    } else if (strDiff(&KmCfgHash[hc1_timer02], pkmConfigStr->hc1_timer02)) {
      updateWebText("p03_hc1_t02", pkmConfigStr->hc1_timer02, false);
    } else if (strDiff(&KmCfgHash[hc1_timer03], pkmConfigStr->hc1_timer03)) {
      updateWebText("p03_hc1_t03", pkmConfigStr->hc1_timer03, false);
    } else if (strDiff(&KmCfgHash[hc1_timer04], pkmConfigStr->hc1_timer04)) {
      updateWebText("p03_hc1_t04", pkmConfigStr->hc1_timer04, false);
    } else if (strDiff(&KmCfgHash[hc1_timer05], pkmConfigStr->hc1_timer05)) {
      updateWebText("p03_hc1_t05", pkmConfigStr->hc1_timer05, false);
    } else if (strDiff(&KmCfgHash[hc1_timer06], pkmConfigStr->hc1_timer06)) {
      updateWebText("p03_hc1_t06", pkmConfigStr->hc1_timer06, false);
    } else if (strDiff(&KmCfgHash[hc1_timer07], pkmConfigStr->hc1_timer07)) {
      updateWebText("p03_hc1_t07", pkmConfigStr->hc1_timer07, false);
    } else if (strDiff(&KmCfgHash[hc1_timer08], pkmConfigStr->hc1_timer08)) {
      updateWebText("p03_hc1_t08", pkmConfigStr->hc1_timer08, false);
    } else if (strDiff(&KmCfgHash[hc1_timer09], pkmConfigStr->hc1_timer09)) {
      updateWebText("p03_hc1_t09", pkmConfigStr->hc1_timer09, false);
    } else if (strDiff(&KmCfgHash[hc1_timer10], pkmConfigStr->hc1_timer10)) {
      updateWebText("p03_hc1_t10", pkmConfigStr->hc1_timer10, false);
    } else if (strDiff(&KmCfgHash[hc1_timer11], pkmConfigStr->hc1_timer11)) {
      updateWebText("p03_hc1_t11", pkmConfigStr->hc1_timer11, false);
    } else if (strDiff(&KmCfgHash[hc1_timer12], pkmConfigStr->hc1_timer12)) {
      updateWebText("p03_hc1_t12", pkmConfigStr->hc1_timer12, false);
    } else if (strDiff(&KmCfgHash[hc1_timer13], pkmConfigStr->hc1_timer13)) {
      updateWebText("p03_hc1_t13", pkmConfigStr->hc1_timer13, false);
    } else if (strDiff(&KmCfgHash[hc1_timer14], pkmConfigStr->hc1_timer14)) {
      updateWebText("p03_hc1_t14", pkmConfigStr->hc1_timer14, false);
    }
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    if (strDiff(&KmCfgHash[hc2_frost_protection_threshold], pkmConfigStr->hc2_frost_protection_threshold)) {
      updateWebValueInt("p02_hc2_frost_prot_th", pkmConfigNum->hc2_frost_protection_threshold);
      updateWebTextInt("p02_hc2_frost_prot_th_txt", pkmConfigNum->hc2_frost_protection_threshold, false);
      updateWebText("p04_hc2_frost_th", pkmConfigStr->hc2_frost_protection_threshold, false);
    } else if (strDiff(&KmCfgHash[hc2_summer_mode_threshold], pkmConfigStr->hc2_summer_mode_threshold)) {
      updateWebValueInt("p02_hc2_summer_th", pkmConfigNum->hc2_summer_mode_threshold);
      updateWebTextInt("p02_hc2_summer_th_txt", pkmConfigNum->hc2_summer_mode_threshold, false);
      updateWebText("p04_hc2_summer_th", pkmConfigStr->hc2_summer_mode_threshold, false);
    }

    else if (strDiff(&KmCfgHash[hc2_night_temp], pkmConfigStr->hc2_night_temp)) {
      updateWebText("p04_hc2_night_temp", pkmConfigStr->hc2_night_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_day_temp], pkmConfigStr->hc2_day_temp)) {
      updateWebText("p04_hc2_day_temp", pkmConfigStr->hc2_day_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_operation_mode], pkmConfigStr->hc2_operation_mode)) {
      updateWebState("p02_hc2_opmode_night", pkmConfigNum->hc2_operation_mode == 0 ? true : false);
      updateWebState("p02_hc2_opmode_day", pkmConfigNum->hc2_operation_mode == 1 ? true : false);
      updateWebState("p02_hc2_opmode_auto", pkmConfigNum->hc2_operation_mode == 2 ? true : false);
      updateWebText("p04_hc2_op_mode", pkmConfigStr->hc2_operation_mode, false);
    } else if (strDiff(&KmCfgHash[hc2_holiday_temp], pkmConfigStr->hc2_holiday_temp)) {
      updateWebText("p04_hc2_hday_temp", pkmConfigStr->hc2_holiday_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_max_temp], pkmConfigStr->hc2_max_temp)) {
      updateWebText("p04_hc2_max_temp", pkmConfigStr->hc2_max_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_interpretation], pkmConfigStr->hc2_interpretation)) {
      updateWebValueInt("p02_hc2_interp", pkmConfigNum->hc2_interpretation);
      updateWebTextInt("p02_hc2_interp_txt", pkmConfigNum->hc2_interpretation, false);
      updateWebText("p04_hc2_interp", pkmConfigStr->hc2_interpretation, false);
    } else if (strDiff(&KmCfgHash[hc2_switch_on_temperature], pkmConfigStr->hc2_switch_on_temperature)) {
      updateWebText("p04_hc2_sw_on_temp", pkmConfigStr->hc2_switch_on_temperature, false);
    } else if (strDiff(&KmCfgHash[hc2_switch_off_threshold], pkmConfigStr->hc2_switch_off_threshold)) {
      updateWebValueInt("p02_hc2_sw_off_th", pkmConfigNum->hc2_switch_off_threshold);
      updateWebTextInt("p02_hc2_sw_off_th_txt", pkmConfigNum->hc2_switch_off_threshold, false);
      updateWebText("p04_hc2_sw_off_th", pkmConfigStr->hc2_switch_off_threshold, false);
    } else if (strDiff(&KmCfgHash[hc2_reduction_mode], pkmConfigStr->hc2_reduction_mode)) {
      updateWebText("p04_hc2_red_mode", pkmConfigStr->hc2_reduction_mode, false);
      updateWebValueInt("p02_hc2_reduct_mode", pkmConfigNum->hc2_reduction_mode);
    } else if (strDiff(&KmCfgHash[hc2_heating_system], pkmConfigStr->hc2_heating_system)) {
      updateWebText("p04_hc2_heatsys", pkmConfigStr->hc2_heating_system, false);
    } else if (strDiff(&KmCfgHash[hc2_temp_offset], pkmConfigStr->hc2_temp_offset)) {
      updateWebText("p04_hc2_temp_offset", pkmConfigStr->hc2_temp_offset, false);
    } else if (strDiff(&KmCfgHash[hc2_remotecontrol], pkmConfigStr->hc2_remotecontrol)) {
      updateWebText("p04_hc2_remotecontrol", pkmConfigStr->hc2_remotecontrol, false);
    } else if (strDiff(&KmCfgHash[hc2_program], pkmConfigStr->hc2_program)) {
      updateWebValueInt("p02_hc2_prg", pkmConfigNum->hc2_program);
    } else if (strDiff(&KmCfgHash[hc2_holiday_days], pkmConfigStr->hc2_holiday_days)) {
      updateWebValueInt("p02_hc2_holiday_days", pkmConfigNum->hc2_holiday_days);
    } else if (strDiff(&KmCfgHash[hc2_timer01], pkmConfigStr->hc2_timer01)) {
      updateWebText("p04_hc2_t01", pkmConfigStr->hc2_timer01, false);
    } else if (strDiff(&KmCfgHash[hc2_timer02], pkmConfigStr->hc2_timer02)) {
      updateWebText("p04_hc2_t02", pkmConfigStr->hc2_timer02, false);
    } else if (strDiff(&KmCfgHash[hc2_timer03], pkmConfigStr->hc2_timer03)) {
      updateWebText("p04_hc2_t03", pkmConfigStr->hc2_timer03, false);
    } else if (strDiff(&KmCfgHash[hc2_timer04], pkmConfigStr->hc2_timer04)) {
      updateWebText("p04_hc2_t04", pkmConfigStr->hc2_timer04, false);
    } else if (strDiff(&KmCfgHash[hc2_timer05], pkmConfigStr->hc2_timer05)) {
      updateWebText("p04_hc2_t05", pkmConfigStr->hc2_timer05, false);
    } else if (strDiff(&KmCfgHash[hc2_timer06], pkmConfigStr->hc2_timer06)) {
      updateWebText("p04_hc2_t06", pkmConfigStr->hc2_timer06, false);
    } else if (strDiff(&KmCfgHash[hc2_timer07], pkmConfigStr->hc2_timer07)) {
      updateWebText("p04_hc2_t07", pkmConfigStr->hc2_timer07, false);
    } else if (strDiff(&KmCfgHash[hc2_timer08], pkmConfigStr->hc2_timer08)) {
      updateWebText("p04_hc2_t08", pkmConfigStr->hc2_timer08, false);
    } else if (strDiff(&KmCfgHash[hc2_timer09], pkmConfigStr->hc2_timer09)) {
      updateWebText("p04_hc2_t09", pkmConfigStr->hc2_timer09, false);
    } else if (strDiff(&KmCfgHash[hc2_timer10], pkmConfigStr->hc2_timer10)) {
      updateWebText("p04_hc2_t10", pkmConfigStr->hc2_timer10, false);
    } else if (strDiff(&KmCfgHash[hc2_timer11], pkmConfigStr->hc2_timer11)) {
      updateWebText("p04_hc2_t11", pkmConfigStr->hc2_timer11, false);
    } else if (strDiff(&KmCfgHash[hc2_timer12], pkmConfigStr->hc2_timer12)) {
      updateWebText("p04_hc2_t12", pkmConfigStr->hc2_timer12, false);
    } else if (strDiff(&KmCfgHash[hc2_timer13], pkmConfigStr->hc2_timer13)) {
      updateWebText("p04_hc2_t13", pkmConfigStr->hc2_timer13, false);
    } else if (strDiff(&KmCfgHash[hc2_timer14], pkmConfigStr->hc2_timer14)) {
      updateWebText("p04_hc2_t14", pkmConfigStr->hc2_timer14, false);
    }
  }

  // hot-water config values
  if (config.km271.use_ww) {
    if (strDiff(&KmCfgHash[ww_priority], pkmConfigStr->ww_priority)) {
      updateWebText("p05_hw_prio", pkmConfigStr->ww_priority, false);
    } else if (strDiff(&KmCfgHash[ww_temp], pkmConfigStr->ww_temp)) {
      updateWebValueInt("p02_ww_temp", pkmConfigNum->ww_temp);
      updateWebTextInt("p02_ww_temp_txt", pkmConfigNum->ww_temp, false);
      updateWebText("p05_hw_temp", pkmConfigStr->ww_temp, false);
    } else if (strDiff(&KmCfgHash[ww_operation_mode], pkmConfigStr->ww_operation_mode)) {
      updateWebState("p02_ww_opmode_night", pkmConfigNum->ww_operation_mode == 0 ? true : false);
      updateWebState("p02_ww_opmode_day", pkmConfigNum->ww_operation_mode == 1 ? true : false);
      updateWebState("p02_ww_opmode_auto", pkmConfigNum->ww_operation_mode == 2 ? true : false);
      updateWebText("p05_hw_op_mode", pkmConfigStr->ww_operation_mode, false);
    } else if (strDiff(&KmCfgHash[ww_processing], pkmConfigStr->ww_processing)) {
      updateWebText("p05_hw_proccess", pkmConfigStr->ww_processing, false);
    } else if (strDiff(&KmCfgHash[ww_circulation], pkmConfigStr->ww_circulation)) {
      updateWebValueInt("p02_ww_circ", pkmConfigNum->ww_circulation);
      updateWebTextInt("p02_ww_circ_txt", pkmConfigNum->ww_circulation, false);
      updateWebText("p05_hw_circ", pkmConfigStr->ww_circulation, false);
    }
  }

  // general config values
  if (strDiff(&KmCfgHash[language], pkmConfigStr->language)) {
    updateWebText("p07_language", pkmConfigStr->language, false);
  } else if (strDiff(&KmCfgHash[display], pkmConfigStr->display)) {
    updateWebText("p07_display", pkmConfigStr->display, false);
  } else if (strDiff(&KmCfgHash[burner_type], pkmConfigStr->burner_type)) {
    updateWebText("p07_burner_type", pkmConfigStr->burner_type, false);
  } else if (strDiff(&KmCfgHash[max_boiler_temperature], pkmConfigStr->max_boiler_temperature)) {
    updateWebText("p06_max_boil_temp", pkmConfigStr->max_boiler_temperature, false);
  } else if (strDiff(&KmCfgHash[pump_logic_temp], pkmConfigStr->pump_logic_temp)) {
    updateWebText("p07_pump_logic_temp", pkmConfigStr->pump_logic_temp, false);
  } else if (strDiff(&KmCfgHash[exhaust_gas_temperature_threshold], pkmConfigStr->exhaust_gas_temperature_threshold)) {
    updateWebText("p07_exh_gas_temp_th", pkmConfigStr->exhaust_gas_temperature_threshold, false);
  } else if (strDiff(&KmCfgHash[burner_min_modulation], pkmConfigStr->burner_min_modulation)) {
    updateWebText("p07_burner_min_mod", pkmConfigStr->burner_min_modulation, false);
  } else if (strDiff(&KmCfgHash[burner_modulation_runtime], pkmConfigStr->burner_modulation_runtime)) {
    updateWebText("p07_burner_mod_run", pkmConfigStr->burner_modulation_runtime, false);
  } else if (strDiff(&KmCfgHash[building_type], pkmConfigStr->building_type)) {
    updateWebText("p07_building_type", pkmConfigStr->building_type, false);
  } else if (strDiff(&KmCfgHash[time_offset], pkmConfigStr->time_offset)) {
    updateWebText("p07_time_offset", pkmConfigStr->time_offset, false);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271StatusElements() {

  // heating circuit 1 values
  if (config.km271.use_hc1) {
    if ((kmStatusCpy.HC1_OperatingStates_1 != pkmStatus->HC1_OperatingStates_1) ||
        (kmStatusCpy.HC1_OperatingStates_2 != pkmStatus->HC1_OperatingStates_2)) {
      kmStatusCpy.HC1_OperatingStates_1 = pkmStatus->HC1_OperatingStates_1;
      kmStatusCpy.HC1_OperatingStates_2 = pkmStatus->HC1_OperatingStates_2;

      // AUTOMATIC / MANUAL (Day/Night)
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc1_opmode", WEB_TXT::AUTOMATIC[config.lang], false);
        updateWebSetIcon("p01_hc1_opmode_icon", "i_auto");
      } else { // MANUAL
        updateWebSetIcon("p01_hc1_opmode_icon", "i_manual");
        updateWebText("p01_hc1_opmode",
                      bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang], false);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc1_summer_winter_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc1_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]),
            false);
        updateWebSetIcon("p01_hc1_summer_winter_icon",
                         (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? "i_summer" : "i_winter"));
      }
      updateWebText("p03_hc1_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)), false);
      updateWebText("p03_hc1_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)), false);
      updateWebText("p03_hc1_ov1_auto", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)), false);
      updateWebText("p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)), false);
      updateWebText("p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)), false);
      updateWebText("p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)), false);
      updateWebText("p03_hc1_ov1_frost", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)), false);

      // Day / Night
      updateWebText("p01_hc1_day_night", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]),
                    false);
      updateWebSetIcon("p01_hc1_day_night_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? "i_day" : "i_night"));

      updateWebText("p03_hc1_ov2_summer", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)), false);
      updateWebText("p03_hc1_ov2_day", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)), false);
      updateWebText("p03_hc1_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)), false);
      updateWebText("p03_hc1_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)), false);
      updateWebText("p03_hc1_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)), false);
      updateWebText("p03_hc1_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)), false);
      updateWebText("p03_hc1_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)), false);
    } else if (kmStatusCpy.HC1_HeatingForwardTargetTemp != pkmStatus->HC1_HeatingForwardTargetTemp) {
      kmStatusCpy.HC1_HeatingForwardTargetTemp = pkmStatus->HC1_HeatingForwardTargetTemp;
      updateWebTextInt("p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
      updateWebTextInt("p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    } else if (kmStatusCpy.HC1_HeatingForwardActualTemp != pkmStatus->HC1_HeatingForwardActualTemp) {
      kmStatusCpy.HC1_HeatingForwardActualTemp = pkmStatus->HC1_HeatingForwardActualTemp;
      updateWebTextInt("p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardActualTemp, false);
      updateWebTextInt("p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardActualTemp, false);
    } else if (kmStatusCpy.HC1_RoomTargetTemp != pkmStatus->HC1_RoomTargetTemp) {
      kmStatusCpy.HC1_RoomTargetTemp = pkmStatus->HC1_RoomTargetTemp;
      updateWebTextInt("p03_hc1_room_set", kmStatusCpy.HC1_RoomTargetTemp, false);
    } else if (kmStatusCpy.HC1_RoomActualTemp != pkmStatus->HC1_RoomActualTemp) {
      kmStatusCpy.HC1_RoomActualTemp = pkmStatus->HC1_RoomActualTemp;
      updateWebTextInt("p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp, false);
    } else if (kmStatusCpy.HC1_SwitchOnOptimizationTime != pkmStatus->HC1_SwitchOnOptimizationTime) {
      kmStatusCpy.HC1_SwitchOnOptimizationTime = pkmStatus->HC1_SwitchOnOptimizationTime;
      updateWebTextInt("p03_hc1_on_time_opt_dur", kmStatusCpy.HC1_SwitchOnOptimizationTime, false);
    } else if (kmStatusCpy.HC1_SwitchOffOptimizationTime != pkmStatus->HC1_SwitchOffOptimizationTime) {
      kmStatusCpy.HC1_SwitchOffOptimizationTime = pkmStatus->HC1_SwitchOffOptimizationTime;
      updateWebTextInt("p03_hc1_off_time_opt_dur", kmStatusCpy.HC1_SwitchOffOptimizationTime, false);
    } else if (kmStatusCpy.HC1_PumpPower != pkmStatus->HC1_PumpPower) {
      kmStatusCpy.HC1_PumpPower = pkmStatus->HC1_PumpPower;
      updateWebText("p01_hc1_pump", (kmStatusCpy.HC1_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]), false);
      updateWebTextInt("p03_hc1_pump", kmStatusCpy.HC1_PumpPower, false);
    } else if (kmStatusCpy.HC1_MixingValue != pkmStatus->HC1_MixingValue) {
      kmStatusCpy.HC1_MixingValue = pkmStatus->HC1_MixingValue;
      updateWebTextInt("p03_hc1_mixer", kmStatusCpy.HC1_MixingValue, false);
    } else if (kmStatusCpy.HC1_HeatingCurvePlus10 != pkmStatus->HC1_HeatingCurvePlus10) {
      kmStatusCpy.HC1_HeatingCurvePlus10 = pkmStatus->HC1_HeatingCurvePlus10;
      updateWebTextInt("p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10, false);
    } else if (kmStatusCpy.HC1_HeatingCurve0 != pkmStatus->HC1_HeatingCurve0) {
      kmStatusCpy.HC1_HeatingCurve0 = pkmStatus->HC1_HeatingCurve0;
      updateWebTextInt("p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0, false);
    } else if (kmStatusCpy.HC1_HeatingCurveMinus10 != pkmStatus->HC1_HeatingCurveMinus10) {
      kmStatusCpy.HC1_HeatingCurveMinus10 = pkmStatus->HC1_HeatingCurveMinus10;
      updateWebTextInt("p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10, false);
    }
  } // END HC1

  // heating circuit 2 values
  if (config.km271.use_hc2) {
    if ((kmStatusCpy.HC2_OperatingStates_1 != pkmStatus->HC2_OperatingStates_1) ||
        (kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2)) {
      kmStatusCpy.HC2_OperatingStates_1 = pkmStatus->HC2_OperatingStates_1;
      kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2;

      // AUTOMATIC / MANUAL (Day/Night)
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc2_opmode", WEB_TXT::AUTOMATIC[config.lang], false);
        updateWebSetIcon("p01_hc2_opmode_icon", "i_auto");
      } else { // MANUAL
        updateWebSetIcon("p01_hc2_opmode_icon", "i_manual");
        updateWebText("p01_hc2_opmode",
                      bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang], false);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc2_summer_winter_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc2_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]),
            false);
        updateWebSetIcon("p01_hc2_summer_winter_icon",
                         (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? "i_summer" : "i_winter"));
      }

      updateWebText("p04_hc2_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)), false);
      updateWebText("p04_hc2_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)), false);
      updateWebText("p04_hc2_ov1_auto", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)), false);
      updateWebText("p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)), false);
      updateWebText("p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)), false);
      updateWebText("p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)), false);
      updateWebText("p04_hc2_ov1_frost", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)), false);

      // Day / Night
      updateWebText("p01_hc2_day_night", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]),
                    false);
      updateWebSetIcon("p01_hc2_day_night_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? "i_day" : "i_night"));

      updateWebText("p04_hc2_ov2_summer", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)), false);
      updateWebText("p04_hc2_ov2_day", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)), false);
      updateWebText("p04_hc2_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)), false);
      updateWebText("p04_hc2_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)), false);
      updateWebText("p04_hc2_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)), false);
      updateWebText("p04_hc2_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)), false);
      updateWebText("p04_hc2_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)), false);
    } else if (kmStatusCpy.HC2_HeatingForwardTargetTemp != pkmStatus->HC2_HeatingForwardTargetTemp) {
      kmStatusCpy.HC2_HeatingForwardTargetTemp = pkmStatus->HC2_HeatingForwardTargetTemp;
      updateWebTextInt("p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
      updateWebTextInt("p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
    } else if (kmStatusCpy.HC2_HeatingForwardActualTemp != pkmStatus->HC2_HeatingForwardActualTemp) {
      kmStatusCpy.HC2_HeatingForwardActualTemp = pkmStatus->HC2_HeatingForwardActualTemp;
      updateWebTextInt("p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
      updateWebTextInt("p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
    } else if (kmStatusCpy.HC2_RoomTargetTemp != pkmStatus->HC2_RoomTargetTemp) {
      kmStatusCpy.HC2_RoomTargetTemp = pkmStatus->HC2_RoomTargetTemp;
      updateWebTextInt("p04_hc2_room_set", kmStatusCpy.HC2_RoomTargetTemp, false);
    } else if (kmStatusCpy.HC2_RoomActualTemp != pkmStatus->HC2_RoomActualTemp) {
      kmStatusCpy.HC2_RoomActualTemp = pkmStatus->HC2_RoomActualTemp;
      updateWebTextInt("p04_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp, false);
    } else if (kmStatusCpy.HC2_SwitchOnOptimizationTime != pkmStatus->HC2_SwitchOnOptimizationTime) {
      kmStatusCpy.HC2_SwitchOnOptimizationTime = pkmStatus->HC2_SwitchOnOptimizationTime;
      updateWebTextInt("p04_hc2_on_time_opt_dur", kmStatusCpy.HC2_SwitchOnOptimizationTime, false);
    } else if (kmStatusCpy.HC2_SwitchOffOptimizationTime != pkmStatus->HC2_SwitchOffOptimizationTime) {
      kmStatusCpy.HC2_SwitchOffOptimizationTime = pkmStatus->HC2_SwitchOffOptimizationTime;
      updateWebTextInt("p04_hc2_off_time_opt_dur", kmStatusCpy.HC2_SwitchOffOptimizationTime, false);
    } else if (kmStatusCpy.HC2_PumpPower != pkmStatus->HC2_PumpPower) {
      kmStatusCpy.HC2_PumpPower = pkmStatus->HC2_PumpPower;
      updateWebText("p01_hc2_pump", (kmStatusCpy.HC2_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]), false);
      updateWebTextInt("p04_hc2_pump", kmStatusCpy.HC2_PumpPower, false);
    } else if (kmStatusCpy.HC2_MixingValue != pkmStatus->HC2_MixingValue) {
      kmStatusCpy.HC2_MixingValue = pkmStatus->HC2_MixingValue;
      updateWebTextInt("p04_hc2_mixer", kmStatusCpy.HC2_MixingValue, false);
    } else if (kmStatusCpy.HC2_HeatingCurvePlus10 != pkmStatus->HC2_HeatingCurvePlus10) {
      kmStatusCpy.HC2_HeatingCurvePlus10 = pkmStatus->HC2_HeatingCurvePlus10;
      updateWebTextInt("p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10, false);
    } else if (kmStatusCpy.HC2_HeatingCurve0 != pkmStatus->HC2_HeatingCurve0) {
      kmStatusCpy.HC2_HeatingCurve0 = pkmStatus->HC2_HeatingCurve0;
      updateWebTextInt("p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0, false);
    } else if (kmStatusCpy.HC2_HeatingCurveMinus10 != pkmStatus->HC2_HeatingCurveMinus10) {
      kmStatusCpy.HC2_HeatingCurveMinus10 = pkmStatus->HC2_HeatingCurveMinus10;
      updateWebTextInt("p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10, false);
    }
  } // END HC2

  // hot-water values
  if (config.km271.use_ww) {
    if (kmStatusCpy.HotWaterOperatingStates_1 != pkmStatus->HotWaterOperatingStates_1) {
      kmStatusCpy.HotWaterOperatingStates_1 = pkmStatus->HotWaterOperatingStates_1;
      // WW-Operating State
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) { // AUTOMATIC
        snprintf(tmpMessage, sizeof(tmpMessage), "%s", WEB_TXT::AUTOMATIC[config.lang]);
        updateWebSetIcon("p01_ww_opmode_icon", "i_auto");
      } else {                                                   // MANUAL
        if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::DAY[config.lang]);
          updateWebSetIcon("p01_ww_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::NIGHT[config.lang]);
          updateWebSetIcon("p01_ww_opmode_icon", "i_manual");
        }
      }
      updateWebText("p01_ww_opmode", tmpMessage, false);

      updateWebText("p05_hw_ov1_auto", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)), false);
      updateWebText("p05_hw_ov1_disinfect", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)), false);
      updateWebText("p05_hw_ov1_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)), false);
      updateWebText("p05_hw_ov1_holiday", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)), false);
      updateWebText("p05_hw_ov1_err_disinfect", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)), false);
      updateWebText("p05_hw_ov1_err_sensor", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)), false);
      updateWebText("p05_hw_ov1_stays_cold", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)), false);
      updateWebText("p05_hw_ov1_err_anode", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)), false);
    } else if (kmStatusCpy.HotWaterOperatingStates_2 != pkmStatus->HotWaterOperatingStates_2) {
      kmStatusCpy.HotWaterOperatingStates_2 = pkmStatus->HotWaterOperatingStates_2;
      updateWebText("p05_hw_ov2_load", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)), false);
      updateWebText("p05_hw_ov2_manual", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)), false);
      updateWebText("p05_hw_ov2_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)), false);
      updateWebText("p05_hw_ov2_off_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)), false);
      updateWebText("p05_hw_ov2_on_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)), false);
      updateWebText("p05_hw_ov2_day", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)), false);
      updateWebText("p05_hw_ov2_hot", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)), false);
      updateWebText("p05_hw_ov2_prio", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)), false);
    } else if (kmStatusCpy.HotWaterTargetTemp != pkmStatus->HotWaterTargetTemp) {
      kmStatusCpy.HotWaterTargetTemp = pkmStatus->HotWaterTargetTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterTargetTemp);
      updateWebText("p05_hw_set_temp", tmpMessage, false);
      updateWebTextInt("p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp, false);
    } else if (kmStatusCpy.HotWaterActualTemp != pkmStatus->HotWaterActualTemp) {
      kmStatusCpy.HotWaterActualTemp = pkmStatus->HotWaterActualTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterActualTemp);
      updateWebText("p05_hw_act_temp", tmpMessage, false);
      updateWebTextInt("p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp, false);
    } else if (kmStatusCpy.HotWaterOptimizationTime != pkmStatus->HotWaterOptimizationTime) {
      kmStatusCpy.HotWaterOptimizationTime = pkmStatus->HotWaterOptimizationTime;
      updateWebText("p05_hw_on_time_opt_dur", onOffString(kmStatusCpy.HotWaterOptimizationTime), false);
    } else if (kmStatusCpy.HotWaterPumpStates != pkmStatus->HotWaterPumpStates) {
      kmStatusCpy.HotWaterPumpStates = pkmStatus->HotWaterPumpStates;
      updateWebText("p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)), false);
      updateWebText("p05_hw_pump_type_circ", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)), false);
      updateWebText("p05_hw_pump_type_gwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)), false);
    }
  } // END HotWater

  // general values
  if (kmStatusCpy.BoilerForwardTargetTemp != pkmStatus->BoilerForwardTargetTemp) {
    kmStatusCpy.BoilerForwardTargetTemp = pkmStatus->BoilerForwardTargetTemp;
    updateWebTextInt("p06_boil_set", kmStatusCpy.BoilerForwardTargetTemp, false);
    updateWebTextInt("p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp, false);
  } else if (kmStatusCpy.BoilerForwardActualTemp != pkmStatus->BoilerForwardActualTemp) {
    kmStatusCpy.BoilerForwardActualTemp = pkmStatus->BoilerForwardActualTemp;
    updateWebTextInt("p06_boil_temp", kmStatusCpy.BoilerForwardActualTemp, false);
    updateWebTextInt("p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp, false);
  } else if (kmStatusCpy.BurnerSwitchOnTemp != pkmStatus->BurnerSwitchOnTemp) {
    kmStatusCpy.BurnerSwitchOnTemp = pkmStatus->BurnerSwitchOnTemp;
    updateWebTextInt("p06_boil_sw_on_temp", kmStatusCpy.BurnerSwitchOnTemp, false);
  } else if (kmStatusCpy.BurnerSwitchOffTemp != pkmStatus->BurnerSwitchOffTemp) {
    kmStatusCpy.BurnerSwitchOffTemp = pkmStatus->BurnerSwitchOffTemp;
    updateWebTextInt("p06_boil_sw_off_temp", kmStatusCpy.BurnerSwitchOffTemp, false);
  } else if (kmStatusCpy.BoilerIntegral_1 != pkmStatus->BoilerIntegral_1) {
    kmStatusCpy.BoilerIntegral_1 = pkmStatus->BoilerIntegral_1;
  } else if (kmStatusCpy.BoilerIntegral_2 != pkmStatus->BoilerIntegral_2) {
    kmStatusCpy.BoilerIntegral_2 = pkmStatus->BoilerIntegral_2;
  } else if (kmStatusCpy.BoilerErrorStates != pkmStatus->BoilerErrorStates) {
    kmStatusCpy.BoilerErrorStates = pkmStatus->BoilerErrorStates;

    updateWebText("p06_boil_fail_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)), false);
    updateWebText("p06_boil_fail_boiler_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)), false);
    updateWebText("p06_boil_fail_aux_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)), false);
    updateWebText("p06_boil_fail_boiler_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)), false);
    updateWebText("p06_boil_fail_exh_gas_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)), false);
    updateWebText("p06_boil_fail_exh_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)), false);
    updateWebText("p06_boil_fail_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)), false);
    updateWebText("p06_boil_fail_ext", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)), false);
  } else if (kmStatusCpy.BoilerOperatingStates != pkmStatus->BoilerOperatingStates) {
    kmStatusCpy.BoilerOperatingStates = pkmStatus->BoilerOperatingStates;

    updateWebText("p06_boil_state_exh_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)), false);
    updateWebText("p06_boil_s_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)), false);
    updateWebText("p06_boil_st_boiler_prot", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)), false);
    updateWebText("p06_boil_s_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)), false);
    updateWebText("p06_boil_s_perf_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)), false);
    updateWebText("p06_boil_s_perf_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)), false);
    updateWebText("p06_boil_s_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)), false);
  } else if (kmStatusCpy.BurnerStates != pkmStatus->BurnerStates) {
    kmStatusCpy.BurnerStates = pkmStatus->BurnerStates;
    updateWebText("p01_burner", (kmStatusCpy.BurnerStates == 0) ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang], false);
    updateWebText("p06_burn_ctrl", KM_CFG_ARRAY::BURNER_STATE[config.lang][kmStatusCpy.BurnerStates], false);
  } else if (kmStatusCpy.ExhaustTemp != pkmStatus->ExhaustTemp) {
    kmStatusCpy.ExhaustTemp = pkmStatus->ExhaustTemp;
    updateWebTextInt("p07_exh_gas_temp", kmStatusCpy.ExhaustTemp, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_2 != pkmStatus->BurnerOperatingDuration_2) {
    kmStatusCpy.BurnerOperatingDuration_2 = pkmStatus->BurnerOperatingDuration_2;
  } else if (kmStatusCpy.BurnerOperatingDuration_1 != pkmStatus->BurnerOperatingDuration_1) {
    kmStatusCpy.BurnerOperatingDuration_1 = pkmStatus->BurnerOperatingDuration_1;
  } else if (kmStatusCpy.BurnerOperatingDuration_0 != pkmStatus->BurnerOperatingDuration_0) {
    kmStatusCpy.BurnerOperatingDuration_0 = pkmStatus->BurnerOperatingDuration_0;
  } else if (kmStatusCpy.BurnerOperatingDuration_Sum != pkmStatus->BurnerOperatingDuration_Sum) {
    kmStatusCpy.BurnerOperatingDuration_Sum = pkmStatus->BurnerOperatingDuration_Sum;
    timeComponents burnerRuntime = convertMinutes(kmStatusCpy.BurnerOperatingDuration_Sum);
    updateWebTextInt("p06_burn_run_years", burnerRuntime.years, false);
    updateWebTextInt("p06_burn_run_days", burnerRuntime.days, false);
    updateWebTextInt("p06_burn_run_hours", burnerRuntime.hours, false);
    updateWebTextInt("p06_burn_run_min", burnerRuntime.minutes, false);
  } else if (kmStatusCpy.BurnerCalcOilConsumption != pkmStatus->BurnerCalcOilConsumption) {
    kmStatusCpy.BurnerCalcOilConsumption = pkmStatus->BurnerCalcOilConsumption;
    // is handled somewhere else
  } else if (kmStatusCpy.OutsideTemp != pkmStatus->OutsideTemp) {
    kmStatusCpy.OutsideTemp = pkmStatus->OutsideTemp;
    updateWebTextInt("p07_out_temp", kmStatusCpy.OutsideTemp, false);
    updateWebTextInt("p01_temp_out_act", kmStatusCpy.OutsideTemp, false);
  } else if (kmStatusCpy.OutsideDampedTemp != pkmStatus->OutsideDampedTemp) {
    kmStatusCpy.OutsideDampedTemp = pkmStatus->OutsideDampedTemp;
    updateWebTextInt("p07_out_temp_damped", kmStatusCpy.OutsideDampedTemp, false);
    updateWebTextInt("p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp, false);
  } else if (kmStatusCpy.ControllerVersionMain != pkmStatus->ControllerVersionMain) {
    kmStatusCpy.ControllerVersionMain = pkmStatus->ControllerVersionMain;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  } else if (kmStatusCpy.ControllerVersionSub != pkmStatus->ControllerVersionSub) {
    kmStatusCpy.ControllerVersionSub = pkmStatus->ControllerVersionSub;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  } else if (kmStatusCpy.Modul != pkmStatus->Modul) {
    kmStatusCpy.Modul = pkmStatus->Modul;
    updateWebTextInt("p09_logamatic_modul", kmStatusCpy.Modul, false);
  } else if (kmStatusCpy.ERR_Alarmstatus != pkmStatus->ERR_Alarmstatus) {
    kmStatusCpy.ERR_Alarmstatus = pkmStatus->ERR_Alarmstatus;
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271StatusElementsAll() {

  km271GetStatusValueCopy(&kmStatusCpy);

  initJsonBuffer(jsonDoc);

  // heating circuit 1 values
  if (config.km271.use_hc1) {
    // AUTOMATIC / MANUAL (Day/Night)
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonDoc, "p01_hc1_opmode", WEB_TXT::AUTOMATIC[config.lang]);
      addJsonIcon(jsonDoc, "p01_hc1_opmode_icon", "i_auto");
    } else { // MANUAL
      addJsonLabelTxt(jsonDoc, "p01_hc1_opmode",
                      bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang]);
      addJsonIcon(jsonDoc, "p01_hc1_opmode_icon", "i_manual");
    }

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonDoc, "p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
      addJsonIcon(jsonDoc, "p01_hc1_summer_winter_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
    } else { // generate status from actual temperature and summer threshold
      addJsonLabelTxt(
          jsonDoc, "p01_hc1_summer_winter",
          (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
      addJsonIcon(jsonDoc, "p01_hc1_summer_winter_icon",
                  (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? "i_summer" : "i_winter"));
    }

    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_auto", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov1_frost", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)));

    // Day / Night
    addJsonLabelTxt(jsonDoc, "p01_hc1_day_night",
                    (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]));
    addJsonIcon(jsonDoc, "p01_hc1_day_night_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? "i_day" : "i_night"));

    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_summer", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_day", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)));
    addJsonLabelTxt(jsonDoc, "p03_hc1_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)));
    addJsonLabelInt(jsonDoc, "p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p03_hc1_room_set", kmStatusCpy.HC1_RoomTargetTemp);
    addJsonLabelInt(jsonDoc, "p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp);
    addJsonLabelInt(jsonDoc, "p03_hc1_on_time_opt_dur", kmStatusCpy.HC1_SwitchOnOptimizationTime);
    addJsonLabelInt(jsonDoc, "p03_hc1_off_time_opt_dur", kmStatusCpy.HC1_SwitchOffOptimizationTime);
    addJsonLabelTxt(jsonDoc, "p01_hc1_pump", (kmStatusCpy.HC1_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]));
    addJsonLabelInt(jsonDoc, "p03_hc1_pump", kmStatusCpy.HC1_PumpPower);
    addJsonLabelInt(jsonDoc, "p03_hc1_mixer", kmStatusCpy.HC1_MixingValue);
    addJsonLabelInt(jsonDoc, "p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10);
    addJsonLabelInt(jsonDoc, "p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0);
    addJsonLabelInt(jsonDoc, "p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10);
  } // END HC1

  // heating circuit 2 values
  if (config.km271.use_hc2) {
    // HC2-Operating State
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonDoc, "p01_hc2_opmode", WEB_TXT::AUTOMATIC[config.lang]);
      addJsonIcon(jsonDoc, "p01_hc2_opmode_icon", "i_auto");
    } else { // MANUAL
      addJsonLabelTxt(jsonDoc, "p01_hc2_opmode",
                      bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang]);
      addJsonIcon(jsonDoc, "p01_hc2_opmode_icon", "i_manual");
    }

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonDoc, "p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
      addJsonIcon(jsonDoc, "p01_hc2_summer_winter_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
    } else { // generate status from actual temperature and summer threshold
      addJsonLabelTxt(
          jsonDoc, "p01_hc2_summer_winter",
          (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
      addJsonIcon(jsonDoc, "p01_hc2_summer_winter_icon",
                  (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? "i_summer" : "i_winter"));
    }

    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_auto", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov1_frost", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)));
    // Day / Night
    addJsonLabelTxt(jsonDoc, "p01_hc2_day_night",
                    (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]));
    addJsonIcon(jsonDoc, "p01_hc2_day_night_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? "i_day" : "i_night"));

    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_summer", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_day", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)));
    addJsonLabelTxt(jsonDoc, "p04_hc2_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)));
    addJsonLabelInt(jsonDoc, "p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonDoc, "p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
    addJsonLabelInt(jsonDoc, "p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
    addJsonLabelInt(jsonDoc, "p04_hc2_room_set", kmStatusCpy.HC2_RoomTargetTemp);
    addJsonLabelInt(jsonDoc, "p04_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp);
    addJsonLabelInt(jsonDoc, "p04_hc2_on_time_opt_dur", kmStatusCpy.HC2_SwitchOnOptimizationTime);
    addJsonLabelInt(jsonDoc, "p04_hc2_off_time_opt_dur", kmStatusCpy.HC2_SwitchOffOptimizationTime);
    addJsonLabelTxt(jsonDoc, "p01_hc2_pump", (kmStatusCpy.HC2_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]));
    addJsonLabelInt(jsonDoc, "p04_hc2_pump", kmStatusCpy.HC2_PumpPower);
    addJsonLabelInt(jsonDoc, "p04_hc2_mixer", kmStatusCpy.HC2_MixingValue);
    addJsonLabelInt(jsonDoc, "p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10);
    addJsonLabelInt(jsonDoc, "p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0);
    addJsonLabelInt(jsonDoc, "p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10);

  } // END HC2

  // hot-water values
  if (config.km271.use_ww) {
    // WW-Operating State
    if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) { // AUTOMATIC
      addJsonLabelTxt(jsonDoc, "p01_ww_opmode", WEB_TXT::AUTOMATIC[config.lang]);
      addJsonIcon(jsonDoc, "p01_ww_opmode_icon", "i_auto");
    } else {                                                   // MANUAL
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
        addJsonLabelTxt(jsonDoc, "p01_ww_opmode", WEB_TXT::MAN_DAY[config.lang]);
        addJsonIcon(jsonDoc, "p01_ww_opmode_icon", "i_manual");
      } else { // NIGHT
        addJsonLabelTxt(jsonDoc, "p01_ww_opmode", WEB_TXT::MAN_NIGHT[config.lang]);
        addJsonIcon(jsonDoc, "p01_ww_opmode_icon", "i_manual");
      }
    }

    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_auto", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_disinfect", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_holiday", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_err_disinfect", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_err_sensor", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_stays_cold", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov1_err_anode", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_load", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_manual", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_off_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_on_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_day", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_hot", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)));
    addJsonLabelTxt(jsonDoc, "p05_hw_ov2_prio", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)));
    snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterTargetTemp);
    addJsonLabelTxt(jsonDoc, "p05_hw_set_temp", tmpMessage);
    addJsonLabelInt(jsonDoc, "p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp);
    snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterActualTemp);
    addJsonLabelTxt(jsonDoc, "p05_hw_act_temp", tmpMessage);
    addJsonLabelInt(jsonDoc, "p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp);
    addJsonLabelTxt(jsonDoc, "p05_hw_on_time_opt_dur", onOffString(kmStatusCpy.HotWaterOptimizationTime));
    addJsonLabelTxt(jsonDoc, "p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)));
    addJsonLabelTxt(jsonDoc, "p05_hw_pump_type_circ", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)));
    addJsonLabelTxt(jsonDoc, "p05_hw_pump_type_gwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)));
  } // END HotWater

  // general values
  addJsonLabelInt(jsonDoc, "p06_boil_set", kmStatusCpy.BoilerForwardTargetTemp);
  addJsonLabelInt(jsonDoc, "p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp);
  addJsonLabelInt(jsonDoc, "p06_boil_temp", kmStatusCpy.BoilerForwardActualTemp);
  addJsonLabelInt(jsonDoc, "p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp);
  addJsonLabelInt(jsonDoc, "p06_boil_sw_on_temp", kmStatusCpy.BurnerSwitchOnTemp);
  addJsonLabelInt(jsonDoc, "p06_boil_sw_off_temp", kmStatusCpy.BurnerSwitchOffTemp);

  addJsonLabelTxt(jsonDoc, "p06_boil_fail_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_boiler_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_aux_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_boiler_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_exh_gas_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_exh_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)));
  addJsonLabelTxt(jsonDoc, "p06_boil_fail_ext", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)));

  addJsonLabelTxt(jsonDoc, "p06_boil_state_exh_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)));
  addJsonLabelTxt(jsonDoc, "p06_boil_s_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)));
  addJsonLabelTxt(jsonDoc, "p06_boil_st_boiler_prot", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)));
  addJsonLabelTxt(jsonDoc, "p06_boil_s_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)));
  addJsonLabelTxt(jsonDoc, "p06_boil_s_perf_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)));
  addJsonLabelTxt(jsonDoc, "p06_boil_s_perf_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)));
  addJsonLabelTxt(jsonDoc, "p06_boil_s_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)));
  addJsonLabelTxt(jsonDoc, "p01_burner", (kmStatusCpy.BurnerStates == 0) ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]);
  addJsonLabelTxt(jsonDoc, "p06_burn_ctrl", KM_CFG_ARRAY::BURNER_STATE[config.lang][kmStatusCpy.BurnerStates]);
  addJsonLabelInt(jsonDoc, "p07_exh_gas_temp", kmStatusCpy.ExhaustTemp);
  timeComponents burnerRuntime = convertMinutes(kmStatusCpy.BurnerOperatingDuration_Sum);
  addJsonLabelInt(jsonDoc, "p06_burn_run_years", burnerRuntime.years);
  addJsonLabelInt(jsonDoc, "p06_burn_run_days", burnerRuntime.days);
  addJsonLabelInt(jsonDoc, "p06_burn_run_hours", burnerRuntime.hours);
  addJsonLabelInt(jsonDoc, "p06_burn_run_min", burnerRuntime.minutes);
  addJsonLabelInt(jsonDoc, "p07_out_temp", kmStatusCpy.OutsideTemp);
  addJsonLabelInt(jsonDoc, "p01_temp_out_act", kmStatusCpy.OutsideTemp);
  addJsonLabelInt(jsonDoc, "p07_out_temp_damped", kmStatusCpy.OutsideDampedTemp);
  addJsonLabelInt(jsonDoc, "p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp);
  static char ControllerVersion[32];
  snprintf(ControllerVersion, sizeof(ControllerVersion), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
  addJsonLabelTxt(jsonDoc, "p09_logamatic_version", ControllerVersion);
  addJsonLabelInt(jsonDoc, "p09_logamatic_modul", kmStatusCpy.Modul);

  updateWebJSON(jsonDoc);
}

void webUIupdates() {

  // ON-BROWSER-REFRESH: refresh ALL elements - do this step by step not to stress the connection
  if (refreshTimer3.cycleTrigger(WEBUI_JSON_REFRESH_TIME_MS) && refreshRequest && !otaActiveState()) {
    switch (UpdateCntRefresh) {
    case 0:
      updateSystemInfoElementsStatic(); // update static informations (≈ 200 Bytes)
      break;
    case 1:
      updateKm271ConfigElementsAll(); // send all "Config" elements as one big JSON update ( ≈ 4 kB)
      break;
    case 2:
      updateKm271StatusElementsAll(); // send all "Status" elements as one big JSON update (≈ 4 kB)
      refreshRequest = false;
      break;
    default:
      UpdateCntRefresh = -1;
      break;
    }
    UpdateCntRefresh = (UpdateCntRefresh + 1) % 3;
  }

  // CHANGE-BASED: check and refresh single elemets if they have changed - do this step by step not to stress the connection
  if (refreshTimer1.cycleTrigger(WEBUI_FAST_REFRESH_TIME_MS) && !otaActiveState()) {

    if (webLogRefreshActive()) {
      webReadLogBufferCyclic(); // webUI logger has to be updated
    } else {
      switch (UpdateCntFast) {
      case 0:
        updateKm271AlarmElements(); // check if one or more "Alarm" elements have changed
        break;
      case 1:
        updateKm271StatusElements(); // check if one or more "Status" elements have changed
        break;
      case 2:
        updateKm271ConfigElements(); // check if one or more "Config" elements have changed
        break;
      default:
        UpdateCntFast = -1;
        break;
      }
      UpdateCntFast = (UpdateCntFast + 1) % 3;
    }
  }

  // CYCLIC: update elemets every 1 seconds - do this step by step not to stress the connection
  if (refreshTimer2.cycleTrigger(WEBUI_SLOW_REFRESH_TIME_MS) && !otaActiveState()) {
    switch (UpdateCntSlow) {
    case 0:
      updateSystemInfoElements(); // refresh all "System" elements as one big JSON update (≈ 570 Bytes)
      break;
    case 1:
      updateOilmeterElements(false); // check oilmeter and refresh if changed
      break;
    case 2:
      updateSensorElements(); // send update of sensor elements
      break;
    default:
      UpdateCntSlow = -1;
      break;
    }
    UpdateCntSlow = (UpdateCntSlow + 1) % 3;

    // show refresh layer
    if (km271GetRefreshState()) {
      updateWebHideElement("refreshBar", false);
      km271RefreshActiveOld = true;
    } else if (km271RefreshActiveOld != km271GetRefreshState()) {
      km271RefreshActiveOld = false;
      updateWebHideElement("refreshBar", true);
    }
  }
}