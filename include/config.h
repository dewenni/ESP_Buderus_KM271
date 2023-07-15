#pragma once
#include <stdint.h>

/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define PROJECT             "ESP_Buderus_KM271"     // project info
#define VERSION             "v3.2.2"                // internal program version
#define DEBUG_ON                                    // enable debug messages

#define WIFI_RECONNECT      10000        // Delay between wifi reconnection tries
#define MQTT_RECONNECT      10000        // Delay between mqtt reconnection tries


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
    char ssid[128];                 // WiFi SSID
    char password[128];             // WiFi Password
    char hostname[128];             // HOSTNAME
} s_cfg_wifi;   

typedef struct {
    bool enable;                    // Enable or disable the MQTT server
    char server[128];               // MQTT Server IP
    char user[128];                 // MQTT User Name
    char password[128];             // MQTT User Password
    char topic[128];                // MQTT Topic Prefix
    uint16_t port = 1883;           // MQTT Server Port
    bool config_retain;             // retain config messages
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
    int lang;
    s_cfg_oilmeter oilmeter;
    s_cfg_wifi wifi;
    s_cfg_mqtt mqtt;
    s_cfg_ntp ntp;
    s_cfg_gpio gpio;
    s_cfg_webUI webUI;
    s_cfg_km271 km271;
} s_config;

extern s_config config;
extern bool setupMode;
void configSetup();
void configSaveToFile();
void configLoadFromFile();