#pragma once
#include <message.h>
#include <stdint.h>
/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION "v4.0.2" // internal program version

#define WIFI_RECONNECT 10000 // Delay between wifi reconnection tries
#define MQTT_RECONNECT 10000 // Delay between mqtt reconnection tries

typedef enum {
  MSG_FILTER_ALARM = 0, // only alarms
  MSG_FILTER_INFO = 1,  // alarm + info messages
} e_MsgFltTyp;

typedef enum {
  LOG_FILTER_ALARM = 0,   // only alarms
  LOG_FILTER_INFO = 1,    // alarm + info messages
  LOG_FILTER_VALUES = 2,  // logamatic values
  LOG_FILTER_UNKNOWN = 3, // only unknown messages
  LOG_FILTER_DEBUG = 4,   // filtered debug messages
} e_LogFltTyp;

typedef struct {
  bool use_hardware_meter;
  bool use_virtual_meter = false;
  float consumption_kg_h = 2.0;
  float oil_density_kg_l = 0.85;
} s_cfg_oilmeter;

typedef struct {
  bool enable = true;
} s_cfg_webUI;

typedef struct {
  bool use_hc1;
  bool use_hc2;
  bool use_ww;
  bool use_alarmMsg;
} s_cfg_km271;

typedef struct {
  char ssid[128];     // WiFi SSID
  char password[128]; // WiFi Password
  char hostname[128]; // HOSTNAME
} s_cfg_wifi;

typedef struct {
  bool enable;          // Enable or disable the MQTT server
  char server[128];     // MQTT Server IP
  char user[128];       // MQTT User Name
  char password[128];   // MQTT User Password
  char topic[128];      // MQTT Topic Prefix
  uint16_t port = 1883; // MQTT Server Port
  bool config_retain;   // retain config messages
  int lang;             // MQTT Topic Language
} s_cfg_mqtt;

typedef struct {
  bool enable = true;
  char server[128] = {"de.pool.ntp.org"};
  char tz[128] = {"CET-1CEST,M3.5.0,M10.5.0/3"};
} s_cfg_ntp;

typedef struct {
  int led_wifi;
  int led_heartbeat;
  int led_logmode;
  int led_oilcounter;
  int trigger_oilcounter;
  int km271_RX;
  int km271_TX;
} s_cfg_gpio;

typedef struct {
  bool enable = true;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
} s_cfg_ip;

typedef struct {
  bool enable = true;
  char user[64];
  char password[64];
} s_cfg_auth;

typedef struct {
  bool enable = true;
  char filter[33];
} s_cfg_debug;

typedef struct {
  bool enable = true;
  int filter;
  int order;
} s_cfg_log;

typedef struct {
  bool ch1_enable = false;
  char ch1_name[32] = {"sensor1"};
  char ch1_description[32];
  int ch1_gpio = 18;
  bool ch2_enable = false;
  char ch2_name[32] = {"sensor2"};
  char ch2_description[32];
  int ch2_gpio = 19;
} s_cfg_sensor;

typedef struct {
  bool enable;       // Enable or disable the Pushover Service
  char token[64];    // Pushover API-Token
  char user_key[64]; // Pushover User-Key
  int filter;        // Messaging filter
} s_cfg_pushover;

typedef struct {
  bool enable;
} s_cfg_sim;

typedef struct {
  int lang;
  s_cfg_sim sim;
  s_cfg_oilmeter oilmeter;
  s_cfg_wifi wifi;
  s_cfg_mqtt mqtt;
  s_cfg_ntp ntp;
  s_cfg_gpio gpio;
  s_cfg_webUI webUI;
  s_cfg_km271 km271;
  s_cfg_ip ip;
  s_cfg_auth auth;
  s_cfg_debug debug;
  s_cfg_sensor sensor;
  s_cfg_pushover pushover;
  s_cfg_log log;
} s_config;

extern s_config config;
extern bool setupMode;
void configSetup();
void configCyclic();
void configSaveToFile();
void configLoadFromFile();
void configInitValue();