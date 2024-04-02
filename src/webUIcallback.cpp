
#include <basics.h>
#include <km271.h>
#include <message.h>
#include <oilmeter.h>
#include <simulation.h>
#include <webUI.h>
#include <webUIupdates.h>

char pushoverMessage[300] = {'\0'};
long oilmeterSetValue;
tm dti;

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/
void webCallback(const char *elementId, const char *value) {
  msg("Received - Element ID: ");
  msg(elementId);
  msg(", Value: ");
  msgLn(value);

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

  // WW-Temp
  if (strcmp(elementId, "p02_ww_temp") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, atoi(value));
  }

  // WW-Pump Cycles
  if (strcmp(elementId, "p02_ww_circ") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, atoi(value));
  }

  // store input of Oilcounter value
  if (strcmp(elementId, "p02_oilmeter_set") == 0) {
    oilmeterSetValue = strtol(value, NULL, 10);
  }

  // Set new Oilcounter value
  if (strcmp(elementId, "p02_oilmeter_btn") == 0) {
    cmdSetOilmeter(oilmeterSetValue);
  }

  // WiFi
  if (strcmp(elementId, "p12_wifi_hostname") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.hostname), value);
  }
  if (strcmp(elementId, "p12_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), value);
  }
  if (strcmp(elementId, "p12_wifi_passw") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), value);
  }

  // IP-Settings
  if (strcmp(elementId, "p12_ip_enable") == 0) {
    config.ip.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_ip_adr") == 0) {
    snprintf(config.ip.ipaddress, sizeof(config.ip.ipaddress), value);
  }
  if (strcmp(elementId, "p12_ip_subnet") == 0) {
    snprintf(config.ip.subnet, sizeof(config.ip.subnet), value);
  }
  if (strcmp(elementId, "p12_ip_gateway") == 0) {
    snprintf(config.ip.gateway, sizeof(config.ip.gateway), value);
  }
  if (strcmp(elementId, "p12_ip_dns") == 0) {
    snprintf(config.ip.dns, sizeof(config.ip.dns), value);
  }

  // Authentication
  if (strcmp(elementId, "p12_auth_enable") == 0) {
    config.auth.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_auth_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), value);
  }
  if (strcmp(elementId, "p12_auth_passw") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), value);
  }

  // NTP-Server
  if (strcmp(elementId, "p12_ntp_enable") == 0) {
    config.ip.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), value);
  }
  if (strcmp(elementId, "p12_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), value);
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

  // MQTT
  if (strcmp(elementId, "p12_mqtt_enable") == 0) {
    config.mqtt.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_mqtt_cfg_ret") == 0) {
    config.mqtt.config_retain = stringToBool(value);
  }
  if (strcmp(elementId, "p12_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), value);
  }
  if (strcmp(elementId, "p12_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), value);
  }
  if (strcmp(elementId, "p12_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), value);
  }
  if (strcmp(elementId, "p12_mqtt_passw") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), value);
  }
  if (strcmp(elementId, "p12_mqtt_lang") == 0) {
    config.mqtt.lang = strtoul(value, NULL, 10);
  }

  // Pushover
  if (strcmp(elementId, "p12_pushover_enable") == 0) {
    config.pushover.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_pushover_api_token") == 0) {
    snprintf(config.pushover.token, sizeof(config.pushover.token), value);
  }
  if (strcmp(elementId, "p12_pushover_user_key") == 0) {
    snprintf(config.pushover.user_key, sizeof(config.pushover.user_key), value);
  }
  if (strcmp(elementId, "p12_pushover_filter") == 0) {
    config.pushover.filter = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_pushover_test_msg") == 0) {
    snprintf(pushoverMessage, sizeof(pushoverMessage), value);
  }
  if (strcmp(elementId, "p12_pushover_test_btn") == 0) {
    addPushoverMsg(pushoverMessage);
  }

  // Logamatic
  if (strcmp(elementId, "p12_hc1_enable") == 0) {
    config.km271.use_hc1 = stringToBool(value);
  }
  if (strcmp(elementId, "p12_hc2_enable") == 0) {
    config.km271.use_hc2 = stringToBool(value);
  }
  if (strcmp(elementId, "p12_hw_enable") == 0) {
    config.km271.use_ww = stringToBool(value);
  }
  if (strcmp(elementId, "p12_alarm_enable") == 0) {
    config.km271.use_alarmMsg = stringToBool(value);
  }

  // Hardware
  if (strcmp(elementId, "p12_gpio_km271_rx") == 0) {
    config.gpio.km271_RX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_km271_tx") == 0) {
    config.gpio.km271_TX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_heartbeat") == 0) {
    config.gpio.led_heartbeat = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_logmode") == 0) {
    config.gpio.led_logmode = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_wifi") == 0) {
    config.gpio.led_oilcounter = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_trig_oil") == 0) {
    config.gpio.trigger_oilcounter = strtoul(value, NULL, 10);
  }

  // Oil-Meter
  if (strcmp(elementId, "p12_oil_hw_enable") == 0) {
    config.oilmeter.use_hardware_meter = stringToBool(value);
  }
  if (strcmp(elementId, "p12_oil_virt_enable") == 0) {
    config.oilmeter.use_virtual_meter = stringToBool(value);
  }
  if (strcmp(elementId, "p12_oil_par1_kg_h") == 0) {
    config.oilmeter.consumption_kg_h = strtof(value, NULL);
  }
  if (strcmp(elementId, "p12_oil_par2_kg_l") == 0) {
    config.oilmeter.oil_density_kg_l = strtof(value, NULL);
  }

  // Optional Sensor
  if (strcmp(elementId, "p12_sens1_enable") == 0) {
    config.sensor.ch1_enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_sens1_name") == 0) {
    snprintf(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), value);
  }
  if (strcmp(elementId, "p12_sens1_description") == 0) {
    snprintf(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), value);
  }
  if (strcmp(elementId, "p12_sens1_gpio") == 0) {
    config.sensor.ch1_gpio = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_sens2_enable") == 0) {
    config.sensor.ch2_enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_sens2_name") == 0) {
    snprintf(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), value);
  }
  if (strcmp(elementId, "p12_sens2_description") == 0) {
    snprintf(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), value);
  }
  if (strcmp(elementId, "p12_sens2_gpio") == 0) {
    config.sensor.ch2_gpio = strtoul(value, NULL, 10);
  }

  // Language
  if (strcmp(elementId, "p12_language") == 0) {
    config.lang = strtoul(value, NULL, 10);
    updateAllElements();
  }

  // Buttons
  if (strcmp(elementId, "p12_btn_restart") == 0) {
    storeData();
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "p10_log_enable") == 0) {
    config.log.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p10_log_mode") == 0) {
    config.log.filter = strtoul(value, NULL, 10);
    clearLogBuffer();
    sendWebUpdate("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    sendWebUpdate("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    sendWebUpdate("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }
  // Simulation
  if (strcmp(elementId, "p12_sim_enable") == 0) {
    config.sim.enable = stringToBool(value);
    showElementClass("simModeBar", config.sim.enable);
  }
  if (strcmp(elementId, "p12_btn_simdata") == 0) {
    startSimData();
  }
  // OTA-Confirm
  if (strcmp(elementId, "p11_ota_confirm_btn") == 0) {
    updateWebDialog("ota_update_done_dialog", "close");
    storeData();
    ESP.restart();
  }
}
