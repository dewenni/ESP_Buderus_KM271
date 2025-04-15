<div align="center">
<img style="width: 100px;" src="./Doc/ESP-Buderus-KM271-Logo.svg"> 

<h3 style="text-align: center;">ESP-Buderus-KM271</h3>
</div>

-----

**[🇩🇪 Deutsche Version der Beschreibung](README_DE.md)**

-----

<div align="center">

[![Current Release](https://img.shields.io/github/release/dewenni/ESP_Buderus_KM271.svg)](https://github.com/dewenni/ESP_Buderus_KM271/releases/latest)
![GitHub Release Date](https://img.shields.io/github/release-date/dewenni/ESP_Buderus_KM271)
![GitHub last commit](https://img.shields.io/github/last-commit/dewenni/ESP_Buderus_KM271)
![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/dewenni/ESP_Buderus_KM271/total?label=downloads%20total&color=%23f0cc59)
![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/dewenni/ESP_Buderus_KM271/latest/total?label=downloads%20latest%20Release&color=%23f0cc59)

![GitHub watchers](https://img.shields.io/github/watchers/dewenni/ESP_Buderus_KM271?style=social)
[![GitHub stars](https://img.shields.io/github/stars/dewenni/ESP_Buderus_KM271.svg?style=social&label=Star)](https://github.com/dewenni/ESP_Buderus_KM271/stargazers/)

[![Stargazers over time](http://starchart.cc/dewenni/ESP_Buderus_KM271.svg?background=%23ffffff00&axis=%236c81a6&line=%236c81a6)](https://github.com/dewenni/ESP_Buderus_KM271/stargazers/)

</div>


-----

<div align="center">
If you like this project, feel free to push the <b>[Star ⭐️]</b> button and click <b>[Watch 👁]</b> to stay updated.
<br><br>
And if you'd like to support my work, you can also<p>

[![Sponsor](https://img.shields.io/badge/Sponsor%20me%20on-GitHub-%23EA4AAA.svg?style=for-the-badge&logo=github)](https://github.com/sponsors/dewenni)

</div>

-----

Control your Buderus Logamatic R2107 / HS 2105 with ESP and MQTT

The information from the heater provides a better understanding of how the heater works and offers opportunities for optimization.

In combination with influxDB and Grafana you can also create useful and impressive Dashboard of your heating system.

But there is also a build in WebUI to view and control your Logamatic without any other Software.


![weubui_dash](Doc/weubui_dash.png)
(Desktop Version)

The WebUI is responsive and also offers a mobile layout.

<img style="display: inline;
  margin-right: 50px;
  width: 200px;" src="./Doc/webui_mobile_1.png"> 
<img style="display: inline;
  margin-right: 50px;
  width: 200px;" src="./Doc/webui_mobile_2.png"> 
<img style="display: inline;
  margin-left: auto;
  width: 200px;" src="./Doc/webui_mobile_3.png">   
(Mobile Version)

For a first impression of the functions and the WebUI, a limited demo is also available.  
This can be accessed via the following link: [WebUI-DEMO](https://dewenni.github.io/ESP_Buderus_KM271/)

-----

# Table of Contents

- [Overview](#overview)

- [Hardware](#hardware)
  - [Option 1 - Board from the78mole](#option-1---board-from-the78mole)
  - [Option 2 - ESP32 with original Buderus KM271](#option-2---esp32-with-original-buderus-km271)
  - [Optional: Hardware Oil Meter](#optional-hardware-oil-meter)
  - [Optional: OneWire Sensor](#optional-onewire-sensor)
  - [Optional: Exhaust Sensor](#optional-exhaust-sensor)
  - [Optional: Ethernet Module W5500](#optional-ethernet-module-w5500)

- [Getting started](#getting-started)
  - [Platform-IO](#platform-io)
  - [ESP-Flash-Tool](#esp-flash-tool)
  - [OTA-Updates](#ota-updates)
  - [Setup-Mode](#setup-mode)
  - [Configuration](#configuration)
  - [Filemanager](#filemanager)

- [MQTT](#mqtt)
  - [Config and Status values](#config-and-status-values)
  - [Commands](#commands)
  - [Home Assistant](#home-assistant)

- [Optional Messaging](#optional-messaging)
  - [Pushover](#pushover)
  - [WebUI-Logger](#webui-logger)
  - [Telnet](#telnet)

- [Optional Components](#optional-components)
  - [node-red](#node-red)
  - [grafana](#grafana)

- [FAQ](#faq)

-----

# Overview

The heart of the project is the reverse engineered Buderus interface, that is based on the 3964R Protocol.  
The main code is based on the work of **Michael Mayer** who has set a really good base for the communication.
It has been extended with the possibility not only to read values, but also to write some common values to the Logamatic.

The software has multi language support and there is already german, and english texts available. It is also possible to add more languages.

Feel free to add more languages. The texts are located in: **[language.h](include/language.h)** and **[lang.js](web/js/lang.js)**

## additional and optional Oilcounter / Oil Meter

The project includes also an additional and optional oilcounter implementation. I have installed a Braun HZ-5 Meter to measure the oil consumption.  
There are different models with (HZ 5R, HZ 5DR) and without pulse output (HZ 5).  
I have used the normal one without pulse output and modified it with a small reed contact - that works fine and was simple to install.

-----

# Hardware

## Option 1 - Board from the78mole

the easiest, smartest and even cheapest option is the DIY Interface that was build by Daniel Glaser. Big thanks for his engagement in this Topic!  
You can find more information here: <https://github.com/the78mole/km271-wifi>  
You can order it here: <https://www.tindie.com/products/24664/>  

In this case you only need this DIY interface and nothing more.
It includes the RS232/TTL Adapter and also an ESP32.  

![KM217_mod](/Doc/KM271-WiFi.png)


## Option 2 - ESP32 with original Buderus KM271

The other option is, to use the original Buderus KM271 Module that has a serial interface (RS232).
In combination with a RS232 TTL Adapter (MAX3232) it can be connected to the TX/RX Port of the ESP.

Logamatic R2107 => KM271 => RS232/TTL Adapter => ESP

Example configuration:

```text
(ESP32)GPIO17/TXD2  -> (MAX3232)TXD -> (serial cable) -> (KM271-SUBD)PIN2:RXD
(ESP32)GPIO16/RXD2 <- (MAX3232)RXD <- (serial cable) <- (KM271-SUBD)PIN3:TXD
(ESP32)GND <-> (MAX3232)GND <-> (serial cable) <-> (KM271-SUBD)PIN5:GND
```

![km271_orig](/Doc/esp32_with_km271.jpeg)

## Optional: Ethernet Module W5500

### Option 1 - single W5500 board

It is also possible to connect a W5500 Ethernet module to the Board or a generic ESP32. For the KM271 Board´s from Daniel you can connect the W5500 to the J7 Connector of the Board.

> [!IMPORTANT]
> The connection cable should be as short as possible (approx 10cm)

Board >= 0.0.6

| Signal | GPIO          | Pin (J7) |
|--------|---------------|----------|
| VCC    |               | J7.2     |
| GND    |               | J7.10    |
| CLK    |  18           | J7.9     |
| MOSI   |  23           | J7.7     |
| MISO   |  19           | J7.5     |
| CS     |  15           | J7.3     |
| INT    |  14           | J7.8     |
| RST    |  13           | J7.6     |


Example for generic ESP32-Mini

| Signal| GPIO |
|-------|------|
| CLK   | 18   |
| MOSI  | 23   |
| MISO  | 19   |
| CS    | 5    |
| INT   | 16   |
| RST   | 17   |

![W5500](/Doc/w5500.png)

### Option 2 - Board from the78mole

There is also an expansion module in the pipeline that fits perfectly on the KM271 WiFi board.  

https://www.tindie.com/products/the78mole/km271-wifi-ethernet-extension

![W5500-Extention](/Doc/w5500_extention.png)

## Optional: Hardware Oil Meter

The software is also prepared to connect an Oil Meter. A well-known manufacturer of oil meters is Braun with the models HZ-5 or HZ6.
These are already available with a potential-free contact.  
I have used one without potential-free contact and have subsequently attached a reed contact. This was also very simple and works very reliably.

![braun_hz5](/Doc/oilmeter.jpeg)

> [!NOTE] 
> but this is only optional and can be used additionally to the Information´s that the software will read from the Logamatic.


## Optional: OneWire Sensor

You can also configure additional OneWire Sensors (e.g. DS18B20). In the configuration you can setup one or two sensors.
The Sensor value will shown on the Dashboard and will also be send by mqtt with Topic `sensor` and the name that you can configure.
Depending on the hardware used, an additional resistor may need to be installed. Classically, the OneWire sensors are connected with a resistor of 4.7kOhm between VCC and the sensor cable and operated with 3.3V - 5V.
Only the GPIO to which the sensor cable is connected is specified in the configuration. The rest is hardware-dependent wiring.

> [!NOTE] 
> The sensor must be connected when the ESP is started, otherwise it will not be found. Changes to the cabling or the GPIO settings require a restart.


<img src="Doc/opt_sensor_dash.png" alt="opt-sensor-dash1" width="75%">

(dashboard controls)

<img src="Doc/opt_sensor_cfg.png" alt="opt-sensor-config" width="75%">

(settings)

> [!NOTE] 
> but this is only optional and can be used additionally to the Information´s that the software will read from the Logamatic.

## Optional: Exhaust Sensor

It is also possible to connect a optional exhaust sensor (NTC 100K) to the Connector J5 of the Board. In some cases you have to add some missing components to the Board.


| Component | Value        |
|-----------|--------------|
| D5        | BZX84C5V1    |
| R17       | 3.3K         |
| R11       | 100K         |
| C11       | 100nF        |
| C12       | 33nF         |
| R35       | 0            |
| R39       | 0            |
 
There is no need to configure the sensor in the software. The logamatic itself will automatically detect the sensor and send the value similar to the other values.

![exhaust-sens](/Doc/exhaust_sens.png)

-----

# Getting started

## Platform-IO

The software is created with [Visual Studio Code](https://code.visualstudio.com) and the [PlatformIO-Plugin](https://platformio.org).  
After installing the software you can clone the project from GitHub or you can download it as zip and open it in PlatformIO.
Then adapt the `upload_port` and corresponding settings in `platformio.ini` to your USB-to-serial Adapter and upload the code to the ESP.

> [!NOTE]
> Python must also be installed in order to fully compile the project. The scripts folder contains, for instance, scripts for creating the web pages that are called when the project is compiled.

## ESP-Flash-Tool

In the releases, you can find also the binary of the Software. If you don´t want to use PlatformIO, you can also use the `buderus_km271_esp32_flash_vx.x.x.bin` file and flash it directly on the ESP. This bin-file is already a merge with bootloader.bin, partitions.bin and the application.bin. You can flash this image an the ESP at address 0x00.  

**Windows**  
There are several tools available to flash binaries to the ESP.  
One of them is [espressif-flash-download-tool](https://www.espressif.com/en/support/download/other-tools)

**macOS/Linux**  
for Mac it is hard to find a tool with a graphical UI, but you can simple use the esptool.py:

1. open Terminal
2. install esptool: `pip install esptool`  
3. optional get the install path: `which esptool.py`  
4. set path: `export PATH="$PATH:/<path>/esptool.py"` (<- change <path> with result from 3.)
5. goto path where the bin file is located
6. get Device String: `ls /dev/tty* | grep usb` (use this in next Step for <UPLOAD-PORT>)
7. upload: `esptool.py -p <UPLOAD-PORT> write_flash 0x00 buderus_km271_esp32_flash_vx.x.x.bin`  

## OTA-Updates

### local Web OTA-Update

The first option is, to download the ota Update File from the latest release at GitHub.
After you have downloaded this to your computer, you can perform a update with the embedded WebUI OTA-Update.
You can find the update function in the "Tools" Tab of the WebUI.

![ota-1](Doc/tools.png)

### GitHub OTA-Update

since Version 5.3.0 it is also possible to update the controller directly in the WebUI without downloading the .bin file before.
If you click on the Version info on the bottom left, a dialog will open. If there is a new version available, you can directly initiate the update here. It will then automatically download and install the latest release from github!

![ota-2](Doc/github_ota.gif)

### PlatformIO OTA-Update

But it is also possible to download the software wireless with platformio.
You only have to change the `upload_port` settings in `platformio.ini`

There are two predefined Options:

- OPTION 1: direct cable upload
- OPTION 2: wireless OTA Update

## Setup Mode

There is a "Setup Mode" available. The "Setup Mode" is activated, when you press the "reset-button" of the ESP two times within 3 Seconds.
The "Setup Mode" will also activated if there is no (initial or wrong) wifi connection configured.

If the ESP goes into "Setup Mode", it will automatically create a own network access point with ssid  
📶 `"ESP-Buderus-KM271"`  
After you are connected to this network, you can open the webUI on ip-address  
**"http://192.168.4.1"**

## Configuration

Here you can setup all the configuration that fits to your heating system and your infrastructure.

- **WiFi**  
enter your WiFi credentials to connect the ESP to your network

- **Ethernet W5500**  
use Ethernet connection based on W5500 to connect the ESP to your network

- **Authentication**  
you can activate the authentication feature and configure user and password.

- **NTP Server**  
the ESP can connect to a NTP server to get the right Time information.
The default Time-Zone should fit if you are located in germany. Otherwise you can change it manually

- **Date and Time**  
Here you can write a new Date and Time to the Logamatic heating system. (manual or actual NTP-Server time)

- **MQTT**  
here you can activate the MQTT communication and enter mandatory parameters

- **Pushover**  
Parameters for Pushover notifications.  
(API-Token and User-Key)  
You can also send a test message here.

- **Logamatic**  
here you can select, which components of your Logamatic should be used.

- **GPIO**  
Here you can configure the GPIO of your ESP-Board. You can use the options in the dropdown to get default values depending of the selected type of board.

- **Oil Meter**  
here you can enable the optional hardware or virtual Oil Meter.
If you use a hardware based Oil Meter, you have to configure also to regarding gpio´s.
If you want to calculate the consumption based on the runtime, you have to configure the additional calculation parameters.

- **optional sensors**  
Activation and configuration of optional DS18B20

- **Simulation**  
Activate Simulation-Mode to generate Logamatic values for testing purposes

- **Language**  
There are two languages available. Choose what you prefer.

> [!NOTE]
> All settings are automatically saved when changes are made

![weubui-settings](Doc/weubui_setting.png)

## Filemanager

there is also a builtin file manager to open (show), download (export) and upload (import) the configuration file.
The configuration is stored in the ```config.json``` file. To backup and restore the configuration you can download and upload this file.

![filemanager](/Doc/tools.png)

-----

# MQTT

> [!NOTE]
> You can set a separate language for the mqtt topics in the mqtt settings that is independent of the webUI language.

## Config and Status values

The Software handles different kind of values:

### config values (read only)

this are config values from the Logamatic. The values are read at startup or if you change them at the Logamatic. The payload of the values are integer or float.

Config values as single topics (see list in [param.txt](Doc/param.txt))

```text
example:
Topic: esp_heizung/config/frost_protection_threshold
Payload:   -1.00 °C     (String)
```

### status values (read only)

this values will mostly change during runtime and will automatically send if changed. The payload of the values is a String.

Status values as single topics (see list in [param.txt](Doc/param.txt))

```text
example:
Topic: esp_heizung/status/hc1_ov1_automatic
Payload:   1    (integer)
```

### additional information's (read only)

status information about WiFi:

```text
Topic: esp_heizung/wifi = {  
    "status":"online",  
    "rssi":"-50",  
    "signal":"90",  
    "ip":"192.168.1.1",  
    "date-time":"01.01.2022 - 10:20:30"  
}
```

debug information:

```text
Topic: esp_heizung/info = {  
    "logmode":true,
    "send_cmd_busy":false,
    "date-time":"01.01.2022 - 10:20:30"  
}
```

### alarm messages (read only)

here you get the information about the last 4 Errors/Faults that are registered by the Logamatic. The payload of the values is a String.

> [!NOTE]
>A complete List of supported values can be found in the **[param.txt](Doc/param.txt)**

you can also change the mqtt topics for your needs by editing: **[language.h](include/language.h)**

## Commands

To change the values of your Logamatic, you can use several `setvalue` commands from the list below.
A complete Topic could be `esp_heizung/setvalue/setdatetime`

**You can control the Logamatic with commands like this:**

```text
command:    restart ESP
topic:      {cmd/restart", cmd/restart"}
payload:    none

command:    Service interface - only for Experts - use at your own risk!!!
topic:      {cmd/service", cmd/service"}
payload:    8 hex values separated with "_" (example: 08_15_04_65_65_65_65_65)

command:    debug function - on/off
topic:      {cmd/debug", cmd/debug"}
payload:    0/1

command:    debug function - set Filter
topic:      {cmd/setdebugflt", cmd/setdebugflt"}
payload:    11 hex values separated with "_" (example: 08_15_XX_XX_XX_XX_XX_XX_XX_XX_XX)

command:    debug function - get Filter
topic:      {cmd/getdebugflt", cmd/getdebugflt"}
payload:    none (return value at message topic)

command:    set date & time of Logamatic
topic:      {"setvalue/setdatetime", setvalue/setdatetime"}
payload:    none

command:    set oilcounter to given value
topic:      {"setvalue/oilcounter", setvalue/oilcounter"}
payload:    counter value including decimals (123,45L = 1234) 

command:    heating circuit 1: operation mode 
topic:      {"setvalue/hk1_betriebsart", setvalue/hc1_operation_mode"}
payload:    0=night / 1=day / 2=auto  

command:    heating circuit 2: operation mode 
topic:      {"setvalue/hk2_betriebsart", setvalue/hc2_operation_mode"}
payload:    (0=night / 1=day / 2=auto)

command:    heating circuit 1: program
topic:      {"setvalue/hk1_programm", setvalue/hc1_program"}
payload:    (0=custom / 1=family / 2=early / 3=late / 4=AM / 5=PM / 6=noon / 7=single / 8=senior)

command:    heating circuit 2: program
topic:      {"setvalue/hk2_programm", setvalue/hc2_program"}
payload:    (0=custom / 1=family / 2=early / 3=late / 4=AM / 5=PM / 6=noon / 7=single / 8=senior)

command:    heating circuit 1: design temperature for heating curves
topic:      {"setvalue/hk1_auslegung", setvalue/hc1_interpretation"}
payload:    Resolution: 1 [°C] - Range: 30 ... 90 [°C]

command:    heating circuit 2: design temperature for heating curves
topic:      {"setvalue/hk2_auslegung", setvalue/hc2_interpretation"}
payload:    Resolution: 1 [°C] - Range: 30 ... 90 [°C]

command:    heating circuit 1: switch on temperature
topic:      {"setvalue/hk1_aufschalttemperatur", setvalue/hc1_switch_on_temperature"}
payload:    Resolution: 1 [°C] - Range: 0 ... +10 [°C]

command:    heating circuit 2: switch on temperature
topic:      {"setvalue/hk1_aufschalttemperatur", setvalue/hc1_switch_on_temperature"}
payload:    Resolution: 1 [°C] - Range: 0 ... +10 [°C]

command:    heating circuit 1: switch off threshold for reduction mode
topic:      {"setvalue/hk1_aussenhalt_ab", setvalue/hc1_switch_off_threshold"}
payload:    Resolution: 1 [°C] - Range: -20 ... +10 [°C]

command:    heating circuit 2: switch off threshold for reduction mode
topic:      {"setvalue/hk2_aussenhalt_ab", setvalue/hc2_switch_off_threshold"}
payload:    Resolution: 1 [°C] - Range: -20 ... +10 [°C]

command:    heating circuit 1: day temperature setpoint
topic:      {"setvalue/hk1_tagtemperatur", "setvalue/hc1_day_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    heating circuit 2: day temperature setpoint
topic:      {"setvalue/hk2_tagtemperatur", "setvalue/hc2_day_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    heating circuit 1: night temperature setpoint
topic:      {"setvalue/hk1_nachttemperatur", setvalue/hc1_night_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    heating circuit 2: night temperature setpoint
topic:      {"setvalue/hk2_nachttemperatur", setvalue/hc2_night_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    heating circuit 1: holiday temperature setpoint
topic:      {"setvalue/hk1_urlaubtemperatur", setvalue/hc1_holiday_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    heating circuit 2: holiday temperature setpoint
topic:      {"setvalue/hk2_urlaubtemperatur", setvalue/hc2_holiday_temp"}
payload:    Resolution: 0.5 [°C] - Range: 10 .. 30 [°C] 

command:    warm water: operation mode
topic:      {"setvalue/ww_betriebsart", setvalue/ww_operation_mode"}
payload:    0=night / 1=day / 2=auto

command:    heating circuit 1: summer mode threshold Temperature
topic:      {"setvalue/hk1_sommer_ab", setvalue/hc1_summer_mode_threshold"}
payload:    Resolution: 1 [°C] - Range: 9:Summer | 10°..30° | 31:Winter

command:    heating circuit 2: summer mode threshold Temperature
topic:      {"setvalue/hk2_sommer_ab", setvalue/hc2_summer_mode_threshold"}
payload:    Resolution: 1 [°C] - Range: 9:Summer | 10°..30° | 31:Winter

command:    heating circuit 1: frost mode threshold Temperature
topic:      {"setvalue/HK1_Frost_ab", "setvalue/hc1_frost_protection_threshold"}
payload:    Resolution: 1 [°C] - Range: -20 ... +10 [°C]

command:    heating circuit 2: frost mode threshold Temperature
topic:      {"setvalue/HK2_Frost_ab", "setvalue/hc2_frost_protection_threshold"}
payload:    Resolution: 1 [°C] - Range: -20 ... +10 [°C]

command:    warm water: setpoint temperature
topic:      {"setvalue/ww_temperatur", setvalue/ww_temp"}
payload:    Resolution: 1 [°C] - Range: 30 ... 60 [°C]

command:    heating circuit 1: count of days for holiday mode (Logamatic will decrement every day by one)
topic:      {"setvalue/HK1_Ferien_Tage", "setvalue/hc1_holiday_days"}
payload:    count of days 0 .. 99

command:    heating circuit 2: count of days for holiday mode (Logamatic will decrement every day by one)
topic:      {"setvalue/HK2_Ferien_Tage", "setvalue/hc2_holiday_days"}
payload:    count of days 0 .. 99

command:    warm water pump cycles
topic:      {"setvalue/ww_zirkulation", setvalue/ww_circulation"}
payload:    Resolution: 1 [cycles/hour] - Range: 0:OFF | 1..6 | 7:ON

command:    heating circuit 1: reduction mode
topic:      {"setvalue/hk1_absenkungsart", setvalue/hc1_reduction_mode"}
payload:    Number 0..3 (Abschalt,Reduziert,Raumhalt,Aussenhalt) / {off,fixed,room,outdoors)

command:    heating circuit 2: reduction mode
topic:      {"setvalue/hk2_absenkungsart", setvalue/hc2_reduction_mode"}
payload:    Number 0..3 (Abschalt,Reduziert,Raumhalt,Aussenhalt) / {off,fixed,room,outdoors)

```

## Home Assistant

MQTT discovery for Home Assistant makes it easy to get all values in Home Assistant.
The Logamatic values will automatically visible as mqtt device in Home Assistant.
The config values and the status values are displayed. Some config values can also be changed as in the WebUI.

see also the official documentation: https://www.home-assistant.io/integrations/mqtt/#discovery-messages

<img src="Doc/mqtt_ha_1.png" alt="mqtt_ha1" width="75%"> <img src="Doc/mqtt_ha_2.png" alt="mqtt_ha2" width="40%">

In the mqtt settings you can activate the discovery function and also set the mqtt discovery topic and the device name for Home Assistant  
<img src="Doc/mqtt_ha_3.png" alt="mqtt_ha1" width="50%">

-----

# Optional Messaging

in addition to mqtt there are more options for notification.

## Pushover

In addition there is also a custom notification as Pushover client.
Depending on the parameter "Filter", you can define what kind of messages you want to receive.
In the settings you can find all necessary parameters to setup the client.

Each application, service, or utility that sends notifications through Pushover's API needs to have its own API token which uniquely identifies all of the API requests that it makes.
API tokens are free and can be registered through [Pushover website](https://pushover.net/apps/build).

<img src="./Doc/pushover.png" width="75%">

## WebUI-Logger

There is also a log function with which you can record various messages depending on the filter and display them via the WebUI. This can be useful for your own debugging and also for the further development of the software.

<img src="./Doc/logger.png" width="75%">

## Telnet

In addition to the WebUI and MQTT, there is also a Telnet interface to communicate with the ESP.
The interface offers several commands to read out information and send commands.
An overview of the commands can be called up using the "help" command.
To connect, a simple Telnet connection can be started via the corresponding IP address of the ESP.

Example: 
```
> telnet 192.168.178.135
```

<img src="./Doc/telnet.png" width="75%">

-----

# Optional Components

## node-red

I´m writing all information's that are transmitted over MQTT into a influxDB Database.  
In my case I'm using [node-red](https://nodered.org/) to receive the MQTT messages and to write it into the [influxDB](https://www.influxdata.com/m).  
Everything runs in Docker on my Synology NAS.  
But there are a lot of other possibilities - use the one that fits you best.

![node-red](/Doc/node-red.png)

If you are interested in my flows, you can use this export file:
[node-red.json](/Doc/node-red.json)

## grafana

To visualize the information's, I'm using [grafana](https://grafana.com) that gets the data out of the influxDB.  
For me this gets me more possibilities to analyze the behavior of the heating system compared to a static dashboard.  

Here are some impressions of what I did with all the information's that comes out of the Logamatic:

![grafana1](/Doc/grafana1.png)
![grafana2](/Doc/grafana2.png)
![grafana3](/Doc/grafana3.png)

If you are interested my dashboard, you can use this export file:
[grafana.json](/Doc/grafana.json)

> [!NOTE]
> It is based on InfluxDB 2.0 with query languange "Flux" and uses the german mqtt topics! If you setup your system the same way, it should be more or less a plug and play solution to import my grafana.json

# FAQ

## No connection to the Logamatic / No values from the Logamatic

1) Check the GPIO setting for your setup. When using the78mole boards, at least the following must be set:
`KM271-RX = 4` and `KM271-2X = 2`

2) Check that the resistance between pin 4 and pin 8 on the pin header where the board is plugged in is approx. 10kOhm.
This resistance is important for the Logamatic to recognise the board.

## ESP restarts automatically

1) WiFi connection is unstable
If the board is not also connected via Ethernet, a WiFi connection is essential. Therefore, if the connection is lost, an attempt is automatically made to re-establish the connection 5 times at intervals of 30 seconds. If this fails, the ESP restarts and tries again.
The reason for the last restart is entered in the WebUI on the ‘System’ page.

2) MQTT activated but no connection possible
If MQTT is activated but the connection is lost, the connection is automatically tried to be re-established 5 times (interval 10s, 20s, 30s, 40s, 50s). If this fails, the ESP restarts and tries again.
The reason for the last restart is entered in the WebUI on the ‘System’ page.

## Use with ioBroker

Please note that there are different topics for reading and writing values. When the ESP is started, the Logamatic automatically sends all config and status values and these are then also sent via MQTT. However, these are all read values that cannot be written via the same topic.
The topics for writing are always ../setvalue/..  
Possible commands can be found here, for example: [Commands](#commands)

You should also deactivate the following settings in ioBroker:

- "Publish states on subscribe"
- "Publish own states when connecting"

## OTA firmware update has failed

Unfortunately, it often happens that an OTA update via the WebUI is not successful. Unfortunately, I do not know the reasons for this.
However, it usually works after a few more attempts.
If you have connected the board via WiFi and Ethernet, try both connections. Sometimes it doesn't work via one connection, but then immediately via the other.

-----

# ❗️ use at own risk ❗️

**feel free to use and adopt to your needs!**

**If you have something to improve, let us all know about you ideas!**

❓ If you have a question, use the [Discussions](https://github.com/dewenni/ESP_Buderus_KM271/discussions)  
🐞 If there is a issue or bug, use the [Issues](https://github.com/dewenni/ESP_Buderus_KM271/issues)
