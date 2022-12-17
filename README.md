# ESP_Buderus_KM271
Control your Buderus Logamatic R2107 with ESP and MQTT

The information from the heater provides a better understanding of how the heater works and offers opportunities for optimization.  
In combination with influxDB and Grafana you can also create usefull and impressive Dashboard of your heating system.

for more informations take a look at the **[wiki](https://github.com/dewenni/ESP_Buderus_KM271/wiki)**

## Functional description

The heart of the project is the reverse engineered Buderus interface, that is based on the 3964R Protocol.  
The main code is based on the work of **Michael Mayer** who has set a really good base for the communication.
It has been extended with the possibility not only to read values, but also to write some common values to the Logamatic.

### List of supported values

| **config values (read)**     | **status values (read)**       | **commands and setvalues (write)** |
|------------------------------|--------------------------------|------------------------------------|
| HK1_Aussenhalt_ab            | Abgastemperatur                | HK1_Betriebsart                    |
| HK1_Betriebsart              | Aussentemperatur_gedaempft     | HK1_Programm                       |
| HK1_Fernbedienung            | Aussentemperatur_gedaempft     | HK1_Auslegung                      |
| HK1_Heizsystem               | Brenner_Ansteuerung            | HK2_Betriebsart                    |
| HK1_Max_Temperatur           | Brenner_Ausschalttemperatur    | HK2_Programm                       |
| HK1_Nachttemperatur          | Brenner_Einschalttemperatur    | HK2_Auslegung                      |
| HK1_Tagtemperatur            | Brenner_Laufzeit_Minuten       | WW_Betriebsart                     |
| HK1_Temperatur_Offset        | Brenner_Laufzeit_Minuten256    | WW_Soll                            |
| HK1_Urlaubtemperatur         | Brenner_Laufzeit_Summe         | Sommer_ab                          |
| HK1_Programm                 | HK1_BW1_Ausschaltoptimierung   | Frost_ab                           |
| HK2_Aussenhalt_ab            | HK1_BW1_Automatik              | Aussenhalt_ab                      |
| HK2_Betriebsart              | HK1_BW1_Einschaltoptimierung   | setdatetime (NTP-Server)           |
| HK2_Fernbedienung            | HK1_BW1_Estrichtrocknung       |                                    |
| HK2_Heizsystem               | HK1_BW1_Ferien                 |                                    |
| HK2_Max_Temperatur           | HK1_BW1_Frostschutz            |                                    |
| HK2_Nachttemperatur          | HK1_BW1_Manuell                |                                    |
| HK2_Tagtemperatur            | HK1_BW1_Warmwasservorrang      |                                    |
| HK2_Temperatur_Offset        | HK1_BW2_Externer_Stoereingang  |                                    |
| HK2_Urlaubtemperatur         | HK1_BW2_FB_fehlerhaft          |                                    |
| HK2_Programm                 | HK1_BW2_Fehler_Vorlauffuehler  |                                    |
| Pumplogik                    | HK1_BW2_Keine_Komm_mit_FB      |                                    |
| Sommer_ab                    | HK1_BW2_Maximaler_Vorlauf      |                                    |
| Sprache                      | HK1_BW2_Sommer                 |                                    |
| WW_Aufbereitung              | HK1_BW2_Tag                    |                                    |
| WW_Betriebsart               | HK1_Heizkennlinie_-10_Grad     |                                    |
| WW_Temperatur                | HK1_Heizkennlinie_0_Grad       |                                    |
| WW_Vorrang                   | HK1_Heizkennlinie_10_Grad      |                                    |
| WW_Zirkulation               | HK1_Pumpe                      |                                    |
| Max_Kesseltemperatur         | HK1_Raumsolltemperatur         |                                    |
|                              | HK1_Vorlaufisttemperatur       |                                    |
|                              | HK1_Vorlaufsolltemperatur      |                                    |
|                              | HK2_BW1_Ausschaltoptimierung   |                                    |
|                              | HK2_BW1_Automatik              |                                    |
|                              | HK2_BW1_Einschaltoptimierung   |                                    |
|                              | HK2_BW1_Estrichtrocknung       |                                    |
|                              | HK2_BW1_Ferien                 |                                    |
|                              | HK2_BW1_Frostschutz            |                                    |
|                              | HK2_BW1_Manuell                |                                    |
|                              | HK2_BW1_Warmwasservorrang      |                                    |
|                              | HK2_BW2_Externer_Stoereingang  |                                    |
|                              | HK2_BW2_FB_fehlerhaft          |                                    |
|                              | HK2_BW2_Fehler_Vorlauffuehler  |                                    |
|                              | HK2_BW2_Keine_Komm_mit_FB      |                                    |
|                              | HK2_BW2_Maximaler_Vorlauf      |                                    |
|                              | HK2_BW2_Sommer                 |                                    |
|                              | HK2_BW2_Tag                    |                                    |
|                              | HK2_Heizkennlinie_-10_Grad     |                                    |
|                              | HK2_Heizkennlinie_0_Grad       |                                    |
|                              | HK2_Heizkennlinie_10_Grad      |                                    |
|                              | HK2_Pumpe                      |                                    |
|                              | HK2_Raumsolltemperatur         |                                    |
|                              | HK2_Vorlaufisttemperatur       |                                    |
|                              | HK2_Vorlaufsolltemperatur      |                                    |
|                              | Kessel_Betrieb_Abgastest       |                                    |
|                              | Kessel_Betrieb_Betrieb_Stufe1  |                                    |
|                              | Kessel_Betrieb_BetriebStufe2   |                                    |
|                              | Kessel_Betrieb_Kesselschutz    |                                    |
|                              | Kessel_Betrieb_Leistung_frei   |                                    |
|                              | Kessel_Betrieb_Leistung_hoch   |                                    |
|                              | Kessel_Betrieb_Unter_Betrieb   |                                    |
|                              | Kessel_Vorlaufisttemperatur    |                                    |
|                              | Kessel_Vorlaufsolltemperatur   |                                    |
|                              | WW_BW2_Ausschaltoptimierung    |                                    |
|                              | WW_BW2_Einschaltoptimierung    |                                    |
|                              | WW_BW2_Laden                   |                                    |
|                              | WW_BW2_Manuell                 |                                    |
|                              | WW_BW2_Nachladen               |                                    |
|                              | WW_BW2_Tag                     |                                    |
|                              | WW_BW2_Vorrang                 |                                    |
|                              | WW_BW2_Warm                    |                                    |
|                              | WW_Isttemperatur               |                                    |
|                              | WW_Pumpentyp_Absenkung_Solar   |                                    |
|                              | WW_Pumpentyp_Ladepumpe         |                                    |
|                              | WW_Pumpentyp_Zirkulationspumpe |                                    |
|                              | WW_Solltemperatur              |                                    |



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
Payload:  Auflösung: 1 °C Stellbereich: 30 – 90 °C WE: 75 °C

Topic: esp_heizung/setvalue/hk1_aussenhalt_ab  
Payload:  -20..10 (°C)

Topic: esp_heizung/setvalue/hk2_betriebsart  
Payload:  0:Nacht | 1:Tag | 2:AUTO

Topic: esp_heizung/setvalue/hk2_programm 
Payload:  Programmnummer 0..8 (see documentation)

Topic: esp_heizung/setvalue/hk2_auslegung  
Payload:  Auflösung: 1 °C Stellbereich: 30 – 90 °C WE: 75 °C

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

see full description in the **[wiki](https://github.com/dewenni/ESP_Buderus_KM271/wiki)**

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
---

# use at own risk!

**feel free to use and adopt to your needs!**

**If you have something to improve, let us all know about you ideas!**

