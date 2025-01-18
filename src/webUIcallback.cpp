
#include <basics.h>
#include <km271.h>
#include <message.h>
#include <oilmeter.h>
#include <simulation.h>
#include <webUI.h>
#include <webUIupdates.h>

static tm dti;
static const char *TAG = "WEB"; // LOG TAG

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/
void webCallback(const char *elementId, const char *value) {

  MY_LOGD(TAG, "Received - Element ID: %s = %s", elementId, value);

  // ------------------------------------------------------------------
  // GitHub / Version
  // ------------------------------------------------------------------

  // Github Check Version
  if (strcmp(elementId, "check_git_version") == 0) {
    requestGitHubVersion();
  }
  // Github Update
  if (strcmp(elementId, "p00_update_btn") == 0) {
    requestGitHubUpdate();
  }
  // OTA-Confirm
  if (strcmp(elementId, "p00_ota_confirm_btn") == 0) {
    updateWebDialog("ota_update_done_dialog", "close");
    EspSysUtil::RestartReason::saveLocal("ota update");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // HC1-OPMODE
  if (strcmp(elementId, "p02_hc1_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 0);
  } else if (strcmp(elementId, "p02_hc1_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 1);
  } else if (strcmp(elementId, "p02_hc1_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 2);
  }

  // HC2-OPMODE
  if (strcmp(elementId, "p02_hc2_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 0);
  } else if (strcmp(elementId, "p02_hc2_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 1);
  } else if (strcmp(elementId, "p02_hc2_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 2);
  }

  // WW-OPMODE
  if (strcmp(elementId, "p02_ww_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 0);
  } else if (strcmp(elementId, "p02_ww_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 1);
  } else if (strcmp(elementId, "p02_ww_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 2);
  }

  // HC1-Program
  if (strcmp(elementId, "p02_hc1_prg") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, atoi(value));
  }

  // HC2-Program
  if (strcmp(elementId, "p02_hc2_prg") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, atoi(value));
  }

  // HC1-Reduction-Mode
  if (strcmp(elementId, "p02_hc1_reduct_mode") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, atoi(value));
  }

  // HC2-Reduction-Mode
  if (strcmp(elementId, "p02_hc2_reduct_mode") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, atoi(value));
  }

  // HC1-Frost Threshold
  if (strcmp(elementId, "p02_hc1_frost_prot_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, atoi(value));
  }

  // HC1-Summer Threshold
  if (strcmp(elementId, "p02_hc1_summer_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SUMMER, atoi(value));
  }

  // HC2-Frost Threshold
  if (strcmp(elementId, "p02_hc2_frost_prot_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, atoi(value));
  }

  // HC2-Summer Threshold
  if (strcmp(elementId, "p02_hc2_summer_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SUMMER, atoi(value));
  }

  // HC1-DesignTemp
  if (strcmp(elementId, "p02_hc1_interp") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, atoi(value));
  }

  // HC2-DesignTemp
  if (strcmp(elementId, "p02_hc2_interp") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, atoi(value));
  }

  // HC1-SwitchOffTemp
  if (strcmp(elementId, "p02_hc1_sw_off_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, atoi(value));
  }

  // HC2-SwitchOffTemp
  if (strcmp(elementId, "p02_hc2_sw_off_th") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, atoi(value));
  }

  // HC1-Holiday days
  if (strcmp(elementId, "p02_hc1_holiday_days") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, atoi(value));
  }

  // HC2-Holiday days
  if (strcmp(elementId, "p02_hc2_holiday_days") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, atoi(value));
  }

  // HC1-Day Temp
  if (strcmp(elementId, "p02_hc1_day_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, atof(value));
  }

  // HC2-Day Temp
  if (strcmp(elementId, "p02_hc2_day_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, atof(value));
  }

  // HC1-Night Temp
  if (strcmp(elementId, "p02_hc1_night_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, atof(value));
  }

  // HC2-Night Temp
  if (strcmp(elementId, "p02_hc2_night_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, atof(value));
  }

  // HC1-Holiday Temp
  if (strcmp(elementId, "p02_hc1_holiday_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, atof(value));
  }

  // HC2-Holiday Temp
  if (strcmp(elementId, "p02_hc2_holiday_temp") == 0) {
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, atof(value));
  }

  // HC1-SwitchOn Temp
  if (strcmp(elementId, "p02_hc1_sw_on_temp") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_ON_TEMP, atoi(value));
  }

  // HC2-SwitchOn Temp
  if (strcmp(elementId, "p02_hc2_sw_on_temp") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_ON_TEMP, atoi(value));
  }

  // WW-Temp
  if (strcmp(elementId, "p02_ww_temp") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, atoi(value));
  }

  // WW-Pump Cycles
  if (strcmp(elementId, "p02_ww_circ") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, atoi(value));
  }

  // Set new Oilcounter value
  if (strcmp(elementId, "p02_oilmeter_set_cmd") == 0) {
    cmdSetOilmeter(strtol(value, NULL, 10));
  }

  // WiFi
  if (strcmp(elementId, "cfg_wifi_hostname") == 0) {
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), value);
  }
  if (strcmp(elementId, "cfg_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), value);
  }
  if (strcmp(elementId, "cfg_wifi_password") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), value);
  }
  if (strcmp(elementId, "cfg_wifi_static_ip") == 0) {
    config.wifi.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_wifi_ipaddress") == 0) {
    snprintf(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_wifi_subnet") == 0) {
    snprintf(config.wifi.subnet, sizeof(config.wifi.subnet), value);
  }
  if (strcmp(elementId, "cfg_wifi_gateway") == 0) {
    snprintf(config.wifi.gateway, sizeof(config.wifi.gateway), value);
  }
  if (strcmp(elementId, "cfg_wifi_dns") == 0) {
    snprintf(config.wifi.dns, sizeof(config.wifi.dns), value);
  }

  // Ethernet
  if (strcmp(elementId, "cfg_eth_enable") == 0) {
    config.eth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_hostname") == 0) {
    snprintf(config.eth.hostname, sizeof(config.eth.hostname), value);
  }
  if (strcmp(elementId, "cfg_eth_gpio_sck") == 0) {
    config.eth.gpio_sck = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_mosi") == 0) {
    config.eth.gpio_mosi = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_miso") == 0) {
    config.eth.gpio_miso = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_cs") == 0) {
    config.eth.gpio_cs = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_irq") == 0) {
    config.eth.gpio_irq = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_gpio_rst") == 0) {
    config.eth.gpio_rst = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_eth_static_ip") == 0) {
    config.eth.static_ip = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_eth_ipaddress") == 0) {
    snprintf(config.eth.ipaddress, sizeof(config.eth.ipaddress), value);
  }
  if (strcmp(elementId, "cfg_eth_subnet") == 0) {
    snprintf(config.eth.subnet, sizeof(config.eth.subnet), value);
  }
  if (strcmp(elementId, "cfg_eth_gateway") == 0) {
    snprintf(config.eth.gateway, sizeof(config.eth.gateway), value);
  }
  if (strcmp(elementId, "cfg_eth_dns") == 0) {
    snprintf(config.eth.dns, sizeof(config.eth.dns), value);
  }

  // Authentication
  if (strcmp(elementId, "cfg_auth_enable") == 0) {
    config.auth.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_auth_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_auth_password") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), "%s", value);
  }

  // NTP-Server
  if (strcmp(elementId, "cfg_ntp_enable") == 0) {
    config.ntp.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "%s", value);
  }

  // set manual date for Logamatic
  if (strcmp(elementId, "p12_dti_date") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    /* ---------------- INFO ---------------------------------
    dti.tm_year + 1900  // years since 1900
    dti.tm_mon + 1      // January = 0 (!)
    dti.tm_mday         // day of month
    dti.tm_hour         // hours since midnight  0-23
    dti.tm_min          // minutes after the hour  0-59
    dti.tm_sec          // seconds after the minute  0-61*
    dti.tm_wday         // days since Sunday 0-6
    dti.tm_isdst        // Daylight Saving Time flag
    --------------------------------------------------------- */
    // get date
    strncpy(tmp1, value, sizeof(tmp1));
    // extract year
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 4);
    dti.tm_year = atoi(tmp2) - 1900;
    // extract month
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 5, 2);
    dti.tm_mon = atoi(tmp2) - 1;
    // extract day
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 8, 2);
    dti.tm_mday = atoi(tmp2);
    // calculate day of week
    int d = dti.tm_mday;        // Day     1-31
    int m = dti.tm_mon + 1;     // Month   1-12
    int y = dti.tm_year + 1900; // Year    2022
    dti.tm_wday = (d += m < 3 ? y-- : y - 2,
                   23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7; // calculate day of week
  }
  // get time
  if (strcmp(elementId, "p12_dti_time") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    strncpy(tmp1, value, sizeof(tmp1));
    // extract hour
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 2);
    dti.tm_hour = atoi(tmp2);
    // extract minutes
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 3, 2);
    dti.tm_min = atoi(tmp2);
  }
  // set date and time on Logamatic
  if (strcmp(elementId, "p12_dti_btn") == 0) {
    km271SetDateTimeDTI(dti);
    // TODO: check
  }
  if (strcmp(elementId, "cfg_ntp_auto_sync") == 0) {
    config.ntp.auto_sync = EspStrUtil::stringToBool(value);
  }

  // MQTT
  if (strcmp(elementId, "cfg_mqtt_enable") == 0) {
    config.mqtt.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_config_retain") == 0) {
    config.mqtt.config_retain = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_password") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_language") == 0) {
    config.mqtt.lang = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_cyclic_send") == 0) {
    config.mqtt.cyclicSendMin = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_enable") == 0) {
    config.mqtt.ha_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_topic") == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "%s", value);
  }
  if (strcmp(elementId, "cfg_mqtt_ha_device") == 0) {
    snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "%s", value);
  }

  // Pushover
  if (strcmp(elementId, "cfg_pushover_enable") == 0) {
    config.pushover.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_pushover_token") == 0) {
    snprintf(config.pushover.token, sizeof(config.pushover.token), "%s", value);
  }
  if (strcmp(elementId, "cfg_pushover_user_key") == 0) {
    snprintf(config.pushover.user_key, sizeof(config.pushover.user_key), "%s", value);
  }
  if (strcmp(elementId, "cfg_pushover_filter") == 0) {
    config.pushover.filter = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_pushover_test_msg_cmd") == 0) {
    addPushoverMsg(value);
  }

  // Logamatic
  if (strcmp(elementId, "cfg_km271_use_hc1") == 0) {
    config.km271.use_hc1 = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_km271_use_hc2") == 0) {
    config.km271.use_hc2 = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_km271_use_ww") == 0) {
    config.km271.use_ww = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_km271_use_solar") == 0) {
    config.km271.use_solar = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_km271_use_alarmMsg") == 0) {
    config.km271.use_alarmMsg = EspStrUtil::stringToBool(value);
  }

  // Hardware
  // predefined gpio settings
  if (strcmp(elementId, "p12_boards") == 0) {

    switch (strtoul(value, NULL, 10)) {

    case 1: // generic ESP32
      config.gpio.km271_RX = 16;
      config.gpio.km271_TX = 17;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = -1;
      config.gpio.led_logmode = -1;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;

    case 2: // KM271-WiFi v0.0.5
      config.gpio.km271_RX = 4;
      config.gpio.km271_TX = 2;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = 22;
      config.gpio.led_logmode = 23;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;

    case 3: // KM271-WiFi >= v0.0.6
      config.gpio.km271_RX = 4;
      config.gpio.km271_TX = 2;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = 22;
      config.gpio.led_logmode = 17;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;
    }
    updateGpioSettings();
    configGPIO();
  }
  if (strcmp(elementId, "cfg_gpio_km271_RX") == 0) {
    config.gpio.km271_RX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_km271_TX") == 0) {
    config.gpio.km271_TX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_gpio_led_heartbeat") == 0) {
    config.gpio.led_heartbeat = strtoul(value, NULL, 10);
    configGPIO();
  }
  if (strcmp(elementId, "cfg_gpio_led_logmode") == 0) {
    config.gpio.led_logmode = strtoul(value, NULL, 10);
    configGPIO();
  }
  if (strcmp(elementId, "cfg_gpio_led_wifi") == 0) {
    config.gpio.led_wifi = strtoul(value, NULL, 10);
    configGPIO();
  }
  if (strcmp(elementId, "cfg_gpio_led_oilcounter") == 0) {
    config.gpio.led_oilcounter = strtoul(value, NULL, 10);
    configGPIO();
  }
  if (strcmp(elementId, "cfg_gpio_trigger_oilcounter") == 0) {
    config.gpio.trigger_oilcounter = strtoul(value, NULL, 10);
    configGPIO();
  }

  // Oil-Meter
  if (strcmp(elementId, "cfg_oilmeter_use_hardware_meter") == 0) {
    config.oilmeter.use_hardware_meter = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_oilmeter_use_virtual_meter") == 0) {
    config.oilmeter.use_virtual_meter = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_oilmeter_consumption_kg_h") == 0) {
    config.oilmeter.consumption_kg_h = strtof(value, NULL);
  }
  if (strcmp(elementId, "cfg_oilmeter_oil_density_kg_l") == 0) {
    config.oilmeter.oil_density_kg_l = strtof(value, NULL);
  }

  // Optional Sensor
  if (strcmp(elementId, "cfg_sensor_ch1_enable") == 0) {
    config.sensor.ch1_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_sensor_ch1_name") == 0) {
    snprintf(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), "%s", value);
  }
  if (strcmp(elementId, "cfg_sensor_ch1_description") == 0) {
    snprintf(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), "%s", value);
  }
  if (strcmp(elementId, "cfg_sensor_ch1_gpio") == 0) {
    config.sensor.ch1_gpio = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "cfg_sensor_ch2_enable") == 0) {
    config.sensor.ch2_enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_sensor_ch2_name") == 0) {
    snprintf(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), "%s", value);
  }
  if (strcmp(elementId, "cfg_sensor_ch2_description") == 0) {
    snprintf(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), "%s", value);
  }
  if (strcmp(elementId, "cfg_sensor_ch2_gpio") == 0) {
    config.sensor.ch2_gpio = strtoul(value, NULL, 10);
  }

  // Language
  if (strcmp(elementId, "cfg_lang") == 0) {
    config.lang = strtoul(value, NULL, 10);
    updateAllElements();
  }

  // Buttons
  if (strcmp(elementId, "p12_btn_restart") == 0) {
    storeData();
    EspSysUtil::RestartReason::saveLocal("webUI command");
    yield();
    delay(1000);
    yield();
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "cfg_logger_enable") == 0) {
    config.log.enable = EspStrUtil::stringToBool(value);
  }
  if (strcmp(elementId, "cfg_logger_filter") == 0) {
    config.log.filter = strtoul(value, NULL, 10);
    clearLogBuffer();
    updateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "cfg_logger_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    updateWebLog("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    updateWebLog("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }
  // Simulation
  if (strcmp(elementId, "cfg_sim_enable") == 0) {
    config.sim.enable = EspStrUtil::stringToBool(value);
    showElementClass("simModeBar", config.sim.enable);
  }
  if (strcmp(elementId, "p12_btn_simdata") == 0) {
    startSimData();
  }
}
