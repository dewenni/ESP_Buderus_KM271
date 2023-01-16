# ESP_Buderus_KM271
![Buderus_s](https://user-images.githubusercontent.com/46074831/209479478-c89a3a4e-e987-468a-82b2-b60f09ca56c3.png)
---
[![Current Release](https://img.shields.io/github/release/dewenni/ESP_Buderus_KM271.svg)](https://github.com/dewenni/ESP_Buderus_KM271/releases/latest)    
![GitHub Release Date](https://img.shields.io/github/release-date/dewenni/ESP_Buderus_KM271)    
![GitHub last commit](https://img.shields.io/github/last-commit/dewenni/ESP_Buderus_KM271)    
![GitHub watchers](https://img.shields.io/github/watchers/dewenni/ESP_Buderus_KM271?style=social)    
[![GitHub stars](https://img.shields.io/github/stars/dewenni/ESP_Buderus_KM271.svg?style=social&label=Star)](https://github.com/dewenni/ESP_Buderus_KM271/stargazers/)
---
Control your Buderus Logamatic R2107 with ESP and MQTT

The information from the heater provides a better understanding of how the heater works and offers opportunities for optimization.

In combination with influxDB and Grafana you can also create usefull and impressive Dashboard of your heating system.

But there is also a build in WebUI to view and control your Logamatic without any other Software.

<img width="1062" alt="image" src="https://user-images.githubusercontent.com/46074831/212485914-3ac7b0ae-12a0-479a-bb8a-3af1ae9e0853.png">

-----

>**Note**  
>for more informations take a look at the **[wiki](https://github.com/dewenni/ESP_Buderus_KM271/wiki)**

## Functional description

The heart of the project is the reverse engineered Buderus interface, that is based on the 3964R Protocol.  
The main code is based on the work of **Michael Mayer** who has set a really good base for the communication.
It has been extended with the possibility not only to read values, but also to write some common values to the Logamatic.

The software supports multi language support. 
You can switch between german and english mqtt topics. (see: **[/include/config.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/aa369b0bc6e71b8ec41ad1284f3467846cb56dcc/include/config.h)**)  
Feel free to add more languages. The texts are located in: **[/include/language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h)**

### List of supported values

The Software handles different kind of values:

- **config**:  
this are config values from the Logamatic. The values are read at startup or if you change them at the Logamatic. The payload of the values are integer or float.

- **status**:  
this values will mostly change during runtime and will automatically send if changed. The payload of the values is a String.

- **alarm**:  
here you get the information about the last 4 Errors/Faults that are registered by the Logamatic. The payload of the values is a String.

A complete List of supportet values can be found in the **[/include/language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h)**

### additional and optional Oilcounter / Oil Meter
The project includes also an additional and optional oilcounter implementation. I have installed an Braun HZ-5 Meter to measure the oil consumtion.  
There are diffeent models with (HZ 5R, HZ 5DR) and without pulse output (HZ 5).  
I have used the normal one without pulse output and modified it with a small reed contact - that works fine and was simple to install.
If you are not interested in the Oil Meter function you can simple disable it in config.h

---

## Hardware Requirements

### Option 1 - Board from the78mole
the easiest, smartest and even cheapest option is the DIY Interface that was build by Daniel Glaser. Big thanks for his engagement in this Topic!  
You can find more information here: [https://the78mole.de](https://the78mole.de/reverse-engineering-the-buderus-km217/)  
You can order it here: https://www.tindie.com/products/24664/

In this case you only need this DIY interface and nothing more.
It includes the RS232/TTL Adapter and also an ESP32.  

![KM217_mod](https://user-images.githubusercontent.com/46074831/206558276-ef8727ac-384c-4b7b-866f-8c3fe644a2cb.jpg)
(this is my board with the customized connector for the oil meter instead of the "USER 1" button)

### Option 2 - original Buderus KM271
The other option is, to use the original Buderus KM271 Module that has a serial interface (RS232).
In combination with a RS232 TTL Adapter (MAX3232) it can be connected to the TX/RX Port of the ESP.

Logamattic R2107 => KM271 => RS232/TTL Adapter => ESP

![km271_orig](https://user-images.githubusercontent.com/46074831/206558264-3d17b69a-e2ac-4e23-8ed6-b17977599c50.jpg)

---

## MQTT Communication

### You can control the Logamatic with commands like this:

```
Topic: esp_heizung/setvalue/hk1_betriebsart  
Payload:  0:Nacht | 1:Tag | 2:AUTO

Topic: esp_heizung/setvalue/hk1_programm 
Payload:  Programmnummer 0..8 (see documentation)

Topic: esp_heizung/setvalue/hk1_auslegung  
Payload:  Resolution: 1 °C Range: 30 – 90 °C WE: 75 °C

Topic: esp_heizung/setvalue/hk1_aussenhalt_ab  
Payload:  -20..10 (°C)

Topic: esp_heizung/setvalue/hk2_betriebsart  
Payload:  0:Nacht | 1:Tag | 2:AUTO

Topic: esp_heizung/setvalue/hk2_programm 
Payload:  Programmnummer 0..8 (see documentation)

Topic: esp_heizung/setvalue/hk2_auslegung  
Payload:  Resolution: 1 °C Range: 30 – 90 °C WE: 75 °C

Topic: esp_heizung/setvalue/hk2_aussenhalt_ab  
Payload:  -20..10 (°C)

Topic: esp_heizung/setvalue/ww_betriebsart  
Payload:  0:Nacht | 1:Tag | 2:AUTO

Topic: esp_heizung/setvalue/ww_soll  
Payload:  30..60 (°C)

Topic: esp_heizung/setvalue/sommer_ab  
Payload:  9:Winter | 10..30 (°C) | 31:Sommer

Topic: esp_heizung/setvalue/frost_ab  
Payload:   -20..10 (°C)

Topic: esp_heizung/setvalue/setdatetime  
Payload: none - it will be set by NTP Server

```

### As Status you will get informations:

Config values as single topics (see list in [language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h))
```
example:
Topic: esp_heizung/config/frost_protection_threshold
Payload:   -1.00 °C     (String)
```
Status values as single topics (see list in [language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h))
```
example:
Topic: esp_heizung/status/hc1_ov1_automatic
Payload:   1    (integer)
```
status information about WiFi:
```
Topic: esp_heizung/wifi = {  
    "status":"online",  
    "rssi":"-50",  
    "signal":"90",  
    "ip":"192.168.1.1",  
    "date-time":"01.01.2022 - 10:20:30"  
}
```
debug information:
```
Topic: esp_heizung/info = {  
    "logmode":true,
    "send_cmd_busy":false,
    "date-time":"01.01.2022 - 10:20:30"  
}
```

you can also change the mqtt topics for your needs by editig: **[/include/language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h)**

---

# use at own risk!

**feel free to use and adopt to your needs!**

**If you have something to improve, let us all know about you ideas!**

