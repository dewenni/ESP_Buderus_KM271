#include <basics.h>
#include <config.h>

/* D E C L A R A T I O N S ****************************************************/

const int CFG_VERSION = 1;

char filename[24] = {"/config.json"};
bool setupMode;
bool configInitDone = false;
unsigned long hashOld;
s_config config;
muTimer checkTimer = muTimer(); // timer to refresh other values
static const char *TAG = "CFG"; // LOG TAG
char encrypted[256] = {0};
char decrypted[128] = {0};
const unsigned char key[16] = {0x6d, 0x79, 0x5f, 0x73, 0x65, 0x63, 0x75, 0x72, 0x65, 0x5f, 0x6b, 0x65, 0x79, 0x31, 0x32, 0x33};

/* P R O T O T Y P E S ********************************************************/
void configGPIO();
void configInitValue();
void checkGPIO();

/**
 * *******************************************************************
 * @brief   Setup for intitial configuration
 * @param   none
 * @return  none
 * *******************************************************************/
void configSetup() {

  // start Filesystem
  if (LittleFS.begin(true)) {
    ESP_LOGI(TAG, "LittleFS successfully started");
  } else {
    ESP_LOGE(TAG, "LittleFS error");
  }

  // load config from file
  configLoadFromFile();

  // check GPIO
  checkGPIO();

  // gpio settings
  configGPIO();
}

/**
 * *******************************************************************
 * @brief   check configured gpio
 * @param   none
 * @return  none
 * *******************************************************************/
#define MAX_GPIO 20
void checkGPIO() {
  short int usedGPIOs[MAX_GPIO];
  short int usedCount = 0;

  auto isDuplicate = [&usedGPIOs, &usedCount](int gpio) {
    if (gpio == -1)
      return false; // -1 ignore
    for (int i = 0; i < usedCount; ++i) {
      if (usedGPIOs[i] == gpio) {
        return true;
      }
    }
    if (usedCount < MAX_GPIO - 1) {
      usedGPIOs[usedCount++] = gpio;
    }
    return false;
  };

  bool invalidKM271 = false;
  bool invalidETH = false;

  if (config.gpio.km271_RX == 0) {
    config.gpio.km271_RX = -1;
    invalidKM271 = true;
  } else if (isDuplicate(config.gpio.km271_RX)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (KM271_RX)", config.gpio.km271_RX);
  }

  if (config.gpio.km271_TX == 0) {
    config.gpio.km271_TX = -1;
    invalidKM271 = true;
  } else if (isDuplicate(config.gpio.km271_TX)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (KM271_TX)", config.gpio.km271_TX);
  }

  if (config.gpio.led_heartbeat == 0) {
    config.gpio.led_heartbeat = -1;
  } else if (isDuplicate(config.gpio.led_heartbeat)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (led_heartbeat)", config.gpio.led_heartbeat);
  }

  if (config.gpio.led_logmode == 0) {
    config.gpio.led_logmode = -1;
  } else if (isDuplicate(config.gpio.led_logmode)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (led_logmode)", config.gpio.led_logmode);
  }

  if (config.gpio.led_oilcounter == 0) {
    config.gpio.led_oilcounter = -1;
  } else if (isDuplicate(config.gpio.led_oilcounter)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (led_oilcounter)", config.gpio.led_oilcounter);
  }

  if (config.gpio.led_wifi == 0) {
    config.gpio.led_wifi = -1;
  } else if (isDuplicate(config.gpio.led_wifi)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (led_wifi)", config.gpio.led_wifi);
  }

  if (config.gpio.trigger_oilcounter == 0) {
    config.gpio.trigger_oilcounter = -1;
  } else if (isDuplicate(config.gpio.trigger_oilcounter)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (trigger_oilcounter)", config.gpio.trigger_oilcounter);
  }

  if (config.eth.gpio_cs == 0) {
    config.eth.gpio_cs = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_cs)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_cs)", config.eth.gpio_cs);
  }

  if (config.eth.gpio_irq == 0) {
    config.eth.gpio_irq = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_irq)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_irq)", config.eth.gpio_irq);
  }

  if (config.eth.gpio_miso == 0) {
    config.eth.gpio_miso = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_miso)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_miso)", config.eth.gpio_miso);
  }

  if (config.eth.gpio_mosi == 0) {
    config.eth.gpio_mosi = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_mosi)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_mosi)", config.eth.gpio_mosi);
  }

  if (config.eth.gpio_rst == 0) {
    config.eth.gpio_rst = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_rst)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_rst)", config.eth.gpio_rst);
  }

  if (config.eth.gpio_sck == 0) {
    config.eth.gpio_sck = -1;
    invalidETH = true;
  } else if (isDuplicate(config.eth.gpio_sck)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (eth.gpio_sck)", config.eth.gpio_sck);
  }

  if (config.sensor.ch1_gpio == 0) {
    config.sensor.ch1_gpio = -1;
    if (config.sensor.ch1_enable) {
      ESP_LOGE(TAG, "invalid GPIO settings for Sensor 1");
    }
  } else if (isDuplicate(config.sensor.ch1_gpio)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (Sensor 1)", config.sensor.ch1_gpio);
  }

  if (config.sensor.ch2_gpio == 0) {
    config.sensor.ch2_gpio = -1;
    if (config.sensor.ch1_enable) {
      ESP_LOGE(TAG, "invalid GPIO settings for Sensor 2");
    }
  } else if (isDuplicate(config.sensor.ch2_gpio)) {
    ESP_LOGE(TAG, "GPIO %d is used multiple times (Sensor 2)", config.sensor.ch2_gpio);
  }

  if (config.eth.enable && invalidETH) {
    ESP_LOGE(TAG, "invalid GPIO settings for Ethernet");
  }
  if (invalidKM271) {
    ESP_LOGE(TAG, "invalid GPIO settings for KM271");
  }
}

/**
 * *******************************************************************
 * @brief   init hash value
 * @param   none
 * @return  none
 * *******************************************************************/
void configHashInit() {
  hashOld = EspStrUtil::hash(&config, sizeof(s_config));
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
    unsigned long hashNew = EspStrUtil::hash(&config, sizeof(s_config));
    if (hashNew != hashOld) {
      hashOld = hashNew;
      configSaveToFile();
      ESP_LOGD(TAG, "config saved to file");
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

  doc["version"] = CFG_VERSION;

  doc["lang"] = (config.lang);

  doc["sim"]["enable"] = config.sim.enable;

  doc["oilmeter"]["use_hardware_meter"] = config.oilmeter.use_hardware_meter;
  doc["oilmeter"]["use_virtual_meter"] = config.oilmeter.use_virtual_meter;
  doc["oilmeter"]["consumption_kg_h"] = config.oilmeter.consumption_kg_h;
  doc["oilmeter"]["oil_density_kg_l"] = config.oilmeter.oil_density_kg_l;
  doc["oilmeter"]["pulse_per_liter"] = config.oilmeter.pulse_per_liter;
  doc["oilmeter"]["virtual_calc_offset"] = config.oilmeter.virtual_calc_offset;

  doc["wifi"]["ssid"] = config.wifi.ssid;

  if (EspStrUtil::encryptPassword(config.wifi.password, key, encrypted, sizeof(encrypted))) {
    doc["wifi"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting WiFi Password");
  }

  doc["wifi"]["hostname"] = config.wifi.hostname;
  doc["wifi"]["static_ip"] = config.wifi.static_ip;
  doc["wifi"]["ipaddress"] = config.wifi.ipaddress;
  doc["wifi"]["subnet"] = config.wifi.subnet;
  doc["wifi"]["gateway"] = config.wifi.gateway;
  doc["wifi"]["dns"] = config.wifi.dns;

  doc["eth"]["enable"] = config.eth.enable;
  doc["eth"]["hostname"] = config.eth.hostname;
  doc["eth"]["static_ip"] = config.eth.static_ip;
  doc["eth"]["ipaddress"] = config.eth.ipaddress;
  doc["eth"]["subnet"] = config.eth.subnet;
  doc["eth"]["gateway"] = config.eth.gateway;
  doc["eth"]["dns"] = config.eth.dns;
  doc["eth"]["gpio_sck"] = config.eth.gpio_sck;
  doc["eth"]["gpio_mosi"] = config.eth.gpio_mosi;
  doc["eth"]["gpio_miso"] = config.eth.gpio_miso;
  doc["eth"]["gpio_cs"] = config.eth.gpio_cs;
  doc["eth"]["gpio_irq"] = config.eth.gpio_irq;
  doc["eth"]["gpio_rst"] = config.eth.gpio_rst;

  doc["mqtt"]["enable"] = config.mqtt.enable;
  doc["mqtt"]["server"] = config.mqtt.server;
  doc["mqtt"]["user"] = config.mqtt.user;

  if (EspStrUtil::encryptPassword(config.mqtt.password, key, encrypted, sizeof(encrypted))) {
    doc["mqtt"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting mqtt Password");
  }

  doc["mqtt"]["topic"] = config.mqtt.topic;
  doc["mqtt"]["port"] = config.mqtt.port;
  doc["mqtt"]["config_retain"] = config.mqtt.config_retain;
  doc["mqtt"]["language"] = config.mqtt.lang;
  doc["mqtt"]["cyclic_send"] = config.mqtt.cyclicSendMin;
  doc["mqtt"]["ha_enable"] = config.mqtt.ha_enable;
  doc["mqtt"]["ha_topic"] = config.mqtt.ha_topic;
  doc["mqtt"]["ha_device"] = config.mqtt.ha_device;

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

  doc["km271"]["use_hc1"] = config.km271.use_hc1;
  doc["km271"]["use_hc2"] = config.km271.use_hc2;
  doc["km271"]["use_ww"] = config.km271.use_ww;
  doc["km271"]["use_solar"] = config.km271.use_solar;
  doc["km271"]["use_alarmMsg"] = config.km271.use_alarmMsg;

  doc["auth"]["enable"] = config.auth.enable;
  doc["auth"]["user"] = config.auth.user;

  if (EspStrUtil::encryptPassword(config.auth.password, key, encrypted, sizeof(encrypted))) {
    doc["auth"]["password"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting auth Password");
  }

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

  if (EspStrUtil::encryptPassword(config.pushover.token, key, encrypted, sizeof(encrypted))) {
    doc["pushover"]["token"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting pushover token");
  }

  if (EspStrUtil::encryptPassword(config.pushover.user_key, key, encrypted, sizeof(encrypted))) {
    doc["pushover"]["user_key"] = encrypted;
  } else {
    ESP_LOGE(TAG, "error encrypting pushover user_key");
  }

  doc["pushover"]["filter"] = config.pushover.filter;

  doc["logger"]["enable"] = config.log.enable;
  doc["logger"]["filter"] = config.log.filter;
  doc["logger"]["order"] = config.log.order;

  // Delete existing file, otherwise the configuration is appended to the file
  LittleFS.remove(filename);

  // Open file for writing
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to create file");
    return;
  }

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    ESP_LOGE(TAG, "Failed to write to file");
  } else {
    ESP_LOGI(TAG, "config successfully saved to file: %s - Version: %i", filename, CFG_VERSION);
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
    ESP_LOGE(TAG, "Failed to read file, using default configuration and start wifi-AP");
    configInitValue();
    setupMode = true;

  } else {
    // Copy values from the JsonDocument to the Config structure

    config.version = doc["version"];

    config.oilmeter.use_hardware_meter = doc["oilmeter"]["use_hardware_meter"];
    config.oilmeter.use_virtual_meter = doc["oilmeter"]["use_virtual_meter"];
    config.oilmeter.consumption_kg_h = doc["oilmeter"]["consumption_kg_h"];
    config.oilmeter.oil_density_kg_l = doc["oilmeter"]["oil_density_kg_l"];
    config.oilmeter.pulse_per_liter = doc["oilmeter"]["pulse_per_liter"];
    config.oilmeter.virtual_calc_offset = doc["oilmeter"]["virtual_calc_offset"];

    config.lang = doc["lang"];

    config.sim.enable = doc["sim"]["enable"];

    EspStrUtil::readJSONstring(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["wifi"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.wifi.password, sizeof(config.wifi.password))) {
        // ESP_LOGD(TAG, "decrypted WiFi password: %s", config.wifi.password);
      } else {
        ESP_LOGE(TAG, "error decrypting WiFi password");
      }
    }

    EspStrUtil::readJSONstring(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
    config.wifi.static_ip = doc["wifi"]["static_ip"];
    EspStrUtil::readJSONstring(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), doc["wifi"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.wifi.subnet, sizeof(config.wifi.subnet), doc["wifi"]["subnet"]);
    EspStrUtil::readJSONstring(config.wifi.gateway, sizeof(config.wifi.gateway), doc["wifi"]["gateway"]);
    EspStrUtil::readJSONstring(config.wifi.dns, sizeof(config.wifi.dns), doc["wifi"]["dns"]);

    config.eth.enable = doc["eth"]["enable"];
    EspStrUtil::readJSONstring(config.eth.hostname, sizeof(config.eth.hostname), doc["eth"]["hostname"]);
    config.eth.static_ip = doc["eth"]["static_ip"];
    EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.eth.subnet, sizeof(config.eth.subnet), doc["eth"]["subnet"]);
    EspStrUtil::readJSONstring(config.eth.gateway, sizeof(config.eth.gateway), doc["eth"]["gateway"]);
    EspStrUtil::readJSONstring(config.eth.dns, sizeof(config.eth.dns), doc["eth"]["dns"]);
    config.eth.gpio_sck = doc["eth"]["gpio_sck"];
    config.eth.gpio_mosi = doc["eth"]["gpio_mosi"];
    config.eth.gpio_miso = doc["eth"]["gpio_miso"];
    config.eth.gpio_cs = doc["eth"]["gpio_cs"];
    config.eth.gpio_irq = doc["eth"]["gpio_irq"];
    config.eth.gpio_rst = doc["eth"]["gpio_rst"];

    config.mqtt.enable = doc["mqtt"]["enable"];
    EspStrUtil::readJSONstring(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
    EspStrUtil::readJSONstring(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["mqtt"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.mqtt.password, sizeof(config.mqtt.password))) {
        // ESP_LOGD(TAG, "decrypted mqtt password: %s", config.mqtt.password);
      } else {
        ESP_LOGE(TAG, "error decrypting mqtt password");
      }
    }

    EspStrUtil::readJSONstring(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
    config.mqtt.port = doc["mqtt"]["port"];
    config.mqtt.config_retain = doc["mqtt"]["config_retain"];
    config.mqtt.lang = doc["mqtt"]["language"];
    config.mqtt.cyclicSendMin = doc["mqtt"]["cyclic_send"];
    config.mqtt.ha_enable = doc["mqtt"]["ha_enable"];
    EspStrUtil::readJSONstring(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), doc["mqtt"]["ha_topic"]);
    EspStrUtil::readJSONstring(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), doc["mqtt"]["ha_device"]);

    config.ntp.enable = doc["ntp"]["enable"];
    EspStrUtil::readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    EspStrUtil::readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);
    config.ntp.auto_sync = doc["ntp"]["auto_sync"];

    config.gpio.led_wifi = doc["gpio"]["led_wifi"];
    config.gpio.led_heartbeat = doc["gpio"]["led_heartbeat"];
    config.gpio.led_logmode = doc["gpio"]["led_logmode"];
    config.gpio.led_oilcounter = doc["gpio"]["led_oilcounter"];
    config.gpio.trigger_oilcounter = doc["gpio"]["trigger_oilcounter"];
    config.gpio.km271_RX = doc["gpio"]["km271_RX"];
    config.gpio.km271_TX = doc["gpio"]["km271_TX"];

    config.km271.use_hc1 = doc["km271"]["use_hc1"];
    config.km271.use_hc2 = doc["km271"]["use_hc2"];
    config.km271.use_ww = doc["km271"]["use_ww"];
    config.km271.use_solar = doc["km271"]["use_solar"];
    config.km271.use_alarmMsg = doc["km271"]["use_alarmMsg"];

    config.auth.enable = doc["auth"]["enable"];
    EspStrUtil::readJSONstring(config.auth.user, sizeof(config.auth.user), doc["auth"]["user"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.auth.password, sizeof(config.auth.password), doc["auth"]["password"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["auth"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.auth.password, sizeof(config.auth.password))) {
        // ESP_LOGD(TAG, "decrypted auth password: %s", config.auth.password);
      } else {
        ESP_LOGE(TAG, "error decrypting auth password");
      }
    }

    config.debug.enable = doc["debug"]["enable"];
    EspStrUtil::readJSONstring(config.debug.filter, sizeof(config.debug.filter), doc["debug"]["filter"]);
    if (strlen(config.debug.filter) == 0) {
      strcpy(config.debug.filter, "XX_XX_XX_XX_XX_XX_XX_XX_XX_XX_XX");
    }

    config.sensor.ch1_enable = doc["sensor"]["ch1_enable"];
    EspStrUtil::readJSONstring(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), doc["sensor"]["ch1_name"]);
    EspStrUtil::readJSONstring(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), doc["sensor"]["ch1_description"]);
    config.sensor.ch1_gpio = doc["sensor"]["ch1_gpio"];
    config.sensor.ch2_enable = doc["sensor"]["ch2_enable"];
    EspStrUtil::readJSONstring(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), doc["sensor"]["ch2_name"]);
    EspStrUtil::readJSONstring(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), doc["sensor"]["ch2_description"]);
    config.sensor.ch2_gpio = doc["sensor"]["ch2_gpio"];

    config.pushover.enable = doc["pushover"]["enable"];
    config.pushover.filter = doc["pushover"]["filter"];

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.pushover.token, sizeof(config.pushover.token), doc["pushover"]["token"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["pushover"]["token"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.pushover.token, sizeof(config.pushover.token))) {
        // ESP_LOGD(TAG, "decrypted pushover token: %s", config.pushover.token);
      } else {
        ESP_LOGE(TAG, "error decrypting pushover token");
      }
    }

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.pushover.user_key, sizeof(config.pushover.user_key), doc["pushover"]["user_key"]);
    } else {
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["pushover"]["user_key"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.pushover.user_key, sizeof(config.pushover.user_key))) {
        // ESP_LOGD(TAG, "decrypted pushover user_key: %s", config.pushover.user_key);
      } else {
        ESP_LOGE(TAG, "error decrypting pushover user_key");
      }
    }

    config.log.enable = doc["logger"]["enable"];
    config.log.filter = doc["logger"]["filter"];
    config.log.order = doc["logger"]["order"];
  }

  if (strlen(config.wifi.ssid) == 0) {
    // no valid wifi setting => start AP-Mode
    ESP_LOGW(TAG, "no valid wifi SSID set => enter SetupMode and start AP-Mode");
    setupMode = true;
  }

  if (strlen(config.mqtt.ha_topic) == 0) {
    snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");
  }
  if (strlen(config.mqtt.ha_device) == 0) {
    snprintf(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), "Logamatic");
  }

  file.close();     // Close the file (Curiously, File's destructor doesn't close the file)
  configHashInit(); // init hash value

  // save config if version is different
  if (config.version != CFG_VERSION) {
    configSaveToFile();
    ESP_LOGI(TAG, "config file was updated from version %i to version: %i", config.version, CFG_VERSION);
  } else {
    ESP_LOGI(TAG, "config file version %i was successfully loaded", config.version);
  }
}
