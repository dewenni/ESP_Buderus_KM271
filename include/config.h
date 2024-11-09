#pragma once
#include <message.h>
#include <stdint.h>
/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION "v4.2.9005" // internal program version

#define WIFI_RECONNECT 10000 // Delay between wifi reconnection tries
#define MQTT_RECONNECT 10000 // Delay between mqtt reconnection tries

enum e_MsgFltTyp {
  MSG_FILTER_ALARM = 0, // only alarms
  MSG_FILTER_INFO = 1,  // alarm + info messages
};

enum e_LogFltTyp {
  LOG_FILTER_ALARM = 0,   // only alarms
  LOG_FILTER_INFO = 1,    // alarm + info messages
  LOG_FILTER_VALUES = 2,  // logamatic values
  LOG_FILTER_UNKNOWN = 3, // only unknown messages
  LOG_FILTER_DEBUG = 4,   // filtered debug messages
  LOG_FILTER_SYSTEM = 5,  // filtered debug messages
};

struct s_cfg_oilmeter {
  bool use_hardware_meter;
  bool use_virtual_meter = false;
  float consumption_kg_h = 2.0;
  float oil_density_kg_l = 0.85;
};

struct s_cfg_webUI {
  bool enable = true;
};

struct s_cfg_km271 {
  bool use_hc1;
  bool use_hc2;
  bool use_ww;
  bool use_alarmMsg;
};

struct s_cfg_wifi {
  char ssid[128];
  char password[128];
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
};

struct s_cfg_eth {
  bool enable = false;
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
  int gpio_sck;
  int gpio_mosi;
  int gpio_miso;
  int gpio_cs;
  int gpio_irq;
  int gpio_rst;
};

struct s_cfg_mqtt {
  bool enable;            // Enable or disable the MQTT server
  char server[128];       // MQTT Server IP
  char user[128];         // MQTT User Name
  char password[128];     // MQTT User Password
  char topic[128];        // MQTT Topic Prefix
  uint16_t port = 1883;   // MQTT Server Port
  bool config_retain;     // retain config messages
  uint16_t cyclicSendMin; // send messages every x minutes
  int lang;               // MQTT Topic Language
  bool ha_enable;         // MQTT discovery
  char ha_topic[64];      // MQTT ha topic
  char ha_device[32];     // MQTT ha topic
};

struct s_cfg_ntp {
  bool enable = true;
  bool auto_sync = false;
  char server[128] = {"de.pool.ntp.org"};
  char tz[128] = {"CET-1CEST,M3.5.0,M10.5.0/3"};
};

struct s_cfg_gpio {
  int led_wifi;
  int led_heartbeat;
  int led_logmode;
  int led_oilcounter;
  int trigger_oilcounter;
  int km271_RX;
  int km271_TX;
};

struct s_cfg_auth {
  bool enable = true;
  char user[64];
  char password[64];
};

struct s_cfg_debug {
  bool enable = true;
  char filter[33];
};

struct s_cfg_log {
  bool enable = true;
  int filter;
  int order;
};

struct s_cfg_sensor {
  bool ch1_enable = false;
  char ch1_name[32] = {"sensor1"};
  char ch1_description[32];
  int ch1_gpio = 18;
  bool ch2_enable = false;
  char ch2_name[32] = {"sensor2"};
  char ch2_description[32];
  int ch2_gpio = 19;
};

struct s_cfg_pushover {
  bool enable;       // Enable or disable the Pushover Service
  char token[64];    // Pushover API-Token
  char user_key[64]; // Pushover User-Key
  int filter;        // Messaging filter
};

struct s_cfg_sim {
  bool enable;
};

struct s_config {
  int lang;
  s_cfg_sim sim;
  s_cfg_oilmeter oilmeter;
  s_cfg_wifi wifi;
  s_cfg_eth eth;
  s_cfg_mqtt mqtt;
  s_cfg_ntp ntp;
  s_cfg_gpio gpio;
  s_cfg_webUI webUI;
  s_cfg_km271 km271;
  s_cfg_auth auth;
  s_cfg_debug debug;
  s_cfg_sensor sensor;
  s_cfg_pushover pushover;
  s_cfg_log log;
};

extern s_config config;
extern bool setupMode;
void configSetup();
void configCyclic();
void configSaveToFile();
void configLoadFromFile();
void configInitValue();
void saveRestartReason(const char *reason);
bool readRestartReason(char *buffer, size_t bufferSize);