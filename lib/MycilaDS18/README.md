# MycilaDS18

[![Latest Release](https://img.shields.io/github/release/mathieucarbou/MycilaDS18.svg)](https://GitHub.com/mathieucarbou/MycilaDS18/releases/)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/MycilaDS18.svg)](https://registry.platformio.org/libraries/mathieucarbou/MycilaDS18)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](code_of_conduct.md)

[![Build](https://github.com/mathieucarbou/MycilaDS18/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/MycilaDS18/actions/workflows/ci.yml)
[![GitHub latest commit](https://badgen.net/github/last-commit/mathieucarbou/MycilaDS18)](https://GitHub.com/mathieucarbou/MycilaDS18/commit/)
[![Gitpod Ready-to-Code](https://img.shields.io/badge/Gitpod-Ready--to--Code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/mathieucarbou/MycilaDS18)

ESP32 / Arduino Library for Dallas / Maxim DS18 sensors using RMT peripheral

- This library is using the implementation from https://github.com/junkfix/esp32-ds18b20 based on EP32 RMT peripheral to read the temperature from DS18 sensors.
- It is compatible with Arduino 2 and 3
- It is compatible with the DS18B20 sensor
- Non-blocking and callback support
- Support value expiration

## Usage

```c++
#include <MycilaDS18.h>

Mycila::DS18 temp;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  temp.begin(18);

  // Listen to temperature changes in a callback
  temp.listen([](float temperature) {
    Serial.printf("Temperature: %.2f\n", temperature);
  });
}

void loop() {
  // non blocking read
  if (!temp.read()) {
    Serial.println("Not ready yet");
  } else {
    Serial.printf("Temperature: %s\n", std::to_string(temp.getTemperature().value_or(0)).c_str());
  }
  delay(500);
}
```
