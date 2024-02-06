#include <config.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/* D E C L A R A T I O N S ****************************************************/  
#define JSON_SIZE 2048
char filename[24] = {"/config.json"};
bool setupMode;
s_config config;


/* P R O T O T Y P E S ********************************************************/
void configGPIO();
void configInitValue();


/**
 * *******************************************************************
 * @brief   check before read from file
 * @param   dest, size, src
 * @return  none
 * *******************************************************************/
void readJSONstring(char* dest, size_t size, const char* src){
  const char* check = src;
  if (check != NULL) {
    snprintf(dest, size, src);
  }
}

/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup(){
  
  // start Filesystem 
  Serial.print("LittleFS Status: ");
  Serial.println(LittleFS.begin(true));

  // load config from file
  configLoadFromFile();  
  // gpio settings
  configGPIO();

}

/**
 * *******************************************************************
 * @brief   Setup for GPIO
 * @param   none
 * @return  none
 * *******************************************************************/
void configGPIO(){

  if (setupMode){
    pinMode(LED_BUILTIN, OUTPUT);   // onboard LED
    pinMode(21, OUTPUT);            // green LED D1 on the78mole boards
  }
  else {
    if (config.gpio.led_wifi != -1)
      pinMode(config.gpio.led_wifi, OUTPUT);        // LED for Wifi-Status
    if (config.gpio.led_heartbeat != -1)
      pinMode(config.gpio.led_heartbeat, OUTPUT);   // LED for heartbeat
    if (config.gpio.led_logmode != -1)
    pinMode(config.gpio.led_logmode, OUTPUT);       // LED for LogMode-Status
    
    if (config.oilmeter.use_hardware_meter){
      if (config.gpio.trigger_oilcounter != -1)
        pinMode(config.gpio.trigger_oilcounter, INPUT_PULLUP);    // Trigger Input
      if (config.gpio.led_oilcounter != -1)
        pinMode(config.gpio.led_oilcounter, OUTPUT);              // Status LED
    }
  }

}

/**
 * *******************************************************************
 * @brief   intitial configuration values
 * @param   none
 * @return  none
 * *******************************************************************/
void configInitValue(){

    memset(&config, 0, sizeof(config));

    // WiFi
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), "enter SSID...");
    snprintf(config.wifi.password, sizeof(config.wifi.password), "enter password...");
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "ESP-Buderus-KM271");

    // MQTT
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), "enter MQTT server...");
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), "enter MQTT user...");
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), "enter MQTT password...");
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), "enter MQTT topic...");
    config.mqtt.port = 1883;
    config.mqtt.enable = false;

    // NTP
    snprintf(config.ntp.server, sizeof(config.ntp.server), "de.pool.ntp.org");
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), "CET-1CEST,M3.5.0,M10.5.0/3");
    config.ntp.enable = true;

    // heating circuits
    config.km271.use_hc1 = false;
    config.km271.use_hc2 = false;
    config.km271.use_ww = false;

    // km271
    config.km271.use_alarmMsg = false;

    // language
    config.lang = 0;
    
    // oilmeter
    config.oilmeter.use_hardware_meter = false;
    config.oilmeter.use_virtual_meter = false;

    // webUI
    config.webUI.enable = true;

    // gpio
    memset(&config.gpio, -1, sizeof(config.gpio));

    // Pushover
    snprintf(config.pushover.token, sizeof(config.pushover.token), "enter Api-Token here");
    snprintf(config.pushover.user_key, sizeof(config.pushover.user_key), "enter User-Key here");

}

/**
 * *******************************************************************
 * @brief   save configuration to file
 * @param   none
 * @return  none
 * *******************************************************************/
