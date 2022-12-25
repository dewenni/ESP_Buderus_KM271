# ESP_Buderus_KM271
Control your Buderus Logamatic R2107 with ESP and MQTT

The information from the heater provides a better understanding of how the heater works and offers opportunities for optimization.  
In combination with influxDB and Grafana you can also create usefull and impressive Dashboard of your heating system.

> **Note**
> for more informations take a look at the **[wiki](https://github.com/dewenni/ESP_Buderus_KM271/wiki)**

## Functional description

The heart of the project is the reverse engineered Buderus interface, that is based on the 3964R Protocol.  
The main code is based on the work of **Michael Mayer** who has set a really good base for the communication.
It has been extended with the possibility not only to read values, but also to write some common values to the Logamatic.

The software supports multi language support. You can switch between german and english mqtt topics. (see: **[/include/config.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/aa369b0bc6e71b8ec41ad1284f3467846cb56dcc/include/config.h)**)  
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

### Option 1
The first option is to use the original Buderus KM271 Module that has a serial interface (RS232).  
In combination with a RS232 TTL Adapter (MAX3232) it can be connected to the TX/RX Port of the ESP.  

Logamattic R2107 => KM271 => RS232/TTL Adapter => ESP

### Option 2
An even smarter and cheaper option, is the DIY Interface that was build by **Daniel Glaser**. Big thanks for his engagement in this Topic!  
You can find more information here: *[https://the78mole.de](https://the78mole.de/reverse-engineering-the-buderus-km217/)*  
You can order it here: https://www.tindie.com/products/24664/

In this case you only need this DIY interface and nothing more.  
It includes the RS232/TTL Adapter and also an ESP32.

---

## MQTT Communication

### You can control the Logamatic with the following commands:

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

```
Topic: esp_heizung/wifi = {  
    "status":"online",  
    "rssi":"-50",  
    "signal":"90",  
    "ip":"192.168.1.1",  
    "date-time":"01.01.2022 - 10:20:30"  
}

Config values as listed above (single topics)

Status values as lised above (single topics)

```

see full description in the **[wiki](https://github.com/dewenni/ESP_Buderus_KM271/wiki)**

you can also change the mqtt topics for your needs by editig: **[/include/language.h](https://github.com/dewenni/ESP_Buderus_KM271/blob/0439aeb246c99b3b6733f8a491dcddebd77829e8/include/language.h)**

---

# use at own risk!

**feel free to use and adopt to your needs!**

**If you have something to improve, let us all know about you ideas!**

