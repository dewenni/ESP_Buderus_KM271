# v4.2.9002

## what's new

update to the new Arduino Release v3.0.7 based on ESP-IDF v5.1.4+

Unfortunately I have to do some breaking changes!


### Ethernet Support W5500

There is a new support for W5500 Ethernet Modules. This brings the possibility to use ethernet connection instead of WiFi connection.
This could be useful in situations where the WiFi connection is not available or stable.

For mor details, please have a look at the [documentation](https://github.com/dewenni/ESP_Buderus_KM271?tab=readme-ov-file#optional-ethernet-module-w5500) 


### BREAKING CHANGES!!!

> [!IMPORTANT]   
> In the past, there was a separate setting for the static IP. Now we have two possible interfaces and therefore the static IP setting is also duplicated and is located together with the WiFi and Ethernet settings. If you previously used static IP, you will need to adjust your settings.
>
>**I would recommend deactivating the static IP before the update!**


## changelog

- change mqtt Library to [mathieucarbou/MycilaMQTT](https://github.com/mathieucarbou/MycilaMQTT)
- update AsyncTCP library to [mathieucarbou/AsyncTCP](https://github.com/mathieucarbou/AsyncTCP) v3.2.10
- add "Mode: SystemLog" to the WebUI Logger function that shows the ESP_LOG messages
- add mqtt information to webUI
- remove "uptime" from MQTT discovery, because it floods the log #108
- fix typo "Ferhler" in OTA Dialog #108
- clean unused webUI texts
- update Descriptions for Ethernet and Exhaust Sensor in README.md