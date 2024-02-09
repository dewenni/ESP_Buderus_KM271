#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>
#include <ESPUI.h>

#define LABLE_STYLE_GROUP               "background-color: unset; width: 100%; text-align: center;"
#define LABLE_STYLE_CLEAR               "background-color: unset; width: 60%; text-align: left;"
#define LABLE_STYLE_VALUE               "width: 30%;"
#define LABLE_STYLE_UNIT                "background-color: unset; width: 10%; text-align: left;"
#define LABLE_STYLE_DASH                "background-color: unset; width: 100%; font-size: 40px"
#define LABLE_STYLE_DESCRIPTION         "background-color: unset; width: 100%; text-align: center; font-weight: normal;"
#define LABLE_STYLE_INPUT_LABEL         "background-color: unset; width: 30%; text-align: left;"
#define LABLE_STYLE_INPUT_TEXT          "width: 65%; color: black"
#define LABLE_STYLE_NUMER_LABEL         "background-color: unset; width: 74%; text-align: left;"
#define LABLE_STYLE_SWITCH_LABEL        "background-color: unset; width: 83%; text-align: left;"

#define CUSTOM_CSS "<style>\
  .d30 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    background-color: unset;\
    width: 30\%;\
    text-align: left;\
  }\
  .d60 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    background-color: unset;\
    width: 60\%;\
    text-align: left;\
  }\
  .v30 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 30\%;\
  }\
  .v40 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 40\%;\
  }\
  .v70 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 70\%;\
  }\
  .u10 {\
    box-sizing: border-box;\
    white-space: nowrap;\
    border-radius: 0.2em;\
    padding: 0.12em 0.4em 0.14em;\
    text-align: center;\
    color: #ffffff;\
    font-weight: 700;\
    line-height: 1.3;\
    margin-bottom: 5px;\
    display: inline-block;\
    white-space: nowrap;\
    vertical-align: baseline;\
    position: relative;\
    top: -0.15em;\
    background-color: #999999;\
    margin-bottom: 10px;\
    white-space: pre-wrap;\
    word-wrap: break-word;\
    width: 10\%;\
    background-color: unset;\
    text-align: left;\
  }\
  a {color: #ffffff;}\
 </style>"



typedef struct {
uint16_t hc1pumpState;
uint16_t hc2pumpState;
uint16_t burnerState;
uint16_t hc1summerWinter;
uint16_t hc1dayNight;
uint16_t hc2summerWinter;
uint16_t hc2dayNight;
uint16_t ww_opmode;
uint16_t hc1_opmode;
uint16_t hc2_opmode;
uint16_t wwActTemp;
uint16_t wwSetTemp;
uint16_t burnerActTemp;
uint16_t burnerSetTemp;
uint16_t hc1flowActTemp;
uint16_t hc1flowSetTemp;
uint16_t hc2flowActTemp;
uint16_t hc2flowSetTemp;
uint16_t oilmeter;
uint16_t tmp_out_act;
uint16_t tmp_out_act_d;
uint16_t sens1_temp;
uint16_t sens2_temp;
} s_webui_id_dash;

typedef struct {
uint16_t wifiIP;
uint16_t wifiSignal;
uint16_t wifiRssi;
uint16_t sw_version;
uint16_t logamatic_version;
uint16_t logamatic_modul;
uint16_t espFreeHeap;
uint16_t espHeapSize;
uint16_t espMaxAllocHeap;
uint16_t espMinFreeHeap;
uint16_t date;
uint16_t time;
uint16_t date_input;
uint16_t time_input;
uint16_t dti_button;
uint16_t ntp_button;
} s_webui_id_system;

typedef struct {
uint16_t setdatetime;
uint16_t oilcounter;
uint16_t hc1_opmode;
uint16_t hc2_opmode;
uint16_t hc1_program;
uint16_t hc2_program;
uint16_t hc1_interpretation;
uint16_t hc2_interpretation;
uint16_t hc1_switch_off_threshold;
uint16_t hc2_switch_off_threshold;
uint16_t hc1_day_setpoint;
uint16_t hc2_day_setpoint;
uint16_t hc1_night_setpoint;
uint16_t hc2_night_setpoint;
uint16_t hc1_holiday_setpoint;
uint16_t hc2_holiday_setpoint;
uint16_t ww_opmode;
uint16_t hc1_summer_mode_threshold;
uint16_t hc1_frost_mode_threshold;
uint16_t hc2_summer_mode_threshold;
uint16_t hc2_frost_mode_threshold;
uint16_t ww_setpoint;
uint16_t hc1_holidays;
uint16_t hc2_holidays;
uint16_t oilmeter_input;
uint16_t oilmeter_button;
uint16_t oilmeter_output;
uint16_t hc1_holiday_days;
uint16_t hc2_holiday_days;
uint16_t ww_pump_cycles;
uint16_t hc1_reduction_mode;
uint16_t hc2_reduction_mode;
} s_webui_id_control;

typedef struct {
uint16_t alarm1;
uint16_t alarm2;
uint16_t alarm3;
uint16_t alarm4;
} s_webui_id_alarm;

typedef struct {
uint16_t btnRestart;
uint16_t btnSave;
uint16_t btnSaveRestart;
uint16_t btnPrint;
uint16_t wifi_ssid;
uint16_t wifi_passw;
uint16_t wifi_hostname;
uint16_t wifi_otaIP;
uint16_t mqtt_enable;
uint16_t mqtt_server;
uint16_t mqtt_user;
uint16_t mqtt_passw;
uint16_t mqtt_topic;
uint16_t mqtt_port;
uint16_t mqtt_config_retain;
uint16_t km271_useHc1;
uint16_t km271_useHc2;
uint16_t km271_useWW;
uint16_t km271_useAlarm;
uint16_t gpio_board;
uint16_t gpio_led_wifi;
uint16_t gpio_led_heatbeat;
uint16_t gpio_led_logmode;
uint16_t gpio_led_oilcounter;
uint16_t gpio_trigger_oilcounter;
uint16_t gpio_km271_tx;
uint16_t gpio_km271_rx;
uint16_t language;
uint16_t ntp_enable;
uint16_t ntp_server;
uint16_t ntp_tz;
uint16_t oil_useHardware;
uint16_t oil_useVirtual;
uint16_t oil_consumption_kg_h;
uint16_t oil_oil_density_kg_l;
uint16_t ip_enable;
uint16_t ip_ipaddress;
uint16_t ip_subnet;
uint16_t ip_gateway;
uint16_t ip_dns;
uint16_t auth_enable;
uint16_t auth_user;
uint16_t auth_passw;
uint16_t webUI_enable;
uint16_t sens1_enable;
uint16_t sens1_name;
uint16_t sens1_gpio;
uint16_t sens2_enable;
uint16_t sens2_name;
uint16_t sens2_gpio;
uint16_t pushover_enable;
uint16_t pushover_token;
uint16_t pushover_user_key;
uint16_t pushover_filter;
uint16_t pushover_btn_test;
uint16_t log_enable;
uint16_t log_filter;
} s_webui_id_settings;

typedef struct {
uint16_t dashboard;
uint16_t control;
uint16_t hc1;
uint16_t hc2;
uint16_t ww;
uint16_t boiler;
uint16_t general;
uint16_t system;
uint16_t alarm;
uint16_t settings;
} s_webui_id_tab;

typedef struct {
uint16_t hc1_config;
uint16_t hc1_status;
uint16_t hc1_bw1;
uint16_t hc1_bw2;
uint16_t hc1_prog;
uint16_t hc2_config;
uint16_t hc2_status;
uint16_t hc2_bw1;
uint16_t hc2_bw2;
uint16_t hc2_prog;
uint16_t ww_config;
uint16_t ww_status;
uint16_t ww_bw1;
uint16_t ww_bw2;
uint16_t boiler_status;
uint16_t boiler_stages;
uint16_t boiler_error;
uint16_t boiler_lifetime;
uint16_t general_config;
uint16_t general_temp;
uint16_t general_limits;
uint16_t alarm;
uint16_t system_wifi;
uint16_t system_esp;
uint16_t system_version;
} s_webui_id_tables;

typedef struct {
s_webui_id_dash dash;
s_webui_id_control ctrl;
s_webui_id_system sys;
s_webui_id_settings settings;
s_webui_id_alarm alarm;
s_webui_id_tables tables;
s_webui_id_tab tab;
} s_webui_id;


// ======================================================
// Prototypes
// ======================================================
void webUISetup();
void webUICylic();
