#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <basics.h>
#include <config.h>

/* D E C L A R A T I O N S ****************************************************/
char filename[24] = {"/config.json"};
bool setupMode;
bool configInitDone = false;
unsigned long hashOld;
s_config config;
muTimer checkTimer = muTimer(); // timer to refresh other values

/* P R O T O T Y P E S ********************************************************/
void configGPIO();
void configInitValue();

/**
 * *******************************************************************
 * @brief   hash djb2 function
 * @param   str, len
 * @return  none
 * *******************************************************************/
unsigned long hash(void *str, size_t len) {
  unsigned char *p = (unsigned char *)str;
  unsigned long hash = 5381;
  size_t i;
  for (i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + p[i]; /* hash * 33 + c */
  }
  return hash;
}

/**
 * *******************************************************************
 * @brief   check before read from file
 * @param   dest, size, src
 * @return  none
 * *******************************************************************/
void readJSONstring(char *dest, size_t size, const char *src) {
  const char *check = src;
  if (check != NULL) {
    snprintf(dest, size, "%s", src);
  }
}

/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup() {

  // start Filesystem
  if (LittleFS.begin(true)) {
    msgLn("LittleFS successfully started");
  } else {
    msgLn("LittleFS error");
  }

  // load config from file
  configLoadFromFile();
  // gpio settings
  configGPIO();
}

/**
 * *******************************************************************
 * @brief   init hash value
 * @param   none
 * @return  none
 * *******************************************************************/
void configHashInit() {
  hashOld = hash(&config, sizeof(s_config));
  configInitDone = true;
}

/**
 * *******************************************************************
 * @brief   cyclic function for configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configCyclic() {

  if (checkTimer.cycleTrigger(1000) && configInitDone) {
    unsigned long hashNew = hash(&config, sizeof(s_config));
    if (hashNew != hashOld) {
      hashOld = hashNew;
      configSaveToFile();
    }
  }
}

/**
 * *******************************************************************
 * @brief   Setup for GPIO
 * @param   none
 * @return  none
 * *******************************************************************/
void configGPIO() {

  if (setupMode) {
    pinMode(LED_BUILTIN, OUTPUT); // onboard LED
    pinMode(21, OUTPUT);          // green LED D1 on the78mole boards
  } else {
    if (config.gpio.led_wifi != -1)
      pinMode(config.gpio.led_wifi, OUTPUT); // LED for Wifi-Status
    if (config.gpio.led_heartbeat != -1)
      pinMode(config.gpio.led_heartbeat, OUTPUT); // LED for heartbeat
    if (config.gpio.led_logmode != -1)
      pinMode(config.gpio.led_logmode, OUTPUT); // LED for LogMode-Status

    if (config.oilmeter.use_hardware_meter) {
      if (config.gpio.trigger_oilcounter != -1)
        pinMode(config.gpio.trigger_oilcounter, INPUT_PULLUP); // Trigger Input
      if (config.gpio.led_oilcounter != -1)
        pinMode(config.gpio.led_oilcounter, OUTPUT); // Status LED
    }
  }
}

/**
 * *******************************************************************
 * @brief   intitial configuration values
 * @param   none
 * @return  none
 * *******************************************************************/
void configInitValue() {

  memset((void *)&config, 0, sizeof(config));

  // WiFi
  snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "ESP-Buderus-KM271");

  // MQTT
  config.mqtt.port = 1883;
  config.mqtt.enable = false;
  snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");

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
}

/**
 * *******************************************************************
 * @brief   save configuration to file
 * @param   none
 * @return  none
 * *******************************************************************/
