#include <basics.h>
#include <github.h>
#include <language.h>
#include <message.h>
#include <webUI.h>
#include <webUIupdates.h>

/* S E T T I N G S ****************************************************/
#define WEBUI_SLOW_REFRESH_TIME_MS 500
#define WEBUI_FAST_REFRESH_TIME_MS 100

/* P R O T O T Y P E S ********************************************************/
void updateOilmeterElements(bool forceUpdate);
void updateSensorElements(bool forceUpdate);
void updateSystemInfoElements();

/* D E C L A R A T I O N S ****************************************************/
static muTimer refreshTimerSingle = muTimer(); // timer to refresh other values
static muTimer refreshTimerAll = muTimer();    // timer to refresh other values
static muTimer otaProgessTimer = muTimer();    // timer to refresh other values

static s_km271_status kmStatusCpy;
static s_km271_status *pkmStatus = km271GetStatusValueAdr();
static s_km271_config_str *pkmConfigStr = km271GetConfigStringsAdr();
static s_km271_config_num *pkmConfigNum = km271GetConfigValueAdr();
static s_km271_alarm_str *pkmAlarmStr = km271GetAlarmMsgAdr();

static char tmpMessage[300] = {'\0'};
static unsigned int KmAlarmHash[4] = {0};
static bool refreshRequest = false;
static bool km271RefreshActiveOld = false;
static int UpdateCntSlow = 0;
static int UpdateCntRefresh = 0;
static unsigned long hashKmCfgNumOld, hashKmCfgStrOld;
static JsonDocument jsonDoc;
static JsonDocument kmCfgJsonDoc;
static const char *TAG = "WEB"; // LOG TAG
static auto &ota = EspSysUtil::OTA::getInstance();
static auto &wdt = EspSysUtil::Wdt::getInstance();
GithubRelease ghLatestRelease;
GithubReleaseInfo ghReleaseInfo;

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

/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAllElements() {

  refreshRequest = true;         // start combined json refresh
  km271RefreshActiveOld = false; // reset reminder for refreshBar

  // reset hash values to force updates
  memset((void *)KmAlarmHash, 0, sizeof(KmAlarmHash));

  updateOilmeterElements(true);
  updateSensorElements(true);
  webUI.wsUpdateWebLanguage(LANG::CODE[config.lang]);
  webUI.wsShowElementClass("simModeBar", config.sim.enable);

  if (setupMode) {
    webUI.wsShowElementClass("setupModeBar", true);
  }
}