void configSaveToFile() {

    DynamicJsonDocument doc(JSON_SIZE); // reserviert 2048 Bytes für das JSON-Objekt

    doc["lang"] = (config.lang);

    doc["oilmeter"]["use_hardware_meter"] = config.oilmeter.use_hardware_meter;
    doc["oilmeter"]["use_virtual_meter"] = config.oilmeter.use_virtual_meter;
    doc["oilmeter"]["consumption_kg_h"] = config.oilmeter.consumption_kg_h;
    doc["oilmeter"]["oil_density_kg_l"] = config.oilmeter.oil_density_kg_l;

    doc["wifi"]["ssid"] = config.wifi.ssid;
    doc["wifi"]["password"] = config.wifi.password;
    doc["wifi"]["hostname"] = config.wifi.hostname;

    doc["mqtt"]["enable"] = config.mqtt.enable;
    doc["mqtt"]["server"] = config.mqtt.server;
    doc["mqtt"]["user"] = config.mqtt.user;
    doc["mqtt"]["password"] = config.mqtt.password;
    doc["mqtt"]["topic"] = config.mqtt.topic;
    doc["mqtt"]["port"] = config.mqtt.port;
    doc["mqtt"]["config_retain"] = config.mqtt.config_retain;

    doc["ntp"]["enable"] = config.ntp.enable;
    doc["ntp"]["server"] = config.ntp.server;
    doc["ntp"]["tz"] = config.ntp.tz;

    doc["gpio"]["led_wifi"] = config.gpio.led_wifi;
    doc["gpio"]["led_heartbeat"] = config.gpio.led_heartbeat;
    doc["gpio"]["led_logmode"] = config.gpio.led_logmode;
    doc["gpio"]["led_oilcounter"] = config.gpio.led_oilcounter;
    doc["gpio"]["trigger_oilcounter"] = config.gpio.trigger_oilcounter;
    doc["gpio"]["km271_RX"] = config.gpio.km271_RX;
    doc["gpio"]["km271_TX"] = config.gpio.km271_TX;

    doc["webUI"]["enable"] = config.webUI.enable;

    doc["km271"]["use_hc1"] = config.km271.use_hc1;
    doc["km271"]["use_hc2"] = config.km271.use_hc2;
    doc["km271"]["use_ww"] = config.km271.use_ww;
    doc["km271"]["use_alarmMsg"] = config.km271.use_alarmMsg;

    doc["ip"]["enable"] = config.ip.enable;
    doc["ip"]["ipaddress"] = config.ip.ipaddress;
    doc["ip"]["subnet"] = config.ip.subnet;
    doc["ip"]["gateway"] = config.ip.gateway;
    doc["ip"]["dns"] = config.ip.dns;

    doc["auth"]["enable"] = config.auth.enable;
    doc["auth"]["user"] = config.auth.user;
    doc["auth"]["password"] = config.auth.password;

    doc["debug"]["enable"] = config.debug.enable;
    doc["debug"]["filter"] = config.debug.filter;

    doc["sensor"]["ch1_enable"] = config.sensor.ch1_enable;
    doc["sensor"]["ch1_name"] = config.sensor.ch1_name;
    doc["sensor"]["ch1_gpio"] = config.sensor.ch1_gpio;
    doc["sensor"]["ch2_enable"] = config.sensor.ch2_enable;
    doc["sensor"]["ch2_name"] = config.sensor.ch2_name;
    doc["sensor"]["ch2_gpio"] = config.sensor.ch2_gpio;

    doc["pushover"]["enable"] = config.pushover.enable;
    doc["pushover"]["token"] = config.pushover.token;
    doc["pushover"]["user_key"] = config.pushover.user_key;
    doc["pushover"]["filter"] = config.pushover.filter;

    doc["logger"]["enable"] = config.log.enable;
    doc["logger"]["filter"] = config.log.filter;

    // Delete existing file, otherwise the configuration is appended to the file
    LittleFS.remove(filename);

    // Open file for writing
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println(F("Failed to create file"));
      return;
    }

    // Serialize JSON to file
    if (serializeJson(doc, file) == 0) {
      Serial.println(F("Failed to write to file"));
    }
    else {
      Serial.print("config successfully saved to file: ");
      Serial.println(filename);
    }

    // Close the file
    file.close();
}

/**
 * *******************************************************************
 * @brief   load configuration from file
 * @param   none
 * @return  none
 * *******************************************************************/
