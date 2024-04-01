## ESP_DoubleResetDetector

[![arduino-library-badge](https://www.ardu-badge.com/badge/ESP_DoubleResetDetector.svg?)](https://www.ardu-badge.com/ESP_DoubleResetDetector)
[![GitHub release](https://img.shields.io/github/release/khoih-prog/ESP_DoubleResetDetector.svg)](https://github.com/khoih-prog/ESP_DoubleResetDetector/releases)
[![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/khoih-prog/ESP_DoubleResetDetector/blob/master/LICENSE)
[![contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](#Contributing)
[![GitHub issues](https://img.shields.io/github/issues/khoih-prog/ESP_DoubleResetDetector.svg)](http://github.com/khoih-prog/ESP_DoubleResetDetector/issues)

---
---

## Table of Contents

* [Changelog](#changelog)
  * [Releases v1.3.2](#releases-v132)
  * [Releases v1.3.1](#releases-v131)
  * [Releases v1.3.0](#releases-v130)
  * [Releases v1.2.1](#releases-v121)
  * [Releases v1.2.0](#releases-v120)
  * [Releases v1.1.2](#releases-v112)
  * [Releases v1.1.1](#releases-v111)
  * [Releases v1.1.0](#releases-v110)
  * [Releases v1.0.3](#releases-v103)
  * [Releases v1.0.2](#releases-v102)
  * [Releases v1.0.1](#releases-v101)
 
---
---

## Changelog

### Releases v1.3.2

1. Fix ESP32 chipID for example `ConfigOnDoubleReset`
2. Remove dependency on `LittleFS_esp32` library to prevent PIO error when using new ESP32 core v1.0.6+

### Releases v1.3.1

1. Add waitingForDRD() function to signal in DRD wating period. Check [DRD is waiting for a double reset? #14](https://github.com/khoih-prog/ESP_DoubleResetDetector/discussions/14)
2. Add example [checkWaitingDRD](https://github.com/khoih-prog/ESP_DoubleResetDetector/tree/master/examples/checkWaitingDRD) to demo how to use the new feature.

### Releases v1.3.0

1. Add support to `ESP32_S3` using [ESP32 core, esp32-s3-support branch, v2.0.2+](https://github.com/espressif/arduino-esp32/tree/esp32-s3-support)

### Releases v1.2.1

1. Fix compile error for ESP32 core v1.0.5-

### Releases v1.2.0

1. Auto detect ESP32 core and use either built-in LittleFS or [LITTLEFS](https://github.com/lorol/LITTLEFS) library
2. Update `library.json` to use new `headers` for PIO

### Releases v1.1.2

1. Update `platform.ini` and `library.json` to use original `khoih-prog` instead of `khoih.prog` after PIO fix

### Releases v1.1.1

1. Clean-up all compiler warnings possible.
2. Add Table of Contents
3. Modify Version String

### Releases v1.1.0

1. Add support to LittleFS for ESP32 using [LITTLEFS](https://github.com/lorol/LITTLEFS) Library

### Releases v1.0.3

1. Update to use the new LittleFS for ESP8266 core 2.7.1+
2. Update [minimal example](examples/minimal)

#### Releases v1.0.2

1. Fix bug by left-over cpp file.
2. Fix bug in example.
3. Enhance README.md

#### Releases v1.0.1

1. Add EEPROM and SPIFFS support, besides RTC memory, for ESP8266
2. Add SPIFFS support, besides EEPROM, for ESP32