/**
 * *******************************************************************
 * @brief   update Sensor informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSensorElements(bool forceUpdate) {

  if (config.sensor.ch1_enable) {

    static char ch1_name[32];
    if (strcmp(config.sensor.ch1_name, ch1_name) != 0 || forceUpdate) {
      webUI.wsUpdateWebText("p01_sens1_name", config.sensor.ch1_name, false);
      strncpy(ch1_name, config.sensor.ch1_name, sizeof(ch1_name));
    }
    static char ch1_desc[32];
    if (strcmp(config.sensor.ch1_description, ch1_desc) != 0 || forceUpdate) {
      webUI.wsUpdateWebText("p01_sens1_description", config.sensor.ch1_description, false);
      strncpy(ch1_desc, config.sensor.ch1_description, sizeof(ch1_desc));
    }
    static float ch1_value;
    if (ch1_value != sensor.ch1_temp || forceUpdate) {
      ch1_value = sensor.ch1_temp;
      if (sensor.ch1_temp == -127) {
        webUI.wsUpdateWebText("p01_sens1_value", "--", false);
      } else {
        webUI.wsUpdateWebText("p01_sens1_value", sensor.ch1_temp, false);
      }
    }
  }
  if (config.sensor.ch2_enable) {
    static char ch2_name[32];
    if (strcmp(config.sensor.ch2_name, ch2_name) != 0 || forceUpdate) {
      webUI.wsUpdateWebText("p01_sens2_name", config.sensor.ch2_name, false);
      strncpy(ch2_name, config.sensor.ch2_name, sizeof(ch2_name));
    }
    static char ch2_desc[32];
    if (strcmp(config.sensor.ch2_description, ch2_desc) != 0 || forceUpdate) {
      webUI.wsUpdateWebText("p01_sens2_description", config.sensor.ch2_description, false);
      strncpy(ch2_desc, config.sensor.ch2_description, sizeof(ch2_desc));
    }
    static float ch2_value;
    if (ch2_value != sensor.ch2_temp || forceUpdate) {
      if (sensor.ch2_temp == -127) {
        webUI.wsUpdateWebText("p01_sens2_value", "--", false);
      } else {
        webUI.wsUpdateWebText("p01_sens2_value", sensor.ch2_temp, false);
      }
      ch2_value = sensor.ch2_temp;
    }
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateOilmeterElements(bool forceUpdate) {

  static long oilcounter, oilcounterOld;
  static double oilcounterVirtOld = 0.0;

  if (config.oilmeter.use_hardware_meter) {
    oilcounter = getOilmeter();
    if (forceUpdate || oilcounter != oilcounterOld) {
      oilcounterOld = oilcounter;

      // Oilmeter value in controlTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f", float(oilcounter) / 100);
      webUI.wsUpdateWebText("p02_oilmeter_act", tmpMessage, false);
      snprintf(tmpMessage, sizeof(tmpMessage), "%lu", (oilcounter));
      webUI.wsUpdateWebText("p02_oilmeter_set", tmpMessage, true);

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(oilcounter) / 100);
      webUI.wsUpdateWebText("p01_hw_oilmeter", tmpMessage, false);
    }
  }
  if (config.oilmeter.use_virtual_meter) {
    if (forceUpdate || pkmStatus->BurnerCalcOilConsumption != oilcounterVirtOld) {
      oilcounterVirtOld = pkmStatus->BurnerCalcOilConsumption;

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(pkmStatus->BurnerCalcOilConsumption));
      webUI.wsUpdateWebText("p01_v_oilmeter", tmpMessage, false);
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

  webUI.initJsonBuffer(jsonDoc);

  webUI.addJson(jsonDoc, "cfg_gpio_km271_RX", config.gpio.km271_RX);
  webUI.addJson(jsonDoc, "cfg_gpio_km271_TX", config.gpio.km271_TX);
  webUI.addJson(jsonDoc, "cfg_gpio_led_heartbeat", config.gpio.led_heartbeat);
  webUI.addJson(jsonDoc, "cfg_gpio_led_logmode", config.gpio.led_logmode);
  webUI.addJson(jsonDoc, "cfg_gpio_led_wifi", config.gpio.led_wifi);
  webUI.addJson(jsonDoc, "cfg_gpio_led_oilcounter", config.gpio.led_oilcounter);
  webUI.addJson(jsonDoc, "cfg_gpio_trigger_oilcounter", config.gpio.trigger_oilcounter);

  webUI.wsUpdateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  webUI.initJsonBuffer(jsonDoc);

  // Network information
  webUI.addJson(jsonDoc, "p09_wifi_ip", wifi.ipAddress);
  snprintf(tmpMessage, sizeof(tmpMessage), "%i %%", wifi.signal);
  webUI.addJson(jsonDoc, "p09_wifi_signal", tmpMessage);
  snprintf(tmpMessage, sizeof(tmpMessage), "%ld dbm", wifi.rssi);
  webUI.addJson(jsonDoc, "p09_wifi_rssi", tmpMessage);

  if (!WiFi.isConnected()) {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_nok");
  } else if (wifi.rssi < -80) {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_1");
  } else if (wifi.rssi < -70) {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_2");
  } else if (wifi.rssi < -60) {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_3");
  } else {
    webUI.addJson(jsonDoc, "p00_wifi_icon", "i_wifi_4");
  }

  webUI.addJson(jsonDoc, "p09_eth_ip", strlen(eth.ipAddress) ? eth.ipAddress : "-.-.-.-");
  webUI.addJson(jsonDoc, "p09_eth_status", eth.connected ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (config.eth.enable) {
    if (eth.connected) {
      webUI.addJson(jsonDoc, "p00_eth_icon", "i_eth_ok");
      snprintf(tmpMessage, sizeof(tmpMessage), "%d Mbps", eth.linkSpeed);
      webUI.addJson(jsonDoc, "p09_eth_link_speed", tmpMessage);
      webUI.addJson(jsonDoc, "p09_eth_full_duplex", eth.fullDuplex ? WEB_TXT::FULL_DUPLEX[config.lang] : "---");

    } else {
      webUI.addJson(jsonDoc, "p00_eth_icon", "i_eth_nok");
      webUI.addJson(jsonDoc, "p09_eth_link_speed", "---");
      webUI.addJson(jsonDoc, "p09_eth_full_duplex", "---");
    }
  } else {
    webUI.addJson(jsonDoc, "p00_eth_icon", "");
    webUI.addJson(jsonDoc, "p09_eth_link_speed", "---");
    webUI.addJson(jsonDoc, "p09_eth_full_duplex", "---");
  }

  // MQTT Status
  webUI.addJson(jsonDoc, "p09_mqtt_status", config.mqtt.enable ? WEB_TXT::ACTIVE[config.lang] : WEB_TXT::INACTIVE[config.lang]);
  webUI.addJson(jsonDoc, "p09_mqtt_connection", mqttIsConnected() ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);

  if (mqttGetLastError() != nullptr) {
    webUI.addJson(jsonDoc, "p09_mqtt_last_err", mqttGetLastError());
  } else {
    webUI.addJson(jsonDoc, "p09_mqtt_last_err", "---");
  }

  // ESP informations
  webUI.addJson(jsonDoc, "p09_esp_flash_usage", ESP.getSketchSize() * 100.0f / ESP.getFreeSketchSpace());
  webUI.addJson(jsonDoc, "p09_esp_heap_usage", (ESP.getHeapSize() - ESP.getFreeHeap()) * 100.0f / ESP.getHeapSize());
  webUI.addJson(jsonDoc, "p09_esp_maxallocheap", ESP.getMaxAllocHeap() / 1000.0f);
  webUI.addJson(jsonDoc, "p09_esp_minfreeheap", ESP.getMinFreeHeap() / 1000.0f);

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  webUI.addJson(jsonDoc, "p09_uptime", uptimeStr);

  // actual date and time
  webUI.addJson(jsonDoc, "p12_ntp_date", EspStrUtil::getDateStringWeb());
  webUI.addJson(jsonDoc, "p12_ntp_time", EspStrUtil::getTimeString());

  // KM271 Status
  webUI.addJson(jsonDoc, "p09_km271_logmode", km271GetLogMode() ? WEB_TXT::CONNECTED[config.lang] : WEB_TXT::NOT_CONNECTED[config.lang]);
  webUI.addJson(jsonDoc, "p09_km271_rx", EspStrUtil::formatBytes(km271GetRxBytes()));
  webUI.addJson(jsonDoc, "p09_km271_tx", EspStrUtil::formatBytes(km271GetTxBytes()));

  webUI.wsUpdateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElementsStatic() {

  webUI.initJsonBuffer(jsonDoc);

  // Version informations
  webUI.addJson(jsonDoc, "p00_version", VERSION);
  webUI.addJson(jsonDoc, "p09_sw_version", VERSION);
  webUI.addJson(jsonDoc, "p00_dialog_version", VERSION);

  webUI.addJson(jsonDoc, "p09_sw_date", EspStrUtil::getDateStringWeb());

  // restart reason
  webUI.addJson(jsonDoc, "p09_restart_reason", EspSysUtil::RestartReason::get());

  webUI.wsUpdateWebJSON(jsonDoc);
}

/**
 * *******************************************************************
 * @brief   update Alarm messages of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271AlarmElements() {

  if (EspStrUtil::strDiff(&KmAlarmHash[0], pkmAlarmStr->alarm1)) {
    webUI.wsUpdateWebText("p08_err_msg1", pkmAlarmStr->alarm1, false);
  }
  if (EspStrUtil::strDiff(&KmAlarmHash[1], pkmAlarmStr->alarm2)) {
    webUI.wsUpdateWebText("p08_err_msg2", pkmAlarmStr->alarm2, false);
  }
  if (EspStrUtil::strDiff(&KmAlarmHash[2], pkmAlarmStr->alarm3)) {
    webUI.wsUpdateWebText("p08_err_msg3", pkmAlarmStr->alarm3, false);
  }
  if (EspStrUtil::strDiff(&KmAlarmHash[3], pkmAlarmStr->alarm4)) {
    webUI.wsUpdateWebText("p08_err_msg4", pkmAlarmStr->alarm4, false);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void generateKm271ConfigJSON() {

  webUI.initJsonBuffer(kmCfgJsonDoc);

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_frost_prot_th", pkmConfigNum->hc1_frost_protection_threshold);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_frost_th", pkmConfigStr->hc1_frost_protection_threshold);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_summer_th", pkmConfigNum->hc1_summer_mode_threshold);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_summer_th", pkmConfigStr->hc1_summer_mode_threshold);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_night_temp", pkmConfigStr->hc1_night_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_night_temp", pkmConfigNum->hc1_night_temp);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_day_temp", pkmConfigStr->hc1_day_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_day_temp", pkmConfigNum->hc1_day_temp);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_hday_temp", pkmConfigStr->hc1_holiday_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_holiday_temp", pkmConfigNum->hc1_holiday_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_opmode_night", pkmConfigNum->hc1_operation_mode == 0 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_opmode_day", pkmConfigNum->hc1_operation_mode == 1 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_opmode_auto", pkmConfigNum->hc1_operation_mode == 2 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_op_mode", pkmConfigStr->hc1_operation_mode);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_max_temp", pkmConfigStr->hc1_max_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_interp", pkmConfigNum->hc1_interpretation);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_interp", pkmConfigStr->hc1_interpretation);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_sw_on_temp", pkmConfigStr->hc1_switch_on_temperature);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_sw_on_temp", pkmConfigNum->hc1_switch_on_temperature);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_sw_off_th", pkmConfigNum->hc1_switch_off_threshold);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_sw_off_th", pkmConfigStr->hc1_switch_off_threshold);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_red_mode", pkmConfigStr->hc1_reduction_mode);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_reduct_mode", pkmConfigNum->hc1_reduction_mode);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_heatsys", pkmConfigStr->hc1_heating_system);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_temp_offset", pkmConfigStr->hc1_temp_offset);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_remotecontrol", pkmConfigStr->hc1_remotecontrol);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_prg", pkmConfigNum->hc1_program);
    webUI.addJson(kmCfgJsonDoc, "p02_hc1_holiday_days", pkmConfigNum->hc1_holiday_days);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_act_prg", KM_CFG_ARRAY::HC_PROGRAM[config.lang][pkmConfigNum->hc1_program]);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t01", pkmConfigStr->hc1_timer01);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t02", pkmConfigStr->hc1_timer02);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t03", pkmConfigStr->hc1_timer03);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t04", pkmConfigStr->hc1_timer04);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t05", pkmConfigStr->hc1_timer05);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t06", pkmConfigStr->hc1_timer06);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t07", pkmConfigStr->hc1_timer07);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t08", pkmConfigStr->hc1_timer08);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t09", pkmConfigStr->hc1_timer09);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t10", pkmConfigStr->hc1_timer10);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t11", pkmConfigStr->hc1_timer11);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t12", pkmConfigStr->hc1_timer12);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t13", pkmConfigStr->hc1_timer13);
    webUI.addJson(kmCfgJsonDoc, "p03_hc1_t14", pkmConfigStr->hc1_timer14);
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_frost_prot_th", pkmConfigNum->hc2_frost_protection_threshold);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_frost_th", pkmConfigStr->hc2_frost_protection_threshold);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_summer_th", pkmConfigNum->hc2_summer_mode_threshold);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_summer_th", pkmConfigStr->hc2_summer_mode_threshold);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_night_temp", pkmConfigStr->hc2_night_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_night_temp", pkmConfigNum->hc2_night_temp);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_day_temp", pkmConfigStr->hc2_day_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_day_temp", pkmConfigNum->hc2_day_temp);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_hday_temp", pkmConfigStr->hc2_holiday_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_holiday_temp", pkmConfigNum->hc2_holiday_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_opmode_night", pkmConfigNum->hc2_operation_mode == 0 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_opmode_day", pkmConfigNum->hc2_operation_mode == 1 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_opmode_auto", pkmConfigNum->hc2_operation_mode == 2 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_op_mode", pkmConfigStr->hc2_operation_mode);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_max_temp", pkmConfigStr->hc2_max_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_interp", pkmConfigNum->hc2_interpretation);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_interp", pkmConfigStr->hc2_interpretation);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_sw_on_temp", pkmConfigStr->hc2_switch_on_temperature);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_sw_on_temp", pkmConfigNum->hc2_switch_on_temperature);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_sw_off_th", pkmConfigNum->hc2_switch_off_threshold);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_sw_off_th", pkmConfigStr->hc2_switch_off_threshold);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_red_mode", pkmConfigStr->hc2_reduction_mode);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_reduct_mode", pkmConfigNum->hc2_reduction_mode);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_heatsys", pkmConfigStr->hc2_heating_system);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_temp_offset", pkmConfigStr->hc2_temp_offset);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_remotecontrol", pkmConfigStr->hc2_remotecontrol);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_prg", pkmConfigNum->hc2_program);
    webUI.addJson(kmCfgJsonDoc, "p02_hc2_holiday_days", pkmConfigNum->hc2_holiday_days);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_act_prg", KM_CFG_ARRAY::HC_PROGRAM[config.lang][pkmConfigNum->hc2_program]);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t01", pkmConfigStr->hc2_timer01);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t02", pkmConfigStr->hc2_timer02);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t03", pkmConfigStr->hc2_timer03);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t04", pkmConfigStr->hc2_timer04);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t05", pkmConfigStr->hc2_timer05);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t06", pkmConfigStr->hc2_timer06);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t07", pkmConfigStr->hc2_timer07);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t08", pkmConfigStr->hc2_timer08);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t09", pkmConfigStr->hc2_timer09);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t10", pkmConfigStr->hc2_timer10);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t11", pkmConfigStr->hc2_timer11);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t12", pkmConfigStr->hc2_timer12);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t13", pkmConfigStr->hc2_timer13);
    webUI.addJson(kmCfgJsonDoc, "p04_hc2_t14", pkmConfigStr->hc2_timer14);
  }

  // hot-water config values
  if (config.km271.use_ww) {
    webUI.addJson(kmCfgJsonDoc, "p05_hw_prio", pkmConfigStr->ww_priority);
    webUI.addJson(kmCfgJsonDoc, "p02_ww_temp", pkmConfigNum->ww_temp);
    webUI.addJson(kmCfgJsonDoc, "p05_hw_temp", pkmConfigStr->ww_temp);
    webUI.addJson(kmCfgJsonDoc, "p02_ww_opmode_night", pkmConfigNum->ww_operation_mode == 0 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_ww_opmode_day", pkmConfigNum->ww_operation_mode == 1 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p02_ww_opmode_auto", pkmConfigNum->ww_operation_mode == 2 ? true : false);
    webUI.addJson(kmCfgJsonDoc, "p05_hw_op_mode", pkmConfigStr->ww_operation_mode);
    webUI.addJson(kmCfgJsonDoc, "p05_hw_proccess", pkmConfigStr->ww_processing);
    webUI.addJson(kmCfgJsonDoc, "p02_ww_circ", pkmConfigNum->ww_circulation);
    webUI.addJson(kmCfgJsonDoc, "p05_hw_circ", pkmConfigStr->ww_circulation);
  }

  // general config values
  webUI.addJson(kmCfgJsonDoc, "p07_language", pkmConfigStr->language);
  webUI.addJson(kmCfgJsonDoc, "p07_display", pkmConfigStr->display);
  webUI.addJson(kmCfgJsonDoc, "p07_burner_type", pkmConfigStr->burner_type);
  webUI.addJson(kmCfgJsonDoc, "p06_max_boil_temp", pkmConfigStr->max_boiler_temperature);
  webUI.addJson(kmCfgJsonDoc, "p07_pump_logic_temp", pkmConfigStr->pump_logic_temp);
  webUI.addJson(kmCfgJsonDoc, "p07_exh_gas_temp_th", pkmConfigStr->exhaust_gas_temperature_threshold);
  webUI.addJson(kmCfgJsonDoc, "p07_burner_min_mod", pkmConfigStr->burner_min_modulation);
  webUI.addJson(kmCfgJsonDoc, "p07_burner_mod_run", pkmConfigStr->burner_modulation_runtime);
  webUI.addJson(kmCfgJsonDoc, "p07_building_type", pkmConfigStr->building_type);
  webUI.addJson(kmCfgJsonDoc, "p07_time_offset", pkmConfigStr->time_offset);

  // solar config values
  if (config.km271.use_solar) {

    if (pkmConfigNum->solar_operation_mode == 2) {
      snprintf(tmpMessage, sizeof(tmpMessage), "%s", WEB_TXT::AUTOMATIC[config.lang]);
      webUI.addJson(kmCfgJsonDoc, "p01_solar_opmode_icon", "i_auto");
    } else if (pkmConfigNum->solar_operation_mode == 1) {
      snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::DAY[config.lang]);
      webUI.addJson(kmCfgJsonDoc, "p01_solar_opmode_icon", "i_manual");
    } else {
      snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::NIGHT[config.lang]);
      webUI.addJson(kmCfgJsonDoc, "p01_solar_opmode_icon", "i_manual");
    }
    webUI.addJson(kmCfgJsonDoc, "p01_solar_opmode", tmpMessage);

    webUI.addJson(kmCfgJsonDoc, "p13_solar_enable", pkmConfigStr->solar_activation);
    webUI.addJson(kmCfgJsonDoc, "p13_solar_opmode", pkmConfigStr->solar_operation_mode);
    webUI.addJson(kmCfgJsonDoc, "p13_solar_min", pkmConfigStr->solar_min);
    webUI.addJson(kmCfgJsonDoc, "p13_solar_max", pkmConfigStr->solar_max);
  }
}

/**
 * *******************************************************************
 * @brief   check and send if config vales has changed
 * @param   forceUpdate
 * @return  none
 * *******************************************************************/