void configSaveToFile() {

  JsonDocument doc; // reserviert 2048 Bytes für das JSON-Objekt

  doc["lang"] = (config.lang);

  doc["sim"]["enable"] = config.sim.enable;

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
  doc["mqtt"]["language"] = config.mqtt.lang;
  doc["mqtt"]["ha_enable"] = config.mqtt.ha_enable;
  doc["mqtt"]["ha_topic"] = config.mqtt.ha_topic;

  doc["ntp"]["enable"] = config.ntp.enable;
  doc["ntp"]["server"] = config.ntp.server;
  doc["ntp"]["tz"] = config.ntp.tz;
  doc["ntp"]["auto_sync"] = config.ntp.auto_sync;

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
  doc["sensor"]["ch1_description"] = config.sensor.ch1_description;
  doc["sensor"]["ch1_gpio"] = config.sensor.ch1_gpio;
  doc["sensor"]["ch2_enable"] = config.sensor.ch2_enable;
  doc["sensor"]["ch2_name"] = config.sensor.ch2_name;
  doc["sensor"]["ch2_description"] = config.sensor.ch2_description;
  doc["sensor"]["ch2_gpio"] = config.sensor.ch2_gpio;

  doc["pushover"]["enable"] = config.pushover.enable;
  doc["pushover"]["token"] = config.pushover.token;
  doc["pushover"]["user_key"] = config.pushover.user_key;
  doc["pushover"]["filter"] = config.pushover.filter;

  doc["logger"]["enable"] = config.log.enable;
  doc["logger"]["filter"] = config.log.filter;
  doc["logger"]["order"] = config.log.order;

  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    msgLn("Failed to create file");
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    msgLn("Failed to write to file");
  } else {
    msg("config successfully saved to file: ");
    msgLn(filename);
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
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    msgLn("Failed to read file, using default configuration and start wifi-AP");
    configInitValue();
    setupMode = true;

  } else {
    // Copy values from the JsonDocument to the Config structure
    config.oilmeter.use_hardware_meter = doc["oilmeter"]["use_hardware_meter"];
    config.oilmeter.use_virtual_meter = doc["oilmeter"]["use_virtual_meter"];
    config.oilmeter.consumption_kg_h = doc["oilmeter"]["consumption_kg_h"];
    config.oilmeter.oil_density_kg_l = doc["oilmeter"]["oil_density_kg_l"];

    config.lang = doc["lang"];

    config.sim.enable = doc["sim"]["enable"];

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
    config.mqtt.lang = doc["mqtt"]["language"];
    config.mqtt.ha_enable = doc["mqtt"]["ha_enable"];
    readJSONstring(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), doc["mqtt"]["ha_topic"]);

    config.ntp.enable = doc["ntp"]["enable"];
    readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);
    config.ntp.auto_sync = doc["ntp"]["auto_sync"];

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
    if (strlen(config.debug.filter) == 0) {
      strcpy(config.debug.filter, "XX_XX_XX_XX_XX_XX_XX_XX_XX_XX_XX");
    }

    config.sensor.ch1_enable = doc["sensor"]["ch1_enable"];
    readJSONstring(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), doc["sensor"]["ch1_name"]);
    readJSONstring(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), doc["sensor"]["ch1_description"]);
    config.sensor.ch1_gpio = doc["sensor"]["ch1_gpio"];
    config.sensor.ch2_enable = doc["sensor"]["ch2_enable"];
    readJSONstring(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), doc["sensor"]["ch2_name"]);
    readJSONstring(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), doc["sensor"]["ch2_description"]);
    config.sensor.ch2_gpio = doc["sensor"]["ch2_gpio"];

    config.pushover.enable = doc["pushover"]["enable"];
    readJSONstring(config.pushover.token, sizeof(config.pushover.token), doc["pushover"]["token"]);
    readJSONstring(config.pushover.user_key, sizeof(config.pushover.user_key), doc["pushover"]["user_key"]);
    config.pushover.filter = doc["pushover"]["filter"];

    config.log.enable = doc["logger"]["enable"];
    config.log.filter = doc["logger"]["filter"];
    config.log.order = doc["logger"]["order"];
  }

  if (strlen(config.wifi.ssid) == 0) {
    // no valid wifi setting => start AP-Mode
    setupMode = true;
  }

  if (strlen(config.mqtt.ha_topic) == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");
  }

  file.close();     // Close the file (Curiously, File's destructor doesn't close the file)
  configHashInit(); // init hash value
}

/**
 * *******************************************************************
 * @brief   save restart reason to file
 * @param   reason
 * @return  none
 * *******************************************************************/
void saveRestartReason(const char *reason) {
  File file = LittleFS.open("/restart_reason.txt", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.println(reason);
  file.close();
}

/**
 * *******************************************************************
 * @brief   read restart reason from file
 * @param   buffer
 * @param   bufferSize
 * @return  none
 * *******************************************************************/
bool readRestartReason(char *buffer, size_t bufferSize) {
  if (!LittleFS.exists("/restart_reason.txt")) {
    return false; // no file
  }

  File file = LittleFS.open("/restart_reason.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  bool result = false;
  if (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    strncpy(buffer, line.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    result = true;
  }
  file.close();

  LittleFS.remove("/restart_reason.txt");

  return result;
}