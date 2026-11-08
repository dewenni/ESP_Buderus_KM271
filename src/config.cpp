#include <basics.h>
#include <config.h>
#include <message.h>

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
void configFinalCheck();

// Fallback for every remote entry point that can flash or reconfigure the
// device, used only until config.auth.password is set. It is compiled into a
// public firmware, so it is not a secret - CHANGE IT via the web UI's Auth
// settings after first login.
static const char *DEFAULT_PASSWORD = "km271-2026!";

const char *devicePassword() { return (strlen(config.auth.password) > 0) ? config.auth.password : DEFAULT_PASSWORD; }

/**
 * *******************************************************************
 * @brief   Load a scalar config field from JSON, leaving the existing
 *          value (in-class default, or whatever was already loaded)
 *          untouched if the key is absent - a missing key must never
 *          silently zero a field that was never meant to change.
 * @param   target  reference to the config struct field to update
 * @param   value   the corresponding doc["section"]["field"] lookup
 * @return  none
 * *******************************************************************/
template <typename T> void loadCfg(T &target, JsonVariantConst value) {
  if (!value.isNull()) {
    target = value.as<T>();
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

  configFinalCheck();
}

void configFinalCheck() {

  // Clamp every field that is later used as an array index. The web UI
  // callbacks validate these, but a hand-edited or uploaded config.json does
  // not go through them - and config.lang indexes arrays of exactly MAX_LANG
  // entries, while config.log.filter indexes LOG_FILTER[][6]. An out-of-range
  // value read from the file would otherwise be dereferenced as a wild
  // pointer on the next web UI page load or log write.
  config.lang = constrain(config.lang, 0, MAX_LANG - 1);
  config.mqtt.lang = constrain(config.mqtt.lang, 0, MAX_LANG - 1);
  config.log.filter = constrain(config.log.filter, 0, LOG_FILTER_SYSTEM);

  // set log level
  setLogLevel(config.log.level);

  if (config.oilmeter.use_hardware_meter && config.oilmeter.debounce_time == 0) {
    config.oilmeter.debounce_time = 500;
  }
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

  // Assigning a value-initialised instance restores every default declared
  // in config.h. This used to be a memset() to zero, which silently discarded
  // all of them - most seriously auth.enable = true, so "config reset" (telnet)
  // left the device with web authentication switched OFF and wrote that to
  // flash. The oilmeter defaults (2.0 kg/h, 0.85 kg/l, 50 pulses, 500 ms) and
  // ntp/log/debug enable flags were lost the same way. Doing it this way also
  // means defaults added to config.h later can no longer be missed here.
  config = s_config{};

  // fields that have no in-class default
  snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), "ESP-Buderus-KM271");
  snprintf(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), "homeassistant");

  // gpio: -1 means "unused", which is not the value-initialised 0
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
  doc["oilmeter"]["virt_calc_offset"] = config.oilmeter.virt_calc_offset;
  doc["oilmeter"]["debounce_time"] = config.oilmeter.debounce_time;

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
  doc["logger"]["level"] = config.log.level;

  // Write to a temporary file first, then rename over the real one. The
  // previous code removed config.json and then re-created it, which left a
  // window where the file was absent or truncated. A power cut in that window
  // (and configCyclic() saves on any change, so it is not a rare moment) meant
  // the next boot failed to parse the config, fell back to defaults, and came
  // up as an access point with no WiFi credentials - unrecoverable without
  // physical access to the device.
  const char *tmpFilename = "/config.json.tmp";

  LittleFS.remove(tmpFilename); // leftover from an interrupted earlier attempt

  File file = LittleFS.open(tmpFilename, FILE_WRITE);
  if (!file) {
    ESP_LOGE(TAG, "Failed to create temporary config file");
    return;
  }

  size_t written = serializeJson(doc, file);
  file.close();

  if (written == 0) {
    ESP_LOGE(TAG, "Failed to write config - keeping the previous file");
    LittleFS.remove(tmpFilename);
    return;
  }

  // rename() cannot overwrite on LittleFS, so the old file has to go first.
  // The window between these two calls is the one remaining exposure, but
  // unlike before, a crash there leaves a complete config.json.tmp behind -
  // which configLoadFromFile() recovers from on the next boot.
  LittleFS.remove(filename);
  if (!LittleFS.rename(tmpFilename, filename)) {
    ESP_LOGE(TAG, "Failed to rename %s to %s", tmpFilename, filename);
    return;
  }

  ESP_LOGI(TAG, "config successfully saved to file: %s - Version: %i", filename, CFG_VERSION);
}