void configLoadFromFile() {
  // Open file for reading
  File file = LittleFS.open(filename);

  // Allocate a temporary JsonDocument
  StaticJsonDocument<JSON_SIZE> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to read file, using default configuration and start wifi-AP"));
    configInitValue();
    setupMode = true;
  }
  else {
    // Copy values from the JsonDocument to the Config structure
    config.oilmeter.use_hardware_meter = doc["oilmeter"]["use_hardware_meter"];
    config.oilmeter.use_virtual_meter = doc["oilmeter"]["use_virtual_meter"];
    config.oilmeter.consumption_kg_h = doc["oilmeter"]["consumption_kg_h"];
    config.oilmeter.oil_density_kg_l = doc["oilmeter"]["oil_density_kg_l"];
    
    config.lang = doc["lang"];

    readJSONstring(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);
    readJSONstring(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
    readJSONstring(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
    
    config.mqtt.enable = doc["mqtt"]["enable"];
    readJSONstring(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
    readJSONstring(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);
    readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
    readJSONstring(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
    config.mqtt.port = doc["mqtt"]["port"];
    config.mqtt.config_retain = doc["mqtt"]["config_retain"];

    config.ntp.enable = doc["ntp"]["enable"];
    readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);
    
    config.gpio.led_wifi = doc["gpio"]["led_wifi"];
    config.gpio.led_heartbeat = doc["gpio"]["led_heartbeat"];
    config.gpio.led_logmode = doc["gpio"]["led_logmode"];
    config.gpio.led_oilcounter = doc["gpio"]["led_oilcounter"];
    config.gpio.trigger_oilcounter = doc["gpio"]["trigger_oilcounter"];
    config.gpio.km271_RX = doc["gpio"]["km271_RX"];
    config.gpio.km271_TX = doc["gpio"]["km271_TX"];
    
    config.webUI.enable = doc["webUI"]["enable"];

    config.km271.use_hc1 = doc["km271"]["use_hc1"];
    config.km271.use_hc2 = doc["km271"]["use_hc2"];
    config.km271.use_ww = doc["km271"]["use_ww"];
    config.km271.use_alarmMsg = doc["km271"]["use_alarmMsg"];

    config.ip.enable = doc["ip"]["enable"];
    readJSONstring(config.ip.ipaddress, sizeof(config.ip.ipaddress), doc["ip"]["ipaddress"]);
    readJSONstring(config.ip.subnet, sizeof(config.ip.subnet), doc["ip"]["subnet"]);
    readJSONstring(config.ip.gateway, sizeof(config.ip.gateway), doc["ip"]["gateway"]);
    readJSONstring(config.ip.dns, sizeof(config.ip.dns), doc["ip"]["dns"]);

    config.auth.enable = doc["auth"]["enable"];
    readJSONstring(config.auth.user, sizeof(config.auth.user), doc["auth"]["user"]);
    readJSONstring(config.auth.password, sizeof(config.auth.password), doc["auth"]["password"]);

    config.debug.enable = doc["debug"]["enable"];
    readJSONstring(config.debug.filter, sizeof(config.debug.filter), doc["debug"]["filter"]);
    if (strlen(config.debug.filter) == 0){
      strcpy(config.debug.filter, "XX_XX_XX_XX_XX_XX_XX_XX_XX_XX_XX");
    }

    config.sensor.ch1_enable = doc["sensor"]["ch1_enable"];
    readJSONstring(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), doc["sensor"]["ch1_name"]);
    config.sensor.ch1_gpio = doc["sensor"]["ch1_gpio"];
    config.sensor.ch2_enable = doc["sensor"]["ch2_enable"];
    readJSONstring(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), doc["sensor"]["ch2_name"]);
    config.sensor.ch2_gpio = doc["sensor"]["ch2_gpio"];

    config.pushover.enable = doc["pushover"]["enable"];
    readJSONstring(config.pushover.token, sizeof(config.pushover.token), doc["pushover"]["token"]);     
    readJSONstring(config.pushover.user_key, sizeof(config.pushover.user_key), doc["pushover"]["user_key"]);
    config.pushover.filter = doc["pushover"]["filter"];

    config.log.enable = doc["logger"]["enable"];
    config.log.filter = doc["logger"]["filter"];

  }
  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