void updateKm271ConfigElements(bool forceUpdate) {

  unsigned long hashKmCfgStrNew = EspStrUtil::hash(pkmConfigStr, sizeof(s_km271_config_str));
  unsigned long hashKmCfgNumNew = EspStrUtil::hash(pkmConfigNum, sizeof(s_km271_config_num));
  if ((hashKmCfgStrNew != hashKmCfgStrOld) || (hashKmCfgNumNew != hashKmCfgNumOld)) {
    hashKmCfgStrOld = hashKmCfgStrNew;
    hashKmCfgNumOld = hashKmCfgNumNew;
    generateKm271ConfigJSON();
    webUI.wsUpdateWebJSON(kmCfgJsonDoc);
  } else if (forceUpdate) {
    webUI.wsUpdateWebJSON(kmCfgJsonDoc);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271StatusElements(bool forceUpdate) {

  webUI.initJsonBuffer(jsonDoc);

  // heating circuit 1 values
  if (config.km271.use_hc1) {
    if (forceUpdate || (kmStatusCpy.HC1_OperatingStates_1 != pkmStatus->HC1_OperatingStates_1) ||
        (kmStatusCpy.HC1_OperatingStates_2 != pkmStatus->HC1_OperatingStates_2)) {
      kmStatusCpy.HC1_OperatingStates_1 = pkmStatus->HC1_OperatingStates_1;
      kmStatusCpy.HC1_OperatingStates_2 = pkmStatus->HC1_OperatingStates_2;

      // AUTOMATIC / MANUAL (Day/Night)
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        webUI.addJson(jsonDoc, "p01_hc1_opmode", WEB_TXT::AUTOMATIC[config.lang]);
        webUI.addJson(jsonDoc, "p01_hc1_opmode_icon", "i_auto");
      } else { // MANUAL
        webUI.addJson(jsonDoc, "p01_hc1_opmode_icon", "i_manual");
        webUI.addJson(jsonDoc, "p01_hc1_opmode",
                      bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang]);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        webUI.addJson(jsonDoc, "p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
        webUI.addJson(jsonDoc, "p01_hc1_summer_winter_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        webUI.addJson(
            jsonDoc, "p01_hc1_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
        webUI.addJson(jsonDoc, "p01_hc1_summer_winter_icon",
                      (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? "i_summer" : "i_winter"));
      }
      webUI.addJson(jsonDoc, "p03_hc1_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_auto", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)));
      webUI.addJson(jsonDoc, "p03_hc1_ov1_frost", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)));

      // Day / Night
      webUI.addJson(jsonDoc, "p01_hc1_day_night",
                    (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]));
      webUI.addJson(jsonDoc, "p01_hc1_day_night_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? "i_day" : "i_night"));

      webUI.addJson(jsonDoc, "p03_hc1_ov2_summer", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_day", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)));
      webUI.addJson(jsonDoc, "p03_hc1_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)));
    }
    if (forceUpdate || kmStatusCpy.HC1_HeatingForwardTargetTemp != pkmStatus->HC1_HeatingForwardTargetTemp) {
      kmStatusCpy.HC1_HeatingForwardTargetTemp = pkmStatus->HC1_HeatingForwardTargetTemp;
      webUI.addJson(jsonDoc, "p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
      webUI.addJson(jsonDoc, "p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp);
    }
    if (forceUpdate || (kmStatusCpy.HC1_HeatingForwardActualTemp != pkmStatus->HC1_HeatingForwardActualTemp)) {
      kmStatusCpy.HC1_HeatingForwardActualTemp = pkmStatus->HC1_HeatingForwardActualTemp;
      webUI.addJson(jsonDoc, "p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardActualTemp);
      webUI.addJson(jsonDoc, "p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardActualTemp);
    }
    if (forceUpdate || kmStatusCpy.HC1_RoomTargetTemp != pkmStatus->HC1_RoomTargetTemp) {
      kmStatusCpy.HC1_RoomTargetTemp = pkmStatus->HC1_RoomTargetTemp;
      webUI.addJson(jsonDoc, "p03_hc1_room_set", kmStatusCpy.HC1_RoomTargetTemp);
    }
    if (forceUpdate || kmStatusCpy.HC1_RoomActualTemp != pkmStatus->HC1_RoomActualTemp) {
      kmStatusCpy.HC1_RoomActualTemp = pkmStatus->HC1_RoomActualTemp;
      webUI.addJson(jsonDoc, "p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp);
    }
    if (forceUpdate || kmStatusCpy.HC1_SwitchOnOptimizationTime != pkmStatus->HC1_SwitchOnOptimizationTime) {
      kmStatusCpy.HC1_SwitchOnOptimizationTime = pkmStatus->HC1_SwitchOnOptimizationTime;
      webUI.addJson(jsonDoc, "p03_hc1_on_time_opt_dur", kmStatusCpy.HC1_SwitchOnOptimizationTime);
    }
    if (forceUpdate || kmStatusCpy.HC1_SwitchOffOptimizationTime != pkmStatus->HC1_SwitchOffOptimizationTime) {
      kmStatusCpy.HC1_SwitchOffOptimizationTime = pkmStatus->HC1_SwitchOffOptimizationTime;
      webUI.addJson(jsonDoc, "p03_hc1_off_time_opt_dur", kmStatusCpy.HC1_SwitchOffOptimizationTime);
    }
    if (forceUpdate || kmStatusCpy.HC1_PumpPower != pkmStatus->HC1_PumpPower) {
      kmStatusCpy.HC1_PumpPower = pkmStatus->HC1_PumpPower;
      webUI.addJson(jsonDoc, "p01_hc1_pump", (kmStatusCpy.HC1_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]));
      webUI.addJson(jsonDoc, "p03_hc1_pump", kmStatusCpy.HC1_PumpPower);
    }
    if (forceUpdate || kmStatusCpy.HC1_MixingValue != pkmStatus->HC1_MixingValue) {
      kmStatusCpy.HC1_MixingValue = pkmStatus->HC1_MixingValue;
      webUI.addJson(jsonDoc, "p03_hc1_mixer", kmStatusCpy.HC1_MixingValue);
    }
    if (forceUpdate || kmStatusCpy.HC1_HeatingCurvePlus10 != pkmStatus->HC1_HeatingCurvePlus10) {
      kmStatusCpy.HC1_HeatingCurvePlus10 = pkmStatus->HC1_HeatingCurvePlus10;
      webUI.addJson(jsonDoc, "p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10);
    }
    if (forceUpdate || kmStatusCpy.HC1_HeatingCurve0 != pkmStatus->HC1_HeatingCurve0) {
      kmStatusCpy.HC1_HeatingCurve0 = pkmStatus->HC1_HeatingCurve0;
      webUI.addJson(jsonDoc, "p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0);
    }
    if (forceUpdate || kmStatusCpy.HC1_HeatingCurveMinus10 != pkmStatus->HC1_HeatingCurveMinus10) {
      kmStatusCpy.HC1_HeatingCurveMinus10 = pkmStatus->HC1_HeatingCurveMinus10;
      webUI.addJson(jsonDoc, "p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10);
    }
  } // END HC1

  // heating circuit 2 values
  if (config.km271.use_hc2) {
    if (forceUpdate || (kmStatusCpy.HC2_OperatingStates_1 != pkmStatus->HC2_OperatingStates_1) ||
        (kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2)) {
      kmStatusCpy.HC2_OperatingStates_1 = pkmStatus->HC2_OperatingStates_1;
      kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2;

      // AUTOMATIC / MANUAL (Day/Night)
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        webUI.addJson(jsonDoc, "p01_hc2_opmode", WEB_TXT::AUTOMATIC[config.lang]);
        webUI.addJson(jsonDoc, "p01_hc2_opmode_icon", "i_auto");
      } else { // MANUAL
        webUI.addJson(jsonDoc, "p01_hc2_opmode_icon", "i_manual");
        webUI.addJson(jsonDoc, "p01_hc2_opmode",
                      bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::MAN_DAY[config.lang] : WEB_TXT::MAN_NIGHT[config.lang]);
      }

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        webUI.addJson(jsonDoc, "p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
        webUI.addJson(jsonDoc, "p01_hc2_summer_winter_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        webUI.addJson(
            jsonDoc, "p01_hc2_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? WEB_TXT::SUMMER[config.lang] : WEB_TXT::WINTER[config.lang]));
        webUI.addJson(jsonDoc, "p01_hc2_summer_winter_icon",
                      (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? "i_summer" : "i_winter"));
      }

      webUI.addJson(jsonDoc, "p04_hc2_ov1_off_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_on_time_opt", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_auto", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)));
      webUI.addJson(jsonDoc, "p04_hc2_ov1_frost", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)));

      // Day / Night
      webUI.addJson(jsonDoc, "p01_hc2_day_night",
                    (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? WEB_TXT::DAY[config.lang] : WEB_TXT::NIGHT[config.lang]));
      webUI.addJson(jsonDoc, "p01_hc2_day_night_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? "i_day" : "i_night"));

      webUI.addJson(jsonDoc, "p04_hc2_ov2_summer", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_day", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_no_con_remote", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_remote_err", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_fail_flow_sens", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_flow_at_max", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)));
      webUI.addJson(jsonDoc, "p04_hc2_ov2_ext_sign_in", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)));
    }
    if (forceUpdate || kmStatusCpy.HC2_HeatingForwardTargetTemp != pkmStatus->HC2_HeatingForwardTargetTemp) {
      kmStatusCpy.HC2_HeatingForwardTargetTemp = pkmStatus->HC2_HeatingForwardTargetTemp;
      webUI.addJson(jsonDoc, "p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
      webUI.addJson(jsonDoc, "p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp);
    }
    if (forceUpdate || kmStatusCpy.HC2_HeatingForwardActualTemp != pkmStatus->HC2_HeatingForwardActualTemp) {
      kmStatusCpy.HC2_HeatingForwardActualTemp = pkmStatus->HC2_HeatingForwardActualTemp;
      webUI.addJson(jsonDoc, "p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
      webUI.addJson(jsonDoc, "p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp);
    }
    if (forceUpdate || kmStatusCpy.HC2_RoomTargetTemp != pkmStatus->HC2_RoomTargetTemp) {
      kmStatusCpy.HC2_RoomTargetTemp = pkmStatus->HC2_RoomTargetTemp;
      webUI.addJson(jsonDoc, "p04_hc2_room_set", kmStatusCpy.HC2_RoomTargetTemp);
    }
    if (forceUpdate || kmStatusCpy.HC2_RoomActualTemp != pkmStatus->HC2_RoomActualTemp) {
      kmStatusCpy.HC2_RoomActualTemp = pkmStatus->HC2_RoomActualTemp;
      webUI.addJson(jsonDoc, "p04_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp);
    }
    if (forceUpdate || kmStatusCpy.HC2_SwitchOnOptimizationTime != pkmStatus->HC2_SwitchOnOptimizationTime) {
      kmStatusCpy.HC2_SwitchOnOptimizationTime = pkmStatus->HC2_SwitchOnOptimizationTime;
      webUI.addJson(jsonDoc, "p04_hc2_on_time_opt_dur", kmStatusCpy.HC2_SwitchOnOptimizationTime);
    }
    if (forceUpdate || kmStatusCpy.HC2_SwitchOffOptimizationTime != pkmStatus->HC2_SwitchOffOptimizationTime) {
      kmStatusCpy.HC2_SwitchOffOptimizationTime = pkmStatus->HC2_SwitchOffOptimizationTime;
      webUI.addJson(jsonDoc, "p04_hc2_off_time_opt_dur", kmStatusCpy.HC2_SwitchOffOptimizationTime);
    }
    if (forceUpdate || kmStatusCpy.HC2_PumpPower != pkmStatus->HC2_PumpPower) {
      kmStatusCpy.HC2_PumpPower = pkmStatus->HC2_PumpPower;
      webUI.addJson(jsonDoc, "p01_hc2_pump", (kmStatusCpy.HC2_PumpPower == 0 ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]));
      webUI.addJson(jsonDoc, "p04_hc2_pump", kmStatusCpy.HC2_PumpPower);
    }
    if (forceUpdate || kmStatusCpy.HC2_MixingValue != pkmStatus->HC2_MixingValue) {
      kmStatusCpy.HC2_MixingValue = pkmStatus->HC2_MixingValue;
      webUI.addJson(jsonDoc, "p04_hc2_mixer", kmStatusCpy.HC2_MixingValue);
    }
    if (forceUpdate || kmStatusCpy.HC2_HeatingCurvePlus10 != pkmStatus->HC2_HeatingCurvePlus10) {
      kmStatusCpy.HC2_HeatingCurvePlus10 = pkmStatus->HC2_HeatingCurvePlus10;
      webUI.addJson(jsonDoc, "p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10);
    }
    if (forceUpdate || kmStatusCpy.HC2_HeatingCurve0 != pkmStatus->HC2_HeatingCurve0) {
      kmStatusCpy.HC2_HeatingCurve0 = pkmStatus->HC2_HeatingCurve0;
      webUI.addJson(jsonDoc, "p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0);
    }
    if (forceUpdate || kmStatusCpy.HC2_HeatingCurveMinus10 != pkmStatus->HC2_HeatingCurveMinus10) {
      kmStatusCpy.HC2_HeatingCurveMinus10 = pkmStatus->HC2_HeatingCurveMinus10;
      webUI.addJson(jsonDoc, "p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10);
    }
  } // END HC2

  // hot-water values
  if (config.km271.use_ww) {
    if (forceUpdate || kmStatusCpy.HotWaterOperatingStates_1 != pkmStatus->HotWaterOperatingStates_1) {
      kmStatusCpy.HotWaterOperatingStates_1 = pkmStatus->HotWaterOperatingStates_1;
      // WW-Operating State
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) { // AUTOMATIC
        snprintf(tmpMessage, sizeof(tmpMessage), "%s", WEB_TXT::AUTOMATIC[config.lang]);
        webUI.addJson(jsonDoc, "p01_ww_opmode_icon", "i_auto");
      } else {                                                   // MANUAL
        if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::DAY[config.lang]);
          webUI.addJson(jsonDoc, "p01_ww_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", WEB_TXT::MANUAL[config.lang], WEB_TXT::NIGHT[config.lang]);
          webUI.addJson(jsonDoc, "p01_ww_opmode_icon", "i_manual");
        }
      }
      webUI.addJson(jsonDoc, "p01_ww_opmode", tmpMessage);

      webUI.addJson(jsonDoc, "p05_hw_ov1_auto", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_disinfect", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_holiday", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_err_disinfect", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_err_sensor", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_stays_cold", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)));
      webUI.addJson(jsonDoc, "p05_hw_ov1_err_anode", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)));
    }
    if (forceUpdate || kmStatusCpy.HotWaterOperatingStates_2 != pkmStatus->HotWaterOperatingStates_2) {
      kmStatusCpy.HotWaterOperatingStates_2 = pkmStatus->HotWaterOperatingStates_2;
      webUI.addJson(jsonDoc, "p05_hw_ov2_load", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_manual", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_off_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_on_time_opt_dur", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_day", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_hot", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)));
      webUI.addJson(jsonDoc, "p05_hw_ov2_prio", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)));
    }
    if (forceUpdate || (kmStatusCpy.HotWaterTargetTemp != pkmStatus->HotWaterTargetTemp)) {
      kmStatusCpy.HotWaterTargetTemp = pkmStatus->HotWaterTargetTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterTargetTemp);
      webUI.addJson(jsonDoc, "p05_hw_set_temp", tmpMessage);
      webUI.addJson(jsonDoc, "p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp);
    }
    if (forceUpdate || (kmStatusCpy.HotWaterActualTemp != pkmStatus->HotWaterActualTemp)) {
      kmStatusCpy.HotWaterActualTemp = pkmStatus->HotWaterActualTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterActualTemp);
      webUI.addJson(jsonDoc, "p05_hw_act_temp", tmpMessage);
      webUI.addJson(jsonDoc, "p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp);
    }
    if (forceUpdate || kmStatusCpy.HotWaterOptimizationTime != pkmStatus->HotWaterOptimizationTime) {
      kmStatusCpy.HotWaterOptimizationTime = pkmStatus->HotWaterOptimizationTime;
      webUI.addJson(jsonDoc, "p05_hw_on_time_opt_dur", onOffString(kmStatusCpy.HotWaterOptimizationTime));
    }
    if (forceUpdate || kmStatusCpy.HotWaterPumpStates != pkmStatus->HotWaterPumpStates) {
      kmStatusCpy.HotWaterPumpStates = pkmStatus->HotWaterPumpStates;
      webUI.addJson(jsonDoc, "p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)));
      webUI.addJson(jsonDoc, "p05_hw_pump_type_circ", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)));
      webUI.addJson(jsonDoc, "p05_hw_pump_type_gwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)));
    }
  } // END HotWater

  // general values
  if (forceUpdate || kmStatusCpy.BoilerForwardTargetTemp != pkmStatus->BoilerForwardTargetTemp) {
    kmStatusCpy.BoilerForwardTargetTemp = pkmStatus->BoilerForwardTargetTemp;
    webUI.addJson(jsonDoc, "p06_boil_set", kmStatusCpy.BoilerForwardTargetTemp);
    webUI.addJson(jsonDoc, "p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp);
  }
  if (forceUpdate || kmStatusCpy.BoilerForwardActualTemp != pkmStatus->BoilerForwardActualTemp) {
    kmStatusCpy.BoilerForwardActualTemp = pkmStatus->BoilerForwardActualTemp;
    webUI.addJson(jsonDoc, "p06_boil_temp", kmStatusCpy.BoilerForwardActualTemp);
    webUI.addJson(jsonDoc, "p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp);
  }
  if (forceUpdate || kmStatusCpy.BurnerSwitchOnTemp != pkmStatus->BurnerSwitchOnTemp) {
    kmStatusCpy.BurnerSwitchOnTemp = pkmStatus->BurnerSwitchOnTemp;
    webUI.addJson(jsonDoc, "p06_boil_sw_on_temp", kmStatusCpy.BurnerSwitchOnTemp);
  }
  if (forceUpdate || kmStatusCpy.BurnerSwitchOffTemp != pkmStatus->BurnerSwitchOffTemp) {
    kmStatusCpy.BurnerSwitchOffTemp = pkmStatus->BurnerSwitchOffTemp;
    webUI.addJson(jsonDoc, "p06_boil_sw_off_temp", kmStatusCpy.BurnerSwitchOffTemp);
  }
  if (forceUpdate || kmStatusCpy.BoilerIntegral_1 != pkmStatus->BoilerIntegral_1) {
    kmStatusCpy.BoilerIntegral_1 = pkmStatus->BoilerIntegral_1;
  }
  if (forceUpdate || kmStatusCpy.BoilerIntegral_2 != pkmStatus->BoilerIntegral_2) {
    kmStatusCpy.BoilerIntegral_2 = pkmStatus->BoilerIntegral_2;
  }
  if (forceUpdate || kmStatusCpy.BoilerErrorStates != pkmStatus->BoilerErrorStates) {
    kmStatusCpy.BoilerErrorStates = pkmStatus->BoilerErrorStates;

    webUI.addJson(jsonDoc, "p06_boil_fail_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)));
    webUI.addJson(jsonDoc, "p06_boil_fail_boiler_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)));
    webUI.addJson(jsonDoc, "p06_boil_fail_aux_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)));
    webUI.addJson(jsonDoc, "p06_boil_fail_boiler_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)));
    webUI.addJson(jsonDoc, "p06_boil_fail_exh_gas_sens", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)));
    webUI.addJson(jsonDoc, "p06_boil_fail_exh_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)));
    webUI.addJson(jsonDoc, "p06_boil_fail_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)));
    webUI.addJson(jsonDoc, "p06_boil_fail_ext", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)));
  }
  if (forceUpdate || kmStatusCpy.BoilerOperatingStates != pkmStatus->BoilerOperatingStates) {
    kmStatusCpy.BoilerOperatingStates = pkmStatus->BoilerOperatingStates;

    webUI.addJson(jsonDoc, "p06_boil_state_exh_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)));
    webUI.addJson(jsonDoc, "p06_boil_s_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)));
    webUI.addJson(jsonDoc, "p06_boil_st_boiler_prot", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)));
    webUI.addJson(jsonDoc, "p06_boil_s_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)));
    webUI.addJson(jsonDoc, "p06_boil_s_perf_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)));
    webUI.addJson(jsonDoc, "p06_boil_s_perf_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)));
    webUI.addJson(jsonDoc, "p06_boil_s_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)));
  }
  if (forceUpdate || kmStatusCpy.BurnerStates != pkmStatus->BurnerStates) {
    kmStatusCpy.BurnerStates = pkmStatus->BurnerStates;
    webUI.addJson(jsonDoc, "p01_burner", (kmStatusCpy.BurnerStates == 0) ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]);
    webUI.addJson(jsonDoc, "p06_burn_ctrl", KM_CFG_ARRAY::BURNER_STATE[config.lang][kmStatusCpy.BurnerStates]);
  }
  if (forceUpdate || kmStatusCpy.ExhaustTemp != pkmStatus->ExhaustTemp) {
    kmStatusCpy.ExhaustTemp = pkmStatus->ExhaustTemp;
    webUI.addJson(jsonDoc, "p07_exh_gas_temp", kmStatusCpy.ExhaustTemp);
  }
  if (forceUpdate || kmStatusCpy.BurnerOperatingDuration_2 != pkmStatus->BurnerOperatingDuration_2) {
    kmStatusCpy.BurnerOperatingDuration_2 = pkmStatus->BurnerOperatingDuration_2;
  }
  if (forceUpdate || kmStatusCpy.BurnerOperatingDuration_1 != pkmStatus->BurnerOperatingDuration_1) {
    kmStatusCpy.BurnerOperatingDuration_1 = pkmStatus->BurnerOperatingDuration_1;
  }
  if (forceUpdate || kmStatusCpy.BurnerOperatingDuration_0 != pkmStatus->BurnerOperatingDuration_0) {
    kmStatusCpy.BurnerOperatingDuration_0 = pkmStatus->BurnerOperatingDuration_0;
  }
  if (forceUpdate || kmStatusCpy.BurnerOperatingDuration_Sum != pkmStatus->BurnerOperatingDuration_Sum) {
    kmStatusCpy.BurnerOperatingDuration_Sum = pkmStatus->BurnerOperatingDuration_Sum;
    timeComponents burnerRuntime = convertMinutes(kmStatusCpy.BurnerOperatingDuration_Sum);
    webUI.addJson(jsonDoc, "p06_burn_run_years", burnerRuntime.years);
    webUI.addJson(jsonDoc, "p06_burn_run_days", burnerRuntime.days);
    webUI.addJson(jsonDoc, "p06_burn_run_hours", burnerRuntime.hours);
    webUI.addJson(jsonDoc, "p06_burn_run_min", burnerRuntime.minutes);
  }
  if (forceUpdate || kmStatusCpy.BurnerCalcOilConsumption != pkmStatus->BurnerCalcOilConsumption) {
    kmStatusCpy.BurnerCalcOilConsumption = pkmStatus->BurnerCalcOilConsumption;
    // is handled somewhere else
  }
  if (forceUpdate || kmStatusCpy.OutsideTemp != pkmStatus->OutsideTemp) {
    kmStatusCpy.OutsideTemp = pkmStatus->OutsideTemp;
    webUI.addJson(jsonDoc, "p07_out_temp", kmStatusCpy.OutsideTemp);
    webUI.addJson(jsonDoc, "p01_temp_out_act", kmStatusCpy.OutsideTemp);
  }
  if (forceUpdate || kmStatusCpy.OutsideDampedTemp != pkmStatus->OutsideDampedTemp) {
    kmStatusCpy.OutsideDampedTemp = pkmStatus->OutsideDampedTemp;
    webUI.addJson(jsonDoc, "p07_out_temp_damped", kmStatusCpy.OutsideDampedTemp);
    webUI.addJson(jsonDoc, "p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp);
  }
  if (forceUpdate || kmStatusCpy.ControllerVersionMain != pkmStatus->ControllerVersionMain) {
    kmStatusCpy.ControllerVersionMain = pkmStatus->ControllerVersionMain;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    webUI.addJson(jsonDoc, "p09_logamatic_version", tmpMessage);
  }
  if (forceUpdate || kmStatusCpy.ControllerVersionSub != pkmStatus->ControllerVersionSub) {
    kmStatusCpy.ControllerVersionSub = pkmStatus->ControllerVersionSub;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    webUI.addJson(jsonDoc, "p09_logamatic_version", tmpMessage);
  }
  if (forceUpdate || kmStatusCpy.Modul != pkmStatus->Modul) {
    kmStatusCpy.Modul = pkmStatus->Modul;
    webUI.addJson(jsonDoc, "p09_logamatic_modul", kmStatusCpy.Modul);
  }
  if (forceUpdate || kmStatusCpy.ERR_Alarmstatus != pkmStatus->ERR_Alarmstatus) {
    kmStatusCpy.ERR_Alarmstatus = pkmStatus->ERR_Alarmstatus;
  }

  if (config.km271.use_solar) {

    if (forceUpdate || kmStatusCpy.SolarLoad != pkmStatus->SolarLoad) {
      kmStatusCpy.SolarLoad = pkmStatus->SolarLoad;
      webUI.addJson(jsonDoc, "p13_solar_load", (kmStatusCpy.SolarLoad == 0) ? WEB_TXT::OFF[config.lang] : WEB_TXT::ON[config.lang]);
    }

    if (forceUpdate || kmStatusCpy.SolarWW != pkmStatus->SolarWW) {
      kmStatusCpy.SolarWW = pkmStatus->SolarWW;
      webUI.addJson(jsonDoc, "p01_solar_ww", kmStatusCpy.SolarWW);
      webUI.addJson(jsonDoc, "p13_solar_ww", kmStatusCpy.SolarWW);
    }

    if (forceUpdate || kmStatusCpy.SolarCollector != pkmStatus->SolarCollector) {
      kmStatusCpy.SolarCollector = pkmStatus->SolarCollector;
      webUI.addJson(jsonDoc, "p01_solar_collector", kmStatusCpy.SolarCollector);
      webUI.addJson(jsonDoc, "p13_solar_collector", kmStatusCpy.SolarCollector);
    }

    if (forceUpdate || kmStatusCpy.SolarOperatingDuration_Sum != pkmStatus->SolarOperatingDuration_Sum) {
      kmStatusCpy.SolarOperatingDuration_Sum = pkmStatus->SolarOperatingDuration_Sum;
      timeComponents solarRuntime = convertMinutes(kmStatusCpy.SolarOperatingDuration_Sum);
      webUI.addJson(jsonDoc, "p13_solar_run_years", solarRuntime.years);
      webUI.addJson(jsonDoc, "p13_solar_run_days", solarRuntime.days);
      webUI.addJson(jsonDoc, "p13_solar_run_hours", solarRuntime.hours);
      webUI.addJson(jsonDoc, "p13_solar_run_min", solarRuntime.minutes);
    }

    if (forceUpdate || kmStatusCpy.Solar9147 != pkmStatus->Solar9147) {
      kmStatusCpy.Solar9147 = pkmStatus->Solar9147;
      webUI.addJson(jsonDoc, "p13_solar_9147", kmStatusCpy.Solar9147);
    }
  }

  // check if something is to send
  if (webUI.dataInJsonBuffer()) {
    webUI.wsUpdateWebJSON(jsonDoc);
  }
}
/**
 * *******************************************************************
 * @brief   callback function for OTA progress
 * @param   none
 * @return  none
 * *******************************************************************/
void otaProgressCallback(int progress) {
  if (otaProgessTimer.cycleTrigger(1000)) {
    webUI.wsSendHeartbeat();
    char buttonTxt[32];
    snprintf(buttonTxt, sizeof(buttonTxt), "updating: %i%%", progress);
    webUI.wsUpdateWebText("p00_update_btn", buttonTxt, false);
  }
}

/**
 * *******************************************************************
 * @brief   initiate GitHub version check
 * @param   none
 * @return  none
 * *******************************************************************/
bool startCheckGitHubVersion;
void requestGitHubVersion() { startCheckGitHubVersion = true; }
void processGitHubVersion() {
  if (startCheckGitHubVersion) {
    startCheckGitHubVersion = false;
    if (ghGetLatestRelease(&ghLatestRelease, &ghReleaseInfo)) {
      webUI.wsUpdateWebBusy("p00_dialog_git_version", false);
      webUI.wsUpdateWebText("p00_dialog_git_version", ghReleaseInfo.tag, false);
      webUI.wsUpdateWebHref("p00_dialog_git_version", ghReleaseInfo.url);
      // if new version is available, show update button
      if (strcmp(ghReleaseInfo.tag, VERSION) != 0) {
        char buttonTxt[32];
        snprintf(buttonTxt, sizeof(buttonTxt), "Update %s", ghReleaseInfo.tag);
        webUI.wsUpdateWebText("p00_update_btn", buttonTxt, false);
        webUI.wsUpdateWebHideElement("p00_update_btn_hide", false);
      }
    } else {
      webUI.wsUpdateWebBusy("p00_dialog_git_version", false);
      webUI.wsUpdateWebText("p00_dialog_git_version", "error", false);
    }
  }
}

/**
 * *******************************************************************
 * @brief   initiate GitHub version OTA update
 * @param   none
 * @return  none
 * *******************************************************************/
bool startGitHubUpdate;
void requestGitHubUpdate() { startGitHubUpdate = true; }
void processGitHubUpdate() {
  if (startGitHubUpdate) {
    startGitHubUpdate = false;
    ghSetProgressCallback(otaProgressCallback);
    webUI.wsUpdateWebText("p00_update_btn", "updating: 0%", false);
    webUI.wsUpdateWebDisabled("p00_update_btn", true);
    ota.setActive(true);
    wdt.disable();
    int result = ghStartOtaUpdate(ghLatestRelease, ghReleaseInfo.asset);
    if (result == OTA_SUCCESS) {
      webUI.wsUpdateWebText("p00_update_btn", "updating: 100%", false);
      webUI.wsUpdateWebDialog("version_dialog", "close");
      webUI.wsUpdateWebDialog("ota_update_done_dialog", "open");
      ESP_LOGI(TAG, "GitHub OTA-Update successful");
    } else {
      char errMsg[32];
      switch (result) {
      case OTA_NULL_URL:
        strcpy(errMsg, "URL is NULL");
        break;
      case OTA_CONNECT_ERROR:
        strcpy(errMsg, "Connection error");
        break;
      case OTA_BEGIN_ERROR:
        strcpy(errMsg, "Begin error");
        break;
      case OTA_WRITE_ERROR:
        strcpy(errMsg, "Write error");
        break;
      case OTA_END_ERROR:
        strcpy(errMsg, "End error");
        break;
      default:
        strcpy(errMsg, "Unknown error");
        break;
      }
      webUI.wsUpdateWebText("p00_ota_upd_err", errMsg, false);
      webUI.wsUpdateWebDialog("version_dialog", "close");
      webUI.wsUpdateWebDialog("ota_update_failed_dialog", "open");
      ESP_LOGE(TAG, "GitHub OTA-Update failed: %s", errMsg);
    }
    ota.setActive(false);
    wdt.enable();
  }
}

/**
 * *******************************************************************
 * @brief   cyclic update of webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUIupdates() {

  // check if new version is available
  processGitHubVersion();
  // perform GitHub update
  processGitHubUpdate();

  if (webLogRefreshActive()) {
    webReadLogBufferCyclic(); // update webUI Logger
  }

  // ON-BROWSER-REFRESH: refresh ALL elements - do this step by step not to stress the connection
  if (refreshTimerAll.cycleTrigger(WEBUI_FAST_REFRESH_TIME_MS) && refreshRequest && !ota.isActive()) {
    switch (UpdateCntRefresh) {
    case 0:
      updateSystemInfoElementsStatic(); // update static informations (≈ 200 Bytes)
      break;
    case 1:
      updateKm271ConfigElements(true); // force send all "Config" elements as one big JSON update ( ≈ 4 kB)
      break;
    case 2:
      updateKm271StatusElements(true); // send all "Status" elements as one big JSON update (≈ 4 kB)
      refreshRequest = false;
      break;
    default:
      UpdateCntRefresh = -1;
      break;
    }
    UpdateCntRefresh = (UpdateCntRefresh + 1) % 3;
  }

  // CYCLIC: update SINGLE elemets every x seconds - do this step by step not to stress the connection
  if (refreshTimerSingle.cycleTrigger(WEBUI_SLOW_REFRESH_TIME_MS) && !refreshRequest && !ota.isActive()) {

    switch (UpdateCntSlow) {
    case 0:
      updateSystemInfoElements(); // refresh all "System" elements as one big JSON update (≈ 570 Bytes)
      break;
    case 1:
      updateOilmeterElements(false); // check oilmeter and refresh if changed
      break;
    case 2:
      updateSensorElements(false); // send update of sensor elements
      break;
    case 3:
      if (!km271GetRefreshState()) {
        updateKm271ConfigElements(false); // check and send if Km271 config values has changed (≈ 4 kB)
      }
      break;
    case 4:
      if (!km271GetRefreshState()) {
        updateKm271AlarmElements(); // check if one or more "Alarm" elements have changed
      }
      break;
    case 5:
      if (!km271GetRefreshState()) {
        updateKm271StatusElements(false); // check if one or more "Status" elements have changed
      }
      break;
    default:
      UpdateCntSlow = -1;
      break;
    }
    UpdateCntSlow = (UpdateCntSlow + 1) % 6;
  }

  // show refresh layer - KM271 is receiving new data
  if (km271GetRefreshState() && !km271RefreshActiveOld) {
    webUI.wsUpdateWebHideElement("refreshBar", false);
    km271RefreshActiveOld = true;
  } else if (!km271GetRefreshState() && km271RefreshActiveOld) {
    km271RefreshActiveOld = false;
    webUI.wsUpdateWebHideElement("refreshBar", true);
  }
}