/**
 * *******************************************************************
 * @brief   load configuration from file
 * @param   none
 * @return  none
 * *******************************************************************/
void configLoadFromFile() {
  const char *tmpFilename = "/config.json.tmp";

  // Open file for reading
  File file = LittleFS.open(filename);

  // Allocate a temporary JsonDocument
  JsonDocument doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);

  // If the main file is missing or unparseable, try the temporary file left
  // behind by configSaveToFile(). A crash between the remove() and the
  // rename() there leaves a complete, valid config there - recovering from it
  // is the difference between carrying on normally and coming up as an access
  // point with no WiFi credentials.
  if (error && LittleFS.exists(tmpFilename)) {
    ESP_LOGW(TAG, "%s missing or corrupt - recovering from %s", filename, tmpFilename);
    file.close();
    file = LittleFS.open(tmpFilename);
    error = deserializeJson(doc, file);
    if (!error) {
      file.close();
      LittleFS.remove(filename);
      LittleFS.rename(tmpFilename, filename);
      file = LittleFS.open(filename);
    }
  }

  if (error) {
    ESP_LOGE(TAG, "Failed to read file, using default configuration and start wifi-AP");
    configInitValue();
    setupMode = true;

  } else {
    // Copy values from the JsonDocument to the Config structure

    config.version = doc["version"];

    loadCfg(config.oilmeter.use_hardware_meter, doc["oilmeter"]["use_hardware_meter"]);
    loadCfg(config.oilmeter.use_virtual_meter, doc["oilmeter"]["use_virtual_meter"]);
    loadCfg(config.oilmeter.consumption_kg_h, doc["oilmeter"]["consumption_kg_h"]);
    loadCfg(config.oilmeter.oil_density_kg_l, doc["oilmeter"]["oil_density_kg_l"]);
    loadCfg(config.oilmeter.pulse_per_liter, doc["oilmeter"]["pulse_per_liter"]);
    loadCfg(config.oilmeter.virt_calc_offset, doc["oilmeter"]["virt_calc_offset"]);
    loadCfg(config.oilmeter.debounce_time, doc["oilmeter"]["debounce_time"]);

    loadCfg(config.lang, doc["lang"]);

    loadCfg(config.sim.enable, doc["sim"]["enable"]);

    EspStrUtil::readJSONstring(config.wifi.ssid, sizeof(config.wifi.ssid), doc["wifi"]["ssid"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.wifi.password, sizeof(config.wifi.password), doc["wifi"]["password"]);
    } else {
      memset(encrypted, 0, sizeof(encrypted));
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["wifi"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.wifi.password, sizeof(config.wifi.password))) {
        // ESP_LOGD(TAG, "decrypted WiFi password: %s", config.wifi.password);
      } else {
        ESP_LOGE(TAG, "error decrypting WiFi password");
      }
    }

    EspStrUtil::readJSONstring(config.wifi.hostname, sizeof(config.wifi.hostname), doc["wifi"]["hostname"]);
    loadCfg(config.wifi.static_ip, doc["wifi"]["static_ip"]);
    EspStrUtil::readJSONstring(config.wifi.ipaddress, sizeof(config.wifi.ipaddress), doc["wifi"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.wifi.subnet, sizeof(config.wifi.subnet), doc["wifi"]["subnet"]);
    EspStrUtil::readJSONstring(config.wifi.gateway, sizeof(config.wifi.gateway), doc["wifi"]["gateway"]);
    EspStrUtil::readJSONstring(config.wifi.dns, sizeof(config.wifi.dns), doc["wifi"]["dns"]);

    loadCfg(config.eth.enable, doc["eth"]["enable"]);
    EspStrUtil::readJSONstring(config.eth.hostname, sizeof(config.eth.hostname), doc["eth"]["hostname"]);
    loadCfg(config.eth.static_ip, doc["eth"]["static_ip"]);
    EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.eth.ipaddress, sizeof(config.eth.ipaddress), doc["eth"]["ipaddress"]);
    EspStrUtil::readJSONstring(config.eth.subnet, sizeof(config.eth.subnet), doc["eth"]["subnet"]);
    EspStrUtil::readJSONstring(config.eth.gateway, sizeof(config.eth.gateway), doc["eth"]["gateway"]);
    EspStrUtil::readJSONstring(config.eth.dns, sizeof(config.eth.dns), doc["eth"]["dns"]);
    loadCfg(config.eth.gpio_sck, doc["eth"]["gpio_sck"]);
    loadCfg(config.eth.gpio_mosi, doc["eth"]["gpio_mosi"]);
    loadCfg(config.eth.gpio_miso, doc["eth"]["gpio_miso"]);
    loadCfg(config.eth.gpio_cs, doc["eth"]["gpio_cs"]);
    loadCfg(config.eth.gpio_irq, doc["eth"]["gpio_irq"]);
    loadCfg(config.eth.gpio_rst, doc["eth"]["gpio_rst"]);

    loadCfg(config.mqtt.enable, doc["mqtt"]["enable"]);
    EspStrUtil::readJSONstring(config.mqtt.server, sizeof(config.mqtt.server), doc["mqtt"]["server"]);
    EspStrUtil::readJSONstring(config.mqtt.user, sizeof(config.mqtt.user), doc["mqtt"]["user"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.mqtt.password, sizeof(config.mqtt.password), doc["mqtt"]["password"]);
    } else {
      memset(encrypted, 0, sizeof(encrypted));
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["mqtt"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.mqtt.password, sizeof(config.mqtt.password))) {
        // ESP_LOGD(TAG, "decrypted mqtt password: %s", config.mqtt.password);
      } else {
        ESP_LOGE(TAG, "error decrypting mqtt password");
      }
    }

    EspStrUtil::readJSONstring(config.mqtt.topic, sizeof(config.mqtt.topic), doc["mqtt"]["topic"]);
    loadCfg(config.mqtt.port, doc["mqtt"]["port"]);
    loadCfg(config.mqtt.config_retain, doc["mqtt"]["config_retain"]);
    loadCfg(config.mqtt.lang, doc["mqtt"]["language"]);
    loadCfg(config.mqtt.cyclicSendMin, doc["mqtt"]["cyclic_send"]);
    loadCfg(config.mqtt.ha_enable, doc["mqtt"]["ha_enable"]);
    EspStrUtil::readJSONstring(config.mqtt.ha_topic, sizeof(config.mqtt.ha_topic), doc["mqtt"]["ha_topic"]);
    EspStrUtil::readJSONstring(config.mqtt.ha_device, sizeof(config.mqtt.ha_device), doc["mqtt"]["ha_device"]);

    loadCfg(config.ntp.enable, doc["ntp"]["enable"]);
    EspStrUtil::readJSONstring(config.ntp.server, sizeof(config.ntp.server), doc["ntp"]["server"]);
    EspStrUtil::readJSONstring(config.ntp.tz, sizeof(config.ntp.tz), doc["ntp"]["tz"]);
    loadCfg(config.ntp.auto_sync, doc["ntp"]["auto_sync"]);

    loadCfg(config.gpio.led_wifi, doc["gpio"]["led_wifi"]);
    loadCfg(config.gpio.led_heartbeat, doc["gpio"]["led_heartbeat"]);
    loadCfg(config.gpio.led_logmode, doc["gpio"]["led_logmode"]);
    loadCfg(config.gpio.led_oilcounter, doc["gpio"]["led_oilcounter"]);
    loadCfg(config.gpio.trigger_oilcounter, doc["gpio"]["trigger_oilcounter"]);
    loadCfg(config.gpio.km271_RX, doc["gpio"]["km271_RX"]);
    loadCfg(config.gpio.km271_TX, doc["gpio"]["km271_TX"]);

    loadCfg(config.km271.use_hc1, doc["km271"]["use_hc1"]);
    loadCfg(config.km271.use_hc2, doc["km271"]["use_hc2"]);
    loadCfg(config.km271.use_ww, doc["km271"]["use_ww"]);
    loadCfg(config.km271.use_solar, doc["km271"]["use_solar"]);
    loadCfg(config.km271.use_alarmMsg, doc["km271"]["use_alarmMsg"]);

    loadCfg(config.auth.enable, doc["auth"]["enable"]);
    EspStrUtil::readJSONstring(config.auth.user, sizeof(config.auth.user), doc["auth"]["user"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.auth.password, sizeof(config.auth.password), doc["auth"]["password"]);
    } else {
      memset(encrypted, 0, sizeof(encrypted));
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["auth"]["password"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.auth.password, sizeof(config.auth.password))) {
        // ESP_LOGD(TAG, "decrypted auth password: %s", config.auth.password);
      } else {
        ESP_LOGE(TAG, "error decrypting auth password");
      }
    }

    loadCfg(config.debug.enable, doc["debug"]["enable"]);
    EspStrUtil::readJSONstring(config.debug.filter, sizeof(config.debug.filter), doc["debug"]["filter"]);
    if (strlen(config.debug.filter) == 0) {
      strcpy(config.debug.filter, "XX_XX_XX_XX_XX_XX_XX_XX_XX_XX_XX");
    }

    loadCfg(config.sensor.ch1_enable, doc["sensor"]["ch1_enable"]);
    EspStrUtil::readJSONstring(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), doc["sensor"]["ch1_name"]);
    EspStrUtil::readJSONstring(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), doc["sensor"]["ch1_description"]);
    loadCfg(config.sensor.ch1_gpio, doc["sensor"]["ch1_gpio"]);
    loadCfg(config.sensor.ch2_enable, doc["sensor"]["ch2_enable"]);
    EspStrUtil::readJSONstring(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), doc["sensor"]["ch2_name"]);
    EspStrUtil::readJSONstring(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), doc["sensor"]["ch2_description"]);
    loadCfg(config.sensor.ch2_gpio, doc["sensor"]["ch2_gpio"]);

    loadCfg(config.pushover.enable, doc["pushover"]["enable"]);
    loadCfg(config.pushover.filter, doc["pushover"]["filter"]);

    if (config.version == 0) {
      EspStrUtil::readJSONstring(config.pushover.token, sizeof(config.pushover.token), doc["pushover"]["token"]);
    } else {
      memset(encrypted, 0, sizeof(encrypted));
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
      memset(encrypted, 0, sizeof(encrypted));
      EspStrUtil::readJSONstring(encrypted, sizeof(encrypted), doc["pushover"]["user_key"]);
      if (EspStrUtil::decryptPassword(encrypted, key, config.pushover.user_key, sizeof(config.pushover.user_key))) {
        // ESP_LOGD(TAG, "decrypted pushover user_key: %s", config.pushover.user_key);
      } else {
        ESP_LOGE(TAG, "error decrypting pushover user_key");
      }
    }

    loadCfg(config.log.enable, doc["logger"]["enable"]);
    loadCfg(config.log.filter, doc["logger"]["filter"]);
    loadCfg(config.log.order, doc["logger"]["order"]);
    loadCfg(config.log.level, doc["logger"]["level"]);
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