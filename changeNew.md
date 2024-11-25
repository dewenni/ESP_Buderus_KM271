# v5.0.1

## what's new

This is a minor update version, but it is based on a major update 5.0.0
Please see here what´s changed in [release v5.0.0](https://github.com/dewenni/ESP_Buderus_KM271/releases/tag/v5.0.0)

## changelog

- change mqtt client-id back to WiFi-Hostname #128
- No restart of the ESP if the WiFi connection cannot be established but there is an Ethernet connection.
- Interval for renewed mqtt connection attempt increases with each attempt by a further 10s (max. 50s). The ESP restarts after 5 failed attempts
- change previous used Libraries for DS18B20 sensors to [mathieucarbou/MycilaDS18](https://github.com/mathieucarbou/MycilaDS18)
- add function to check whether the configured GPIOs are valid and have not been assigned twice
- update Readme.md