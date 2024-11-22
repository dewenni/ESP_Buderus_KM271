# v5.0.0

## what's new

This is a major update with some new features and some redesign under the hood.
The Framework is updated to the new Arduino v3 based on ESP-IDF v5.1+
Several modifications have been made to achieve better performance and stability.

### Performance and stability improvements

Various improvements have been made to increase performance and stability. In particular, the communication between ESP and the web server has been redesigned and improved. The protocol was changed from SSE (Server-Sent-Events) to Websockets.
The maximum number of client connections is monitored and limited to the number of 2 active connections.


### Ethernet Support W5500

There is a new support for W5500 Ethernet Modules. This brings the possibility to use ethernet connection in parallel to WiFi connection.
This could be useful in situations where the WiFi connection is not available or stable.

For mor details, please have a look at the [documentation](https://github.com/dewenni/ESP_Buderus_KM271?tab=readme-ov-file#optional-ethernet-module-w5500) 


### Solar Support FM244

Support for optional Buderus Solar Module FM244 added. You can activate this in the same way as the heating circuits and hot-water.
Values are send by mqtt and are also available in the webUI.

### System Logger

The logging function has been extended. The mode can now also be switched to ‘System log’. In this case, the serial monitor is redirected to the log and you can see the messages that are normally only visible when you are connected via cable.

### new HC1/HC2 Control Elements

There are new HC1/HC2 control elements on the web UI

- HC1 Day Temperature / HC2 Day Temperature
- HC1 Night Temperature / HC2 Night Temperature
- HC1 Holiday Temperature / HC2 Holiday Temperature
- HC1 Switch-On Temperature / HC2 Switch-On Temperature

### additional Status Information in the web UI

There are some additional information in the status Tab of the web UI

- Status information about Ethernet connection
- Status information about KM271 connection
- Status information about MQTT connection

### BREAKING CHANGES!!!

> [!IMPORTANT]   
> In the past, there was a separate setting for the static IP. Now we have two possible interfaces and therefore the static IP setting is also duplicated and is located together with the WiFi and Ethernet settings. If you previously used static IP, you will need to adjust your settings.
>
>**I would recommend deactivating the static IP before the update!**


## changelog

- update to espressif/arduino-esp32 V3.07
- update AsyncTCP library to [mathieucarbou/AsyncTCPS](https://github.com/mathieucarbou/AsyncTCP)
- change communication from SSE to websocket
- optimization and reduction of the data sent from the ESP to the web server
- remove "uptime" from MQTT discovery, because it floods the log #108
- fix typo "Ferhler" in OTA Dialog #108
- fix name of exported config.json file
- clean unused webUI texts
- direct apply of the settings after import of config.json
- update Descriptions for Ethernet and Exhaust Sensor in README.md
- add watchdog to automatically reboot if ESP is getting stuck
- add solar FM244 support #64
- add status icons for WiFi and Ethernet
- add "Mode: SystemLog" to the WebUI Logger function that shows the ESP_LOG messages
- add Status information about Ethernet connection
- add Status information about KM271 connection
- add Status information about MQTT connection
- add control for HC1 Day Temperature / HC2 Day Temperature
- add control for HC1 Night Temperature / HC2 Night Temperature to webUI
- add control for HC1 Holiday Temperature / HC2 Holiday Temperature to webUI
- add control for HC1 Switch-On Temperature / HC2 Switch-On Temperature to webUI