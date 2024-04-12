
#include <webUI.h>
#include <webUIupdates.h>

/* S E T T I N G S ****************************************************/
#define WEBUI_FAST_REFRESH_TIME_MS 10
#define WEBUI_SLOW_REFRESH_TIME_MS 1000
#define WEBUI_JSON_REFRESH_TIME_MS 20

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
int UpdateCntFast = 0;
int UpdateCntSlow = 0;
int UpdateCntRefresh = 0;
s_lang LANG;
char jsonSet[2048];
char jsonCfg[5120];
char jsonStat[4096];

// initialize JSON-Buffer
void initJsonBuffer(char *buffer, size_t bufferSize) {
  if (bufferSize > 0) {
    buffer[0] = '{';
    buffer[1] = '\0';
  }
}

// add JSON Element to JSON-Buffer
bool addJsonElement(char *buffer, size_t bufferSize, const char *elementID, const char *typSuffix, const char *value) {
  size_t currentLength = strlen(buffer);
  size_t additionalLength = strlen(elementID) + strlen(typSuffix) + strlen(value) + 7; //+7 because of (":#,)
  if (currentLength + additionalLength < bufferSize - 2) {                             // -2 for buffer termination
    strcat(buffer, "\"");
    strcat(buffer, elementID);
    strcat(buffer, "#");
    strcat(buffer, typSuffix);
    strcat(buffer, "\":\"");
    strcat(buffer, value);
    strcat(buffer, "\",");
    return true;
  }
  return false;
  Serial.println(">>>>>>> JSON-Buffer to small!!! <<<<<<<");
}

// Terminate JSON-Buffer
void finalizeJsonBuffer(char *buffer, size_t bufferSize) {
  size_t length = strlen(buffer);
  if (length > 0 && buffer[length - 1] == ',') {
    buffer[length - 1] = '\0'; // remove trailing comma
  }
  if (length + 1 < bufferSize) {
    strcat(buffer, "}");
  }
}

void addJsonLabelInt(char *buffer, size_t bufferSize, const char *elementID, intmax_t value) {
  addJsonElement(buffer, bufferSize, elementID, "l", intmaxToString(value));
};

void addJsonLabelTxt(char *buffer, size_t bufferSize, const char *elementID, const char *value) {
  addJsonElement(buffer, bufferSize, elementID, "l", value);
};

void addJsonValueTxt(char *buffer, size_t bufferSize, const char *elementID, const char *value) {
  addJsonElement(buffer, bufferSize, elementID, "v", value);
};

void addJsonValueInt(char *buffer, size_t bufferSize, const char *elementID, intmax_t value) {
  addJsonElement(buffer, bufferSize, elementID, "v", intmaxToString(value));
};

void addJsonValueFlt(char *buffer, size_t bufferSize, const char *elementID, float value) {
  addJsonElement(buffer, bufferSize, elementID, "v", floatToString(value));
};

void addJsonValueFlt8(char *buffer, size_t bufferSize, const char *elementID, float value) {
  addJsonElement(buffer, bufferSize, elementID, "v", floatToString8(value));
};

void addJsonState(char *buffer, size_t bufferSize, const char *elementID, bool value) {
  addJsonElement(buffer, bufferSize, elementID, "c", (value ? "true" : "false"));
};

void addJsonIcon(char *buffer, size_t bufferSize, const char *elementID, const char *value) {
  addJsonElement(buffer, bufferSize, elementID, "i", value);
};

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
  updateSensorElements();
  updateSystemInfoElements();

  setLanguage(LANG.CODE[config.lang]);               // set language for webUI based on config
  showElementClass("simModeBar", config.sim.enable); // show SIMULATION_MODE in webUI based on config

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
void updateSettingsElements() {

  initJsonBuffer(jsonSet, sizeof(jsonSet));

  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_wifi_hostname", config.wifi.hostname);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_wifi_ssid", config.wifi.ssid);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_wifi_passw", config.wifi.password);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_ip_enable", config.ip.enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ip_adr", config.ip.ipaddress);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ip_subnet", config.ip.subnet);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ip_gateway", config.ip.gateway);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ip_dns", config.ip.dns);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_auth_enable", config.auth.enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_auth_user", config.auth.user);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_auth_passw", config.auth.password);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_mqtt_enable", config.mqtt.enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_mqtt_server", config.mqtt.server);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_mqtt_port", (config.mqtt.port));
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_mqtt_user", config.mqtt.user);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_mqtt_passw", config.mqtt.password);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_mqtt_topic", config.mqtt.topic);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_mqtt_lang", config.mqtt.lang);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_pushover_enable", config.pushover.enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_pushover_api_token", config.pushover.token);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_pushover_user_key", config.pushover.user_key);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_pushover_filter", config.pushover.filter);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_hc1_enable", config.km271.use_hc1);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_hc2_enable", config.km271.use_hc2);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_hw_enable", config.km271.use_ww);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_alarm_enable", config.km271.use_alarmMsg);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_km271_rx", config.gpio.km271_RX);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_km271_tx", config.gpio.km271_TX);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_led_heartbeat", config.gpio.led_heartbeat);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_led_logmode", config.gpio.led_logmode);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_led_wifi", config.gpio.led_wifi);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_led_oil", config.gpio.led_oilcounter);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_gpio_trig_oil", config.gpio.trigger_oilcounter);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_oil_hw_enable", config.oilmeter.use_hardware_meter);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_oil_virt_enable", config.oilmeter.use_virtual_meter);
  addJsonValueFlt8(jsonSet, sizeof(jsonSet), "p12_oil_par1_kg_h", config.oilmeter.consumption_kg_h);
  addJsonValueFlt8(jsonSet, sizeof(jsonSet), "p12_oil_par2_kg_l", config.oilmeter.oil_density_kg_l);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_sens1_enable", config.sensor.ch1_enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_sens1_name", config.sensor.ch1_name);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_sens1_description", config.sensor.ch1_description);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_sens1_gpio", config.sensor.ch1_gpio);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_sens2_enable", config.sensor.ch2_enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_sens2_name", config.sensor.ch2_name);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_sens2_description", config.sensor.ch2_description);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_sens2_gpio", config.sensor.ch2_gpio);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p12_language", config.lang);
  addJsonState(jsonSet, sizeof(jsonSet), "p10_log_enable", config.log.enable);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p10_log_mode", config.log.filter);
  addJsonValueInt(jsonSet, sizeof(jsonSet), "p10_log_order", config.log.order);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_ntp_enable", config.ntp.enable);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ntp_server", config.ntp.server);
  addJsonValueTxt(jsonSet, sizeof(jsonSet), "p12_ntp_tz", config.ntp.tz);
  addJsonState(jsonSet, sizeof(jsonSet), "p12_sim_enable", config.sim.enable);

  finalizeJsonBuffer(jsonSet, sizeof(jsonSet));
  // Serial.printf("settings string size: %d\n", strlen(jsonSet));
  updateWebJSON(jsonSet);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  // WiFi
  updateWebText("p09_wifi_ip", wifi.ipAddress, false);
  snprintf(tmpMessage, sizeof(tmpMessage), "%i %%", wifi.signal);
  updateWebText("p09_wifi_signal", tmpMessage, false);
  snprintf(tmpMessage, sizeof(tmpMessage), "%ld dbm", wifi.rssi);
  updateWebText("p09_wifi_rssi", tmpMessage, false);

  // Version informations
  updateWebText("p00_version", VERSION, false);
  updateWebText("p09_sw_version", VERSION, false);
  updateWebText("p00_dialog_version", VERSION, false);

  getBuildDateTime(tmpMessage);
  updateWebText("p09_sw_date", tmpMessage, false);

  // ESP informations
  updateWebTextFloat("p09_esp_flash_usage", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace(), false, 0);
  updateWebTextFloat("p09_esp_heap_usage", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize(), false, 0);
  updateWebTextFloat("p09_esp_maxallocheap", (float)ESP.getMaxAllocHeap() / 1000.0, false, 0);
  updateWebTextFloat("p09_esp_minfreeheap", (float)ESP.getMinFreeHeap() / 1000.0, false, 0);

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  updateWebText("p09_uptime", uptimeStr, false);
  char restartReason[64];
  getRestartReason(restartReason, sizeof(restartReason));
  updateWebText("p09_restart_reason", restartReason, false);

  // actual date and time
  updateWebText("p12_ntp_date", getDateStringWeb(), true);
  updateWebText("p12_ntp_time", getTimeString(), true);
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

  initJsonBuffer(jsonCfg, sizeof(jsonCfg));

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_frost_prot_th", pkmConfigNum->hc1_frost_protection_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_frost_prot_th_txt", pkmConfigNum->hc1_frost_protection_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_frost_th", pkmConfigStr->hc1_frost_protection_threshold);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_summer_th", pkmConfigNum->hc1_summer_mode_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_summer_th_txt", pkmConfigNum->hc1_summer_mode_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_summer_th", pkmConfigStr->hc1_summer_mode_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_night_temp", pkmConfigStr->hc1_night_temp);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_day_temp", pkmConfigStr->hc1_day_temp);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc1_opmode_night", pkmConfigNum->hc1_operation_mode == 0 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc1_opmode_day", pkmConfigNum->hc1_operation_mode == 1 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc1_opmode_auto", pkmConfigNum->hc1_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_op_mode", pkmConfigStr->hc1_operation_mode);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_hday_temp", pkmConfigStr->hc1_holiday_temp);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_max_temp", pkmConfigStr->hc1_max_temp);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_interp", pkmConfigNum->hc1_interpretation);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_interp_txt", pkmConfigNum->hc1_interpretation);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_interp", pkmConfigStr->hc1_interpretation);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_sw_on_temp", pkmConfigStr->hc1_switch_on_temperature);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_sw_off_th", pkmConfigNum->hc1_switch_off_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_sw_off_th_txt", pkmConfigNum->hc1_switch_off_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_sw_off_th", pkmConfigStr->hc1_switch_off_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_red_mode", pkmConfigStr->hc1_reduction_mode);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_reduct_mode", pkmConfigNum->hc1_reduction_mode);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_heatsys", pkmConfigStr->hc1_heating_system);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_temp_offset", pkmConfigStr->hc1_temp_offset);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_remotecontrol", pkmConfigStr->hc1_remotecontrol);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_prg", pkmConfigNum->hc1_program);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc1_holiday_days", pkmConfigNum->hc1_holiday_days);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t01", pkmConfigStr->hc1_timer01);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t02", pkmConfigStr->hc1_timer02);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t03", pkmConfigStr->hc1_timer03);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t04", pkmConfigStr->hc1_timer04);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t05", pkmConfigStr->hc1_timer05);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t06", pkmConfigStr->hc1_timer06);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t07", pkmConfigStr->hc1_timer07);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t08", pkmConfigStr->hc1_timer08);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t09", pkmConfigStr->hc1_timer09);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t10", pkmConfigStr->hc1_timer10);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t11", pkmConfigStr->hc1_timer11);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t12", pkmConfigStr->hc1_timer12);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t13", pkmConfigStr->hc1_timer13);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p03_hc1_t14", pkmConfigStr->hc1_timer14);
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_frost_prot_th", pkmConfigNum->hc2_frost_protection_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_frost_prot_th_txt", pkmConfigNum->hc2_frost_protection_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_frost_th", pkmConfigStr->hc2_frost_protection_threshold);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_summer_th", pkmConfigNum->hc2_summer_mode_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_summer_th_txt", pkmConfigNum->hc2_summer_mode_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_summer_th", pkmConfigStr->hc2_summer_mode_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_night_temp", pkmConfigStr->hc2_night_temp);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_day_temp", pkmConfigStr->hc2_day_temp);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc2_opmode_night", pkmConfigNum->hc2_operation_mode == 0 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc2_opmode_day", pkmConfigNum->hc2_operation_mode == 1 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_hc2_opmode_auto", pkmConfigNum->hc2_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_op_mode", pkmConfigStr->hc2_operation_mode);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_hday_temp", pkmConfigStr->hc2_holiday_temp);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_max_temp", pkmConfigStr->hc2_max_temp);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_interp", pkmConfigNum->hc2_interpretation);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_interp_txt", pkmConfigNum->hc2_interpretation);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_interp", pkmConfigStr->hc2_interpretation);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_sw_on_temp", pkmConfigStr->hc2_switch_on_temperature);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_sw_off_th", pkmConfigNum->hc2_switch_off_threshold);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_sw_off_th_txt", pkmConfigNum->hc2_switch_off_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_sw_off_th", pkmConfigStr->hc2_switch_off_threshold);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_red_mode", pkmConfigStr->hc2_reduction_mode);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_reduct_mode", pkmConfigNum->hc2_reduction_mode);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_heatsys", pkmConfigStr->hc2_heating_system);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_temp_offset", pkmConfigStr->hc2_temp_offset);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_remotecontrol", pkmConfigStr->hc2_remotecontrol);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_prg", pkmConfigNum->hc2_program);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_hc2_holiday_days", pkmConfigNum->hc2_holiday_days);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t01", pkmConfigStr->hc2_timer01);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t02", pkmConfigStr->hc2_timer02);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t03", pkmConfigStr->hc2_timer03);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t04", pkmConfigStr->hc2_timer04);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t05", pkmConfigStr->hc2_timer05);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t06", pkmConfigStr->hc2_timer06);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t07", pkmConfigStr->hc2_timer07);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t08", pkmConfigStr->hc2_timer08);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t09", pkmConfigStr->hc2_timer09);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t10", pkmConfigStr->hc2_timer10);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t11", pkmConfigStr->hc2_timer11);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t12", pkmConfigStr->hc2_timer12);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t13", pkmConfigStr->hc2_timer13);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p04_hc2_t14", pkmConfigStr->hc2_timer14);
  }

  // hot-water config values
  if (config.km271.use_ww) {
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p05_hw_prio", pkmConfigStr->ww_priority);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_ww_temp", pkmConfigNum->ww_temp);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_ww_temp_txt", pkmConfigNum->ww_temp);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p05_hw_temp", pkmConfigStr->ww_temp);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_ww_opmode_night", pkmConfigNum->ww_operation_mode == 0 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_ww_opmode_day", pkmConfigNum->ww_operation_mode == 1 ? true : false);
    addJsonState(jsonCfg, sizeof(jsonCfg), "p02_ww_opmode_auto", pkmConfigNum->ww_operation_mode == 2 ? true : false);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p05_hw_op_mode", pkmConfigStr->ww_operation_mode);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p05_hw_proccess", pkmConfigStr->ww_processing);
    addJsonValueInt(jsonCfg, sizeof(jsonCfg), "p02_ww_circ", pkmConfigNum->ww_circulation);
    addJsonLabelInt(jsonCfg, sizeof(jsonCfg), "p02_ww_circ_txt", pkmConfigNum->ww_circulation);
    addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p05_hw_circ", pkmConfigStr->ww_circulation);
  }

  // general config values
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_language", pkmConfigStr->language);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_display", pkmConfigStr->display);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_burner_type", pkmConfigStr->burner_type);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p06_max_boil_temp", pkmConfigStr->max_boiler_temperature);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_pump_logic_temp", pkmConfigStr->pump_logic_temp);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_exh_gas_temp_th", pkmConfigStr->exhaust_gas_temperature_threshold);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_burner_min_mod", pkmConfigStr->burner_min_modulation);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_burner_mod_run", pkmConfigStr->burner_modulation_runtime);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_building_type", pkmConfigStr->building_type);
  addJsonLabelTxt(jsonCfg, sizeof(jsonCfg), "p07_time_offset", pkmConfigStr->time_offset);

  finalizeJsonBuffer(jsonCfg, sizeof(jsonCfg));
  // Serial.printf("config string size: %d\n", strlen(jsonCfg));
  updateWebJSON(jsonCfg);
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
        updateWebText("p01_hc1_opmode", webText.AUTOMATIC[config.lang], false);
        updateWebSetIcon("p01_hc1_opmode_icon", "i_auto");
      } else { // MANUAL
        updateWebSetIcon("p01_hc1_opmode_icon", "i_manual");
        updateWebText("p01_hc1_opmode", bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.MAN_DAY[config.lang] : webText.MAN_NIGHT[config.lang],
                      false);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc1_summer_winter_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc1_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]),
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
      updateWebText("p01_hc1_day_night", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]),
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
      updateWebText("p01_hc1_pump", (kmStatusCpy.HC1_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false);
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
        updateWebText("p01_hc2_opmode", webText.AUTOMATIC[config.lang], false);
        updateWebSetIcon("p01_hc2_opmode_icon", "i_auto");
      } else { // MANUAL
        updateWebSetIcon("p01_hc2_opmode_icon", "i_manual");
        updateWebText("p01_hc2_opmode", bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.MAN_DAY[config.lang] : webText.MAN_NIGHT[config.lang],
                      false);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc2_summer_winter_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc2_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]),
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
      updateWebText("p01_hc2_day_night", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]),
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
      updateWebText("p01_hc2_pump", (kmStatusCpy.HC2_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false);
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
        snprintf(tmpMessage, sizeof(tmpMessage), "%s", webText.AUTOMATIC[config.lang]);
        updateWebSetIcon("p01_ww_opmode_icon", "i_auto");
      } else {                                                   // MANUAL
        if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
          updateWebSetIcon("p01_ww_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
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
    updateWebText("p01_burner", (kmStatusCpy.BurnerStates == 0) ? webText.OFF[config.lang] : webText.ON[config.lang], false);
    updateWebText("p06_burn_ctrl", cfgArrayTexts.BURNER_STATE[config.lang][kmStatusCpy.BurnerStates], false);
  } else if (kmStatusCpy.ExhaustTemp != pkmStatus->ExhaustTemp) {
    kmStatusCpy.ExhaustTemp = pkmStatus->ExhaustTemp;
    updateWebTextInt("p07_exh_gas_temp", kmStatusCpy.ExhaustTemp, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_2 != pkmStatus->BurnerOperatingDuration_2) {
    kmStatusCpy.BurnerOperatingDuration_2 = pkmStatus->BurnerOperatingDuration_2;
    updateWebTextInt("p06_burn_run_min65536", kmStatusCpy.BurnerOperatingDuration_2, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_1 != pkmStatus->BurnerOperatingDuration_1) {
    kmStatusCpy.BurnerOperatingDuration_1 = pkmStatus->BurnerOperatingDuration_1;
    updateWebTextInt("p06_burn_run_min256", kmStatusCpy.BurnerOperatingDuration_1, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_0 != pkmStatus->BurnerOperatingDuration_0) {
    kmStatusCpy.BurnerOperatingDuration_0 = pkmStatus->BurnerOperatingDuration_0;
    updateWebTextInt("p06_burn_run_min", kmStatusCpy.BurnerOperatingDuration_0, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_Sum != pkmStatus->BurnerOperatingDuration_Sum) {
    kmStatusCpy.BurnerOperatingDuration_Sum = pkmStatus->BurnerOperatingDuration_Sum;
    updateWebTextInt("p06_burn_run_all", kmStatusCpy.BurnerOperatingDuration_Sum, false);
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
    // TODO: check
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

  initJsonBuffer(jsonStat, sizeof(jsonStat));

  // heating circuit 1 values
  if (config.km271.use_hc1) {
    // AUTOMATIC / MANUAL (Day/Night)
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc1_opmode", webText.AUTOMATIC[config.lang]);
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc1_opmode_icon", "i_auto");
    } else { // MANUAL
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc1_opmode",
                      bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.MAN_DAY[config.lang] : webText.MAN_NIGHT[config.lang]);
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc1_opmode_icon", "i_manual");
    }

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]));
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc1_summer_winter_icon",
                  (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
    } else { // generate status from actual temperature and summer threshold
      addJsonLabelTxt(
          jsonStat, sizeof(jsonStat), "p01_hc1_summer_winter",
          (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]));
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc1_summer_winter_icon",
                  (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? "i_summer" : "i_winter"));
    }

    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_auto", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov1_frost", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)));

    // Day / Night
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc1_day_night",
                    (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]));
    addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc1_day_night_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? "i_day" : "i_night"));

    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_summer", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_day", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p03_hc1_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)));
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_room_set", kmStatusCpy.HC1_RoomTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_on_time_opt_dur", kmStatusCpy.HC1_SwitchOnOptimizationTime);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_off_time_opt_dur", kmStatusCpy.HC1_SwitchOffOptimizationTime);
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc1_pump",
                    (kmStatusCpy.HC1_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]));
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_pump", kmStatusCpy.HC1_PumpPower);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_mixer", kmStatusCpy.HC1_MixingValue);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10);
  } // END HC1

  // heating circuit 2 values
  if (config.km271.use_hc2) {
    // HC2-Operating State
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc2_opmode", webText.AUTOMATIC[config.lang]);
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc2_opmode_icon", "i_auto");
    } else { // MANUAL
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc2_opmode",
                      bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.MAN_DAY[config.lang] : webText.MAN_NIGHT[config.lang]);
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc2_opmode_icon", "i_manual");
    }

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]));
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc2_summer_winter_icon",
                  (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
    } else { // generate status from actual temperature and summer threshold
      addJsonLabelTxt(
          jsonStat, sizeof(jsonStat), "p01_hc2_summer_winter",
          (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]));
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc2_summer_winter_icon",
                  (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? "i_summer" : "i_winter"));
    }

    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_auto", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov1_frost", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)));
    // Day / Night
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc2_day_night",
                    (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]));
    addJsonIcon(jsonStat, sizeof(jsonStat), "p01_hc2_day_night_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? "i_day" : "i_night"));

    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_summer", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_day", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p04_hc2_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)));
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_room_set", kmStatusCpy.HC2_RoomTargetTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_on_time_opt_dur", kmStatusCpy.HC2_SwitchOnOptimizationTime);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_off_time_opt_dur", kmStatusCpy.HC2_SwitchOffOptimizationTime);
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_hc2_pump",
                    (kmStatusCpy.HC2_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]));
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_pump", kmStatusCpy.HC2_PumpPower);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_mixer", kmStatusCpy.HC2_MixingValue);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10);

  } // END HC2

  // hot-water values
  if (config.km271.use_ww) {
    // WW-Operating State
    if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) { // AUTOMATIC
      addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_ww_opmode", webText.AUTOMATIC[config.lang]);
      addJsonIcon(jsonStat, sizeof(jsonStat), "p01_ww_opmode_icon", "i_auto");
    } else {                                                   // MANUAL
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
        addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_ww_opmode", webText.MAN_DAY[config.lang]);
        addJsonIcon(jsonStat, sizeof(jsonStat), "p01_ww_opmode_icon", "i_manual");
      } else { // NIGHT
        addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_ww_opmode", webText.MAN_NIGHT[config.lang]);
        addJsonIcon(jsonStat, sizeof(jsonStat), "p01_ww_opmode_icon", "i_manual");
      }
    }

    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_auto", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_disinfect", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_holiday", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_err_disinfect", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_err_sensor", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_stays_cold", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov1_err_anode", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_load", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_manual", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_off_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_on_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_day", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_hot", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_ov2_prio", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)));
    snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterTargetTemp);
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_set_temp", tmpMessage);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp);
    snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterActualTemp);
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_act_temp", tmpMessage);
    addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp);
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_on_time_opt_dur", onOffString(kmStatusCpy.HotWaterOptimizationTime));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_pump_type_circ", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)));
    addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p05_hw_pump_type_gwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)));
  } // END HotWater

  // general values
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_boil_set", kmStatusCpy.BoilerForwardTargetTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_boil_temp", kmStatusCpy.BoilerForwardActualTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_boil_sw_on_temp", kmStatusCpy.BurnerSwitchOnTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_boil_sw_off_temp", kmStatusCpy.BurnerSwitchOffTemp);

  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_boiler_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_aux_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_boiler_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_exh_gas_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_exh_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_fail_ext", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)));

  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_state_exh_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_s_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_st_boiler_prot", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_s_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_s_perf_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_s_perf_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_boil_s_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)));
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p01_burner", (kmStatusCpy.BurnerStates == 0) ? webText.OFF[config.lang] : webText.ON[config.lang]);
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p06_burn_ctrl", cfgArrayTexts.BURNER_STATE[config.lang][kmStatusCpy.BurnerStates]);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p07_exh_gas_temp", kmStatusCpy.ExhaustTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_burn_run_min65536", kmStatusCpy.BurnerOperatingDuration_2);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_burn_run_min256", kmStatusCpy.BurnerOperatingDuration_1);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_burn_run_min", kmStatusCpy.BurnerOperatingDuration_0);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p06_burn_run_all", kmStatusCpy.BurnerOperatingDuration_Sum);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p07_out_temp", kmStatusCpy.OutsideTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_temp_out_act", kmStatusCpy.OutsideTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p07_out_temp_damped", kmStatusCpy.OutsideDampedTemp);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp);
  static char ControllerVersion[32];
  snprintf(ControllerVersion, sizeof(ControllerVersion), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
  addJsonLabelTxt(jsonStat, sizeof(jsonStat), "p09_logamatic_version", ControllerVersion);
  addJsonLabelInt(jsonStat, sizeof(jsonStat), "p09_logamatic_modul", kmStatusCpy.Modul);
  // TODO: check

  finalizeJsonBuffer(jsonStat, sizeof(jsonStat));
  // Serial.printf("status string size: %d\n", strlen(jsonStat));
  updateWebJSON(jsonStat);
}

void webUIupdates() {

  // refresh elemets not faster than XXms
  if (refreshTimer1.cycleTrigger(WEBUI_FAST_REFRESH_TIME_MS)) {

    if (webLogRefreshActive()) {
      webReadLogBufferCyclic();
    } else {
      switch (UpdateCntFast) {
      case 0:
        updateKm271AlarmElements();
        break;
      case 1:
        updateKm271StatusElements();
        break;
      case 2:
        updateKm271ConfigElements();
        break;
      default:
        UpdateCntFast = -1;
        break;
      }
      UpdateCntFast = (UpdateCntFast + 1) % 3;
    }
  }

  // refresh elemets every 1 seconds
  if (refreshTimer2.cycleTrigger(WEBUI_SLOW_REFRESH_TIME_MS)) {
    switch (UpdateCntSlow) {
    case 0:
      updateSystemInfoElements();
      break;
    case 1:
      updateOilmeterElements(false);
      break;
    case 2:
      updateSensorElements();
      break;
    default:
      UpdateCntSlow = -1;
      break;
    }
    UpdateCntSlow = (UpdateCntSlow + 1) % 3;
  }

  // refresh all elements on browser refresh
  if (refreshTimer3.cycleTrigger(WEBUI_JSON_REFRESH_TIME_MS) && refreshRequest) {
    switch (UpdateCntRefresh) {
    case 0:
      updateSettingsElements();
      break;
    case 1:
      updateKm271ConfigElementsAll();
      break;
    case 2:
      updateKm271StatusElementsAll();
      refreshRequest = false;
      break;
    default:
      UpdateCntRefresh = -1;
      break;
    }
    UpdateCntRefresh = (UpdateCntRefresh + 1) % 3;
  }
}