# v5.0.9000

## what's new

This is a major update with some new features and some redesign under the hood.

The Framework is updated to the new Arduino v3 based on ESP-IDF v5.1+

Unfortunately I have to do some breaking changes!


### Ethernet Support W5500

There is a new support for W5500 Ethernet Modules. This brings the possibility to use ethernet connection instead of WiFi connection.
This could be useful in situations where the WiFi connection is not available or stable.

For mor details, please have a look at the [documentation](https://github.com/dewenni/ESP_Buderus_KM271?tab=readme-ov-file#optional-ethernet-module-w5500) 

### Solar Support FM244

Support for optional Buderus Solar Module FM244 added. You can activate this in the same way as the heating circuits and hot-water.
Values are send by mqtt and will be also available in the webUI.

### System Logger

The logging function has been extended. The mode can now also be switched to ‘System log’. In this case, the serial monitor is then redirected to the log and you can see the messages that are normally only visible when you are connected via cable.


### internal communication between ESP and WebUI changed

The internal communication between ESP and WebUI has changed from "Server Side Events" to Websockets.
The maximum number of client connections is monitored and limited to the number of 2 active connections.

### BREAKING CHANGES!!!

> [!IMPORTANT]   
> In the past, there was a separate setting for the static IP. Now we have two possible interfaces and therefore the static IP setting is also duplicated and is located together with the WiFi and Ethernet settings. If you previously used static IP, you will need to adjust your settings.
>
>**I would recommend deactivating the static IP before the update!**


## changelog

- change mqtt Library to [mathieucarbou/MycilaMQTT](https://github.com/mathieucarbou/MycilaMQTT)
- update AsyncTCPSock library to [mathieucarbou/AsyncTCPSock](https://github.com/mathieucarbou/AsyncTCPSock) v3.2.10
- change communication from SSE to websocket
- add "Mode: SystemLog" to the WebUI Logger function that shows the ESP_LOG messages
- add mqtt information to webUI
- remove "uptime" from MQTT discovery, because it floods the log #108
- fix typo "Ferhler" in OTA Dialog #108
- clean unused webUI texts
- update Descriptions for Ethernet and Exhaust Sensor in README.md
- add watchdog to automatically reboot if ESP is getting stuck
- add solar FM244 support #64
- add status icons for WiFi and Ethernet

