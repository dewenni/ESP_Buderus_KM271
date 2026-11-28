# MQTT-Referenz

[Zurück zur Anleitung](../README_DE.md) · [English](MQTT.md)

Diese Referenz vereint die bisherigen Angaben aus `param.txt` mit der aktuellen Befehls- und Topic-Definition der Firmware v6.1.0. Die Topic-Tabellen enthalten alle 79 Konfigurations-, 118 Status- und vier Fehlerspeicher-Topics. Werte werden nur veröffentlicht, soweit die Regelung sie liefert und die entsprechende Funktion aktiviert ist.

## Inhalt

- [Einrichtung und Topic-Aufbau](#einrichtung-und-topic-aufbau)
- [Konfigurationswerte](#konfigurationswerte)
- [Statuswerte](#statuswerte)
- [Alarmmeldungen](#alarmmeldungen)
- [Schreibbefehle](#schreibbefehle)
- [Beispiele](#beispiele)
- [Home Assistant und Quellen](#home-assistant-und-quellen)

## Einrichtung und Topic-Aufbau

MQTT unter **Einstellungen → Integrationen → MQTT** aktivieren und Broker, Port, Zugangsdaten sowie Basis-Topic setzen. WebUI- und MQTT-Sprache sind unabhängig. Eine Sprachänderung kann Topic-Namen und Textwerte ändern; Automationen entsprechend prüfen. `<basis>` steht unten für das konfigurierte Basis-Topic, beispielsweise `esp_heizung`.

| Topic | Inhalt | Retain |
| --- | --- | --- |
| `<basis>/config/<name>` | Logamatic-Konfiguration, Textwerte mit Einheiten oder Auswahltexten | einstellbar |
| `<basis>/status/<name>` | Messwerte und Betriebszustände der Logamatic | nein |
| `<basis>/alarm/<name>` | Vier Fehlerspeicherplätze, offene und beendete Meldungen | nein |
| `<basis>/sensor/<name>` | Optionaler OneWire-Sensor, Temperatur in °C als Zahl ohne Einheit | nein |
| `<basis>/oilcounter` | Hardware-Zählerstand in Hundertsteln Liter | ja |
| `<basis>/status` | ESP-Verfügbarkeit: `online` / `offline` (Last Will); kein Logamatic-Verbindungsnachweis | offline: ja; online: nein |
| `<basis>/wifi` | JSON: `status`, `rssi`, `signal`, `ip`, `date_time` | nein |
| `<basis>/eth` | JSON: `status`, `ip`, `link_up`, `link_speed`, `full_duplex`, `date_time` | nein |
| `<basis>/sysinfo` | JSON: `uptime`, `restart_reason`, `heap`, `flash` | nein |
| `<basis>/info` | JSON: `burner`, `pump`, `ww_temp`, `boiler_temp` | nein |
| `<basis>/message` | Befehlsrückmeldungen und Betriebsmeldungen | nein |
| `<basis>/debug` | JSON: `logmode`, `rx_age_s`, `send_cmd_busy`, `sw_version`, `date-time` | nein |
| `<basis>/debug_message` | Gefilterte Diagnosetelegramme und Schreibdiagnose | nein |
| `<basis>/undefined_message` | Unbekannte Telegramme | nein |

`status` und `status/<name>` sind verschiedene Topics. Zahlen verwenden einen Dezimalpunkt. Konfigurationswerte können beispielsweise `21.5 °C` enthalten, während Temperatur-Statuswerte ohne Einheit erscheinen. Einheiten in den folgenden Statustabellen erklären die Zahl und sind nicht Teil ihres Payloads.

Konfigurationswerte stammen aus empfangenen Telegrammen oder werden nach einer erfolgreichen Schreibquittierung als übernommen nachgeführt. Eine Quittierung bestätigt den Transport und ist keine unabhängige Rücklesebestätigung. Spätere echte Empfangswerte überschreiben die Annahme. Ein gelesener Wert ist nicht automatisch schreibbar; dafür gilt ausschließlich die Befehlsliste.

## Konfigurationswerte

Vollständiges Topic: `<basis>/config/<name>`.

### HK1 Konfiguration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK1_Frost_ab` | `hc1_frost_protection_threshold` | Zahl mit `°C` |
| `HK1_Sommer_ab` | `hc1_summer_mode_threshold` | Sommer / 10…30 °C / Winter |
| `HK1_Nachttemperatur` | `hc1_night_temp` | Zahl mit `°C` |
| `HK1_Tagtemperatur` | `hc1_day_temp` | Zahl mit `°C` |
| `HK1_Betriebsart` | `hc1_operation_mode` | Nacht / Tag / Automatik |
| `HK1_Urlaubtemperatur` | `hc1_holiday_temp` | Zahl mit `°C` |
| `HK1_Max_Temperatur` | `hc1_max_temp` | Zahl mit `°C` |
| `HK1_Auslegung` | `hc1_interpretation` | Zahl mit `°C` |
| `HK1_Aufschalttemperatur` | `hc1_switch_on_temperature` | Aus oder 1…10 °C |
| `HK1_Aussenhalt_ab` | `hc1_switch_off_threshold` | Zahl mit `°C` |
| `HK1_Absenkungsart` | `hc1_reduction_mode` | Abschalt / Reduziert / Raumhalt / Aussenhalt |
| `HK1_Heizsystem` | `hc1_heating_system` | Aus / Heizkoerper / - / Fussboden |
| `HK1_Temperatur_Offset` | `hc1_temp_offset` | Zahl mit `°C` |
| `HK1_Fernbedienung` | `hc1_remotecontrol` | Aus / An |
| `HK1_Programm` | `hc1_program` | Eigen / Familie / Frueh / Spaet / Vormittag / Nachmittag / Mittag / Single / Senior |
| `HK1_Ferien_Tage` | `hc1_holiday_days` | Zahl mit `Tage` |
| `HK1_Timer01` | `hc1_timer01` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer02` | `hc1_timer02` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer03` | `hc1_timer03` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer04` | `hc1_timer04` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer05` | `hc1_timer05` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer06` | `hc1_timer06` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer07` | `hc1_timer07` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer08` | `hc1_timer08` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer09` | `hc1_timer09` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer10` | `hc1_timer10` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer11` | `hc1_timer11` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer12` | `hc1_timer12` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer13` | `hc1_timer13` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK1_Timer14` | `hc1_timer14` | Drei formatierte Schaltpunkte; siehe Timerformat |

### HK2 Konfiguration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK2_Frost_ab` | `hc2_frost_protection_threshold` | Zahl mit `°C` |
| `HK2_Sommer_ab` | `hc2_summer_mode_threshold` | Sommer / 10…30 °C / Winter |
| `HK2_Nachttemperatur` | `hc2_night_temp` | Zahl mit `°C` |
| `HK2_Tagtemperatur` | `hc2_day_temp` | Zahl mit `°C` |
| `HK2_Betriebsart` | `hc2_operation_mode` | Nacht / Tag / Automatik |
| `HK2_Urlaubtemperatur` | `hc2_holiday_temp` | Zahl mit `°C` |
| `HK2_Max_Temperatur` | `hc2_max_temp` | Zahl mit `°C` |
| `HK2_Auslegung` | `hc2_interpretation` | Zahl mit `°C` |
| `HK2_Aufschalttemperatur` | `hc2_switch_on_temperature` | Aus oder 1…10 °C |
| `HK2_Aussenhalt_ab` | `hc2_switch_off_threshold` | Zahl mit `°C` |
| `HK2_Absenkungsart` | `hc2_reduction_mode` | Abschalt / Reduziert / Raumhalt / Aussenhalt |
| `HK2_Heizsystem` | `hc2_heating_system` | Aus / Heizkoerper / - / Fussboden |
| `HK2_Temperatur_Offset` | `hc2_temp_offset` | Zahl mit `°C` |
| `HK2_Fernbedienung` | `hc2_remotecontrol` | Aus / An |
| `HK2_Programm` | `hc2_program` | Eigen / Familie / Frueh / Spaet / Vormittag / Nachmittag / Mittag / Single / Senior |
| `HK2_Ferien_Tage` | `hc2_holiday_days` | Zahl mit `Tage` |
| `HK2_Timer01` | `hc2_timer01` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer02` | `hc2_timer02` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer03` | `hc2_timer03` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer04` | `hc2_timer04` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer05` | `hc2_timer05` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer06` | `hc2_timer06` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer07` | `hc2_timer07` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer08` | `hc2_timer08` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer09` | `hc2_timer09` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer10` | `hc2_timer10` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer11` | `hc2_timer11` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer12` | `hc2_timer12` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer13` | `hc2_timer13` | Drei formatierte Schaltpunkte; siehe Timerformat |
| `HK2_Timer14` | `hc2_timer14` | Drei formatierte Schaltpunkte; siehe Timerformat |

### Warmwasser Konfiguration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `WW_Vorrang` | `ww_priority` | Aus / An |
| `WW_Temperatur` | `ww_temp` | Zahl mit `°C` |
| `WW_Betriebsart` | `ww_operation_mode` | Nacht / Tag / Automatik |
| `WW_Aufbereitung` | `ww_processing` | Aus / An |
| `WW_Zirkulation` | `ww_circulation` | Aus / 1…6 / An |

### Solar Konfiguration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Solar_Betriebsart` | `solar_operation_mode` | Nacht / Tag / Automatik |
| `Solar_Aktivierung` | `solar_enabled` | Aus / An |
| `Solar_Maximaltemperatur` | `solar_temp_max` | Zahl mit `°C` |
| `Solar_Minimaltemperatur` | `solar_temp_min` | Zahl mit `°C` |

### Allgemeine Regelungsdaten Konfiguration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Gebaeudeart` | `building_type` | Leicht / Mittel / Schwer |
| `Sprache` | `language` | DE / FR / IT / NL / EN / PL |
| `Anzeige` | `display` | Automatik / Kessel / Warmwasser / Aussen |
| `Brennerart` | `burner_type` | 1-stufig / 2-stufig / Modulierend |
| `Max_Kesseltemperatur` | `max_boiler_temperature` | Zahl mit `°C` |
| `Pumplogik` | `pump_logic_temp` | Zahl mit `°C` |
| `Abgastemperaturschwelle` | `exhaust_gas_temperature_threshold` | Aus oder Temperaturtext 50…250 (°C) |
| `Brenner_Min_Modulation` | `burner_min_modulation` | Zahl mit `%` |
| `Brenner_Mod_Laufzeit` | `burner_modulation_runtime` | Zahl mit `s` |
| `Uhrzeit_Offset` | `time_offset` | vorzeichenbehaftete Zahl mit `s/d` |

### Timerformat

`HK1_Timer01`…`HK1_Timer14` bzw. `hc1_timer01`…`hc1_timer14` enthalten jeweils drei Schaltpunkte des Eigen-Programms. HK2 besitzt dieselben 14 Blöcke. Beispiel: `SP01: Mo 05:40 (An) | SP02: Mo 22:10 (Aus) | SP03: Di 05:30 (An)`. Tage und An/Aus-Texte folgen der MQTT-Sprache; freie Punkte werden als frei gekennzeichnet. Insgesamt sind 42 Schaltpunkte je Heizkreis möglich.

Die WebUI bearbeitet diese Rohdaten über ihren Wochenprogramm-Editor. Es gibt keinen MQTT-Schreibbefehl für einzelne Timerblöcke; quittierte Änderungen werden über die vorhandenen Config-Timertopics veröffentlicht.

## Statuswerte

Vollständiges Topic: `<basis>/status/<name>`.

### HK1 Status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK1_BW1_Ausschaltoptimierung` | `hc1_ov1_off_time_optimization` | `0` / `1` |
| `HK1_BW1_Einschaltoptimierung` | `hc1_ov1_on_time_optimization` | `0` / `1` |
| `HK1_BW1_Automatik` | `hc1_ov1_automatic` | `0` / `1` |
| `HK1_BW1_Warmwasservorrang` | `hc1_ov1_ww_priority` | `0` / `1` |
| `HK1_BW1_Estrichtrocknung` | `hc1_ov1_screed_drying` | `0` / `1` |
| `HK1_BW1_Ferien` | `hc1_ov1_holiday` | `0` / `1` |
| `HK1_BW1_Frostschutz` | `hc1_ov1_frost_protection` | `0` / `1` |
| `HK1_BW1_Manuell` | `hc1_ov1_manual` | `0` / `1` |
| `HK1_BW2_Sommer` | `hc1_ov2_summer` | `0` / `1` |
| `HK1_BW2_Tag` | `hc1_ov2_day` | `0` / `1` |
| `HK1_BW2_Keine_Komm_mit_FB` | `hc1_ov2_no_conn_to_remotectrl` | `0` / `1` |
| `HK1_BW2_FB_fehlerhaft` | `hc1_ov2_remotectrl_error` | `0` / `1` |
| `HK1_BW2_Fehler_Vorlauffuehler` | `hc1_ov2_failure_flow_sensor` | `0` / `1` |
| `HK1_BW2_Maximaler_Vorlauf` | `hc1_ov2_flow_at_maximum` | `0` / `1` |
| `HK1_BW2_Externer_Stoereingang` | `hc1_ov2_external_signal_input` | `0` / `1` |
| `HK1_Vorlaufsolltemperatur` | `hc1_flow_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Vorlaufisttemperatur` | `hc1_flow_temp` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Raumsolltemperatur` | `hc1_room_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Raumisttemperatur` | `hc1_room_temp` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Einschaltoptimierung` | `hc1_on_time_opt_duration` | Zahl, Minuten |
| `HK1_Ausschaltoptimierung` | `hc1_off_time_opt_duration` | Zahl, Minuten |
| `HK1_Pumpe` | `hc1_pump` | Zahl, % (Mischer vorzeichenbehaftet) |
| `HK1_Mischerstellung` | `hc1_mixer` | Zahl, % (Mischer vorzeichenbehaftet) |
| `HK1_Heizkennlinie_10_Grad` | `hc1_heat_curve_10C` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Heizkennlinie_0_Grad` | `hc1_heat_curve_0C` | Zahl, °C (ohne Einheit im Payload) |
| `HK1_Heizkennlinie_-10_Grad` | `hc1_heat_curve_-10C` | Zahl, °C (ohne Einheit im Payload) |

### HK2 Status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK2_BW1_Ausschaltoptimierung` | `hc2_ov1_off_time_opt` | `0` / `1` |
| `HK2_BW1_Einschaltoptimierung` | `hc2_ov1_on_time_opt` | `0` / `1` |
| `HK2_BW1_Automatik` | `hc2_ov1_automatic` | `0` / `1` |
| `HK2_BW1_Warmwasservorrang` | `hc2_ov1_ww_priority` | `0` / `1` |
| `HK2_BW1_Estrichtrocknung` | `hc2_ov1_screed_drying` | `0` / `1` |
| `HK2_BW1_Ferien` | `hc2_ov1_holiday` | `0` / `1` |
| `HK2_BW1_Frostschutz` | `hc2_ov1_frost_protection` | `0` / `1` |
| `HK2_BW1_Manuell` | `hc2_ov1_manual` | `0` / `1` |
| `HK2_BW2_Sommer` | `hc2_ov2_summer` | `0` / `1` |
| `HK2_BW2_Tag` | `hc2_ov2_day` | `0` / `1` |
| `HK2_BW2_Keine_Komm_mit_FB` | `hc2_ov2_no_conn_to_remotectrl` | `0` / `1` |
| `HK2_BW2_FB_fehlerhaft` | `hc2_ov2_remotectrl_error` | `0` / `1` |
| `HK2_BW2_Fehler_Vorlauffuehler` | `hc2_ov2_failure_flow_sensor` | `0` / `1` |
| `HK2_BW2_Maximaler_Vorlauf` | `hc2_ov2_flow_at_maximum` | `0` / `1` |
| `HK2_BW2_Externer_Stoereingang` | `hc2_ov2_external_signal_input` | `0` / `1` |
| `HK2_Vorlaufsolltemperatur` | `hc2_flow_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Vorlaufisttemperatur` | `hc2_flow_temp` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Raumsolltemperatur` | `hc2_room_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Raumisttemperatur` | `hc2_room_temp` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Einschaltoptimierung` | `hc2_on_time_opt_duration` | Zahl, Minuten |
| `HK2_Ausschaltoptimierung` | `hc2_off_time_opt_duration` | Zahl, Minuten |
| `HK2_Pumpe` | `hc2_pump` | Zahl, % (Mischer vorzeichenbehaftet) |
| `HK2_Mischerstellung` | `hc2_mixer` | Zahl, % (Mischer vorzeichenbehaftet) |
| `HK2_Heizkennlinie_10_Grad` | `hc2_heat_curve_10C` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Heizkennlinie_0_Grad` | `hc2_heat_curve_0C` | Zahl, °C (ohne Einheit im Payload) |
| `HK2_Heizkennlinie_-10_Grad` | `hc2_heat_curve_-10C` | Zahl, °C (ohne Einheit im Payload) |

### Warmwasser Status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `WW_BW1_Automatik` | `ww_ov1_auto` | `0` / `1` |
| `WW_BW1_Desinfektion` | `ww_ov1_disinfection` | `0` / `1` |
| `WW_BW1_Nachladung` | `ww_ov1_reload` | `0` / `1` |
| `WW_BW1_Ferien` | `ww_ov1_holiday` | `0` / `1` |
| `WW_BW1_Fehler_Desinfektion` | `ww_ov1_err_disinfection` | `0` / `1` |
| `WW_BW1_Fehler_Fuehler` | `ww_ov1_err_sensor` | `0` / `1` |
| `WW_BW1_Fehler_WW_bleibt_kalt` | `ww_ov1_ww_stays_cold` | `0` / `1` |
| `WW_BW1_Fehler_Anode` | `ww_ov1_err_anode` | `0` / `1` |
| `WW_BW2_Laden` | `ww_ov2_load` | `0` / `1` |
| `WW_BW2_Manuell` | `ww_ov2_manual` | `0` / `1` |
| `WW_BW2_Nachladen` | `ww_ov2_reload` | `0` / `1` |
| `WW_BW2_Ausschaltoptimierung` | `ww_ov2_off_time_opt_duration` | `0` / `1` |
| `WW_BW2_Einschaltoptimierung` | `ww_ov2_on_time_opt_duration` | `0` / `1` |
| `WW_BW2_Tag` | `ww_ov2_day` | `0` / `1` |
| `WW_BW2_Warm` | `ww_ov2_hot` | `0` / `1` |
| `WW_BW2_Vorrang` | `ww_ov2_priority` | `0` / `1` |
| `WW_Solltemperatur` | `ww_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `WW_Isttemperatur` | `ww_temp` | Zahl, °C (ohne Einheit im Payload) |
| `WW_Einschaltoptimierung` | `ww_on_time_opt_duration` | Zahl, Minuten |
| `WW_Pumpentyp_Ladepumpe` | `ww_pump_type_charge` | `0` / `1` |
| `WW_Pumpentyp_Zirkulationspumpe` | `ww_pump_type_circulation` | `0` / `1` |
| `WW_Pumpentyp_Absenkung_Solar` | `ww_pump_type_groundwater_solar` | `0` / `1` |

### Solar Status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Solar_Ladung` | `solar_load` | Ganzzahl |
| `Solar_Warmwasser` | `solar_ww` | Zahl, °C (ohne Einheit im Payload) |
| `Solar_Kollektor` | `solar_collector` | Zahl, °C (ohne Einheit im Payload) |
| `Solar_Laufzeit` | `solar_runtime` | Zahl, Minuten |
| `Solar_9147` | `solar_9147` | Ganzzahl (Solar_9147: Bedeutung unbekannt) |

### Allgemeine Regelungsdaten Status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Kessel_Vorlaufsolltemperatur` | `boiler_setpoint` | Zahl, °C (ohne Einheit im Payload) |
| `Kessel_Vorlaufisttemperatur` | `boiler_temp` | Zahl, °C (ohne Einheit im Payload) |
| `Brenner_Einschalttemperatur` | `boiler_switch_on_temp` | Zahl, °C (ohne Einheit im Payload) |
| `Brenner_Ausschalttemperatur` | `boiler_switch_off_temp` | Zahl, °C (ohne Einheit im Payload) |
| `Kessel_Fehler_Brennerstoerung` | `boiler_failure_burner` | `0` / `1` |
| `Kessel_Fehler_Kesselfuehler` | `boiler_failure_boiler_sensor` | `0` / `1` |
| `Kessel_Fehler_Zusatzfuehler` | `boiler_failure_aux_sensor` | `0` / `1` |
| `Kessel_Fehler_Kessel_bleibt_kalt` | `boiler_failure_boiler_stays_cold` | `0` / `1` |
| `Kessel_Fehler_Abgasfuehler` | `boiler_failure_exhaust_gas_sensor` | `0` / `1` |
| `Kessel_Fehler_Abgas_max_Grenzwert` | `boiler_failure_exhaust_gas_over_limit` | `0` / `1` |
| `Kessel_Fehler_Sicherungskette` | `boiler_failure_safety_chain` | `0` / `1` |
| `Kessel_Fehler_Externe_Stoerung` | `boiler_failure_external` | `0` / `1` |
| `Kessel_Betrieb_Abgastest` | `boiler_state_exhaust_gas_test` | `0` / `1` |
| `Kessel_Betrieb_Betrieb_Stufe1` | `boiler_state_stage1` | `0` / `1` |
| `Kessel_Betrieb_Kesselschutz` | `boiler_state_boiler_protection` | `0` / `1` |
| `Kessel_Betrieb_Unter_Betrieb` | `boiler_state_active` | `0` / `1` |
| `Kessel_Betrieb_Leistung_frei` | `boiler_state_performance_free` | `0` / `1` |
| `Kessel_Betrieb_Leistung_hoch` | `boiler_state_performance_high` | `0` / `1` |
| `Kessel_Betrieb_BetriebStufe2` | `boiler_state_stage2` | `0` / `1` |
| `Brenner_Ansteuerung` | `burner_control` | Rohwert: 0=aus, 1=Stufe 1, 4=Stufe 2 bzw. Modulation frei |
| `Abgastemperatur` | `exhaust_gas_temp` | Zahl, °C (ohne Einheit im Payload) |
| `Brenner_Laufzeit_Minuten65536` | `burner_runtime_minutes65536` | Rohbyte; Gewicht 65536 / 256 / 1 Minuten für Byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Minuten256` | `burner_runtime_minutes256` | Rohbyte; Gewicht 65536 / 256 / 1 Minuten für Byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Minuten` | `burner_runtime_minutes` | Rohbyte; Gewicht 65536 / 256 / 1 Minuten für Byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Summe` | `burner_runtime_overall` | Gesamtlaufzeit, Minuten |
| `Oelverbrauch_Gesamt_berechnet` | `oil_consumption_overall_calc` | Zahl, berechnete Liter; keine Messung |
| `Aussentemperatur` | `outside_temp` | Zahl, °C (ohne Einheit im Payload) |
| `Aussentemperatur_gedaempft` | `outside_temp_damped` | Zahl, °C (ohne Einheit im Payload) |
| `Versionsnummer_VK` | `version_vk` | Ganzzahl |
| `Versionsnummer_NK` | `version_nk` | Ganzzahl |
| `Modulkennung` | `module_id` | Ganzzahl |
| `ERR_Alarmstatus_Abgasfuehler` | `err_alarm_exhaust` | `0` / `1` |
| `ERR_Alarmstatus_02` | `err_alarm_02` | `0` / `1` |
| `ERR_Alarmstatus_Kesselvorlauffuehler` | `err_alarm_boiler_flow_sensor` | `0` / `1` |
| `ERR_Alarmstatus_08` | `err_alarm_08` | `0` / `1` |
| `ERR_Alarmstatus_Brenner` | `err_alarm_burner` | `0` / `1` |
| `ERR_Alarmstatus_20` | `err_alarm_20` | `0` / `1` |
| `ERR_Alarmstatus_HK2-Vorlauffuehler` | `err_alarm_HK2-flow_sensor` | `0` / `1` |
| `ERR_Alarmstatus_80` | `err_alarm_80` | `0` / `1` |

## Alarmmeldungen

Vollständiges Topic: `<basis>/alarm/<name>`.

| Topic DE | Topic EN |
| --- | --- |
| `Fehlerspeicher1` | `error_buffer1` |
| `Fehlerspeicher2` | `error_buffer2` |
| `Fehlerspeicher3` | `error_buffer3` |
| `Fehlerspeicher4` | `error_buffer4` |

Payloads enthalten übersetzte Fehlertexte und relative Zeitangaben. `>>` kennzeichnet Gekommen, `<<` Gegangen. Fehlt die Gegangen-Zeit, ist die Meldung im Speicher noch offen; ein leerer Platz wird als „Kein Fehler“ gemeldet. Der Speicher umfasst vier Plätze und ist kein vollständiges Ereignisarchiv. Die `ERR_Alarmstatus_...`-Topics unter `status/` sind davon getrennte 0/1-Fehlerflags. Unbekannte Bits wie 02/08/20/80 haben keine gesicherte zusätzliche Bedeutung.

## Schreibbefehle

Gelesene Werte werden nicht auf demselben Topic zurückgeschrieben. Zum Setzen dient `<basis>/setvalue/...`, für Gerätebefehle `<basis>/cmd/...`. Beispiel: `esp_heizung/setvalue/hc1_day_temp` mit Payload `21.5` bei englischer MQTT-Sprache.

Die Tabelle enthält den Suffix **hinter `<basis>/`** und numerische Payloads. Temperaturen mit Nachkommastellen werden mit Dezimalpunkt übertragen. Unterstützte Textalternativen richten sich nach der MQTT-Sprache.

| Parameter | Topic DE | Topic EN | Payload |
| --- | --- | --- | --- |
| HK1: Betriebsart | `setvalue/HK1_Betriebsart` | `setvalue/hc1_operation_mode` | 0=Nacht, 1=Tag, 2=AUTO |
| HK1: Programm | `setvalue/HK1_Programm` | `setvalue/hc1_program` | 0=Eigen, 1=Familie, 2=Früh, 3=Spät, 4=Vormittag, 5=Nachmittag, 6=Mittag, 7=Single, 8=Senior |
| HK1: Tagtemperatur | `setvalue/HK1_Tagtemperatur` | `setvalue/hc1_day_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK1: Nachttemperatur | `setvalue/HK1_Nachttemperatur` | `setvalue/hc1_night_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK1: Urlaubtemperatur | `setvalue/HK1_Urlaubtemperatur` | `setvalue/hc1_holiday_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK1: Urlaubstage | `setvalue/HK1_Ferien_Tage` | `setvalue/hc1_holiday_days` | 0…99 Tage |
| HK1: Auslegung | `setvalue/HK1_Auslegung` | `setvalue/hc1_interpretation` | 30…90 °C, 1-°C-Schritte |
| HK1: Aufschalttemperatur | `setvalue/HK1_Aufschalttemperatur` | `setvalue/hc1_switch_on_temperature` | 0…10 °C, 1-°C-Schritte |
| HK1: Außenhalt ab | `setvalue/HK1_Aussenhalt_ab` | `setvalue/hc1_switch_off_threshold` | −20…10 °C, 1-°C-Schritte |
| HK1: Frostschutz | `setvalue/HK1_Frost_ab` | `setvalue/hc1_frost_protection_threshold` | −20…10 °C, 1-°C-Schritte |
| HK1: Sommer ab | `setvalue/HK1_Sommer_ab` | `setvalue/hc1_summer_mode_threshold` | 9=Sommer, 10…30 °C=Schwelle, 31=Winter |
| HK1: Absenkungsart | `setvalue/HK1_Absenkungsart` | `setvalue/hc1_reduction_mode` | 0=Abschalt, 1=Reduziert, 2=Raumhalt, 3=Außenhalt |
| HK1: Maximaltemperatur | `setvalue/HK1_Max_Temperatur` | `setvalue/hc1_max_temp` | 20…90 °C |
| HK1: Temperaturabgleich | `setvalue/HK1_Temperatur_Offset` | `setvalue/hc1_temp_offset` | −5…+5 °C, 0,5-°C-Schritte |
| HK1: Heizsystem | `setvalue/HK1_Heizsystem` | `setvalue/hc1_heating_system` | 0=Aus, 1=Heizkörper |
| HK2: Betriebsart | `setvalue/HK2_Betriebsart` | `setvalue/hc2_operation_mode` | 0=Nacht, 1=Tag, 2=AUTO |
| HK2: Programm | `setvalue/HK2_Programm` | `setvalue/hc2_program` | 0=Eigen, 1=Familie, 2=Früh, 3=Spät, 4=Vormittag, 5=Nachmittag, 6=Mittag, 7=Single, 8=Senior |
| HK2: Tagtemperatur | `setvalue/HK2_Tagtemperatur` | `setvalue/hc2_day_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK2: Nachttemperatur | `setvalue/HK2_Nachttemperatur` | `setvalue/hc2_night_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK2: Urlaubtemperatur | `setvalue/HK2_Urlaubtemperatur` | `setvalue/hc2_holiday_temp` | 10…30 °C, 0,5-°C-Schritte |
| HK2: Urlaubstage | `setvalue/HK2_Ferien_Tage` | `setvalue/hc2_holiday_days` | 0…99 Tage |
| HK2: Auslegung | `setvalue/HK2_Auslegung` | `setvalue/hc2_interpretation` | 30…90 °C, 1-°C-Schritte |
| HK2: Aufschalttemperatur | `setvalue/HK2_Aufschalttemperatur` | `setvalue/hc2_switch_on_temperature` | 0…10 °C, 1-°C-Schritte |
| HK2: Außenhalt ab | `setvalue/HK2_Aussenhalt_ab` | `setvalue/hc2_switch_off_threshold` | −20…10 °C, 1-°C-Schritte |
| HK2: Frostschutz | `setvalue/HK2_Frost_ab` | `setvalue/hc2_frost_protection_threshold` | −20…10 °C, 1-°C-Schritte |
| HK2: Sommer ab | `setvalue/HK2_Sommer_ab` | `setvalue/hc2_summer_mode_threshold` | 9=Sommer, 10…30 °C=Schwelle, 31=Winter |
| HK2: Absenkungsart | `setvalue/HK2_Absenkungsart` | `setvalue/hc2_reduction_mode` | 0=Abschalt, 1=Reduziert, 2=Raumhalt, 3=Außenhalt |
| HK2: Maximaltemperatur | `setvalue/HK2_Max_Temperatur` | `setvalue/hc2_max_temp` | 20…90 °C; HK2 Fußboden bis 60 °C |
| HK2: Temperaturabgleich | `setvalue/HK2_Temperatur_Offset` | `setvalue/hc2_temp_offset` | −5…+5 °C, 0,5-°C-Schritte |
| HK2: Heizsystem | `setvalue/HK2_Heizsystem` | `setvalue/hc2_heating_system` | 0=Aus, 1=Heizkörper; HK2 zusätzlich 3=Fußboden |
| Warmwasser: Betriebsart | `setvalue/WW_Betriebsart` | `setvalue/ww_operation_mode` | 0=Nacht, 1=Tag, 2=AUTO |
| Warmwasser: Temperatur | `setvalue/WW_Temperatur` | `setvalue/ww_temp` | 30…60 °C, 1-°C-Schritte |
| Warmwasser: Zirkulation | `setvalue/WW_Zirkulation` | `setvalue/ww_circulation` | 0=AUS, 1…6 Zyklen/Stunde, 7=EIN |
| Warmwasser: Bereitung aktiv | `setvalue/WW_Aufbereitung` | `setvalue/ww_processing` | 0=Aus, 1=An |
| Warmwasser: Vorrang für HK2 | `setvalue/WW_Vorrang` | `setvalue/ww_priority` | 0=Aus, 1=An |

### Geräte- und Diagnosebefehle

| Topic-Suffix | Payload / Wirkung |
| --- | --- |
| `setvalue/setdatetime` | Leer; aktuelle ESP-Zeit an die Logamatic übertragen |
| `setvalue/oilcounter` | Nicht negative Ganzzahl, z. B. `12345` für 123,45 L; ersetzt den Zählerstand |
| `cmd/restart` | Leer; ESP neu starten |
| `cmd/debug` | `0` / `1` für Debug-Aufzeichnung aus/ein |
| `cmd/setdebugflt` | 11 durch `_` getrennte Hex-Werte, `XX` als Platzhalter, z. B. `08_15_XX_XX_XX_XX_XX_XX_XX_XX_XX` |
| `cmd/getdebugflt` | Leer; aktuellen Filter abfragen |
| `cmd/simdata` | Leer; Simulationsdaten senden, nur bei aktivierter Simulation |
| `cmd/service` | Expertenfunktion: 8 durch `_` getrennte Hex-Werte als Rohbefehl; nur mit gesichertem Protokollwissen verwenden |

Befehle nicht mit Retain senden: Ein gespeicherter Stellbefehl kann beim Wiederverbinden erneut ausgeführt werden. Alte retained Nachrichten auf `setvalue`-/`cmd`-Topics müssen am Broker gelöscht werden; ein neuer Statuswert auf einem anderen Topic überschreibt sie nicht. Ungültige Ölzähler-Payloads wie `offline` werden verworfen und im Log gemeldet.

Die Liste umfasst 35 Heizungs-Konfigurationsbefehle sowie die Geräte-/Diagnosebefehle. Zahlen werden vollständig geprüft; ungültige Werte werden verworfen. Numerische Payloads aus der Tabelle benötigen keine Einheit. Auswahltexte aus der aktuellen MQTT-Sprache werden bei entsprechenden Auswahlbefehlen ebenfalls akzeptiert. Während eines Timerauftrags sind andere Schreibanforderungen gesperrt. Eine angenommene Queue-Anforderung bedeutet noch keine abgeschlossene Übertragung; Rückmeldungen im Log und auf `message` beachten.

## Beispiele

Bei englischer MQTT-Sprache und Basis-Topic `esp_heizung`:

```text
Topic                                              Payload
esp_heizung/setvalue/hc1_day_temp                   21.5
esp_heizung/setvalue/hc2_heating_system             3
esp_heizung/setvalue/ww_processing                  1
esp_heizung/setvalue/oilcounter                     12345
```

Der letzte Befehl setzt den Hardware-Zähler auf 123,45 L, nicht auf 12345 L. Der entsprechende Status wird unter `esp_heizung/oilcounter` ebenfalls als `12345` veröffentlicht. Auch die aktuelle WebUI-Eingabe erwartet Hundertstel Liter, während ihr Istwert in Litern angezeigt wird. Zum Beobachten aller Geräte-Topics `<basis>/#` abonnieren; dadurch werden keine neuen Messwerte angefordert.

## Home Assistant und Quellen

[Home-Assistant-Einrichtung und Entitätsmigration](../README_DE.md#home-assistant). Topic-Namen und Auswahltexte: [language.h](../include/language.h); Decodierung und Quittierungsnachführung: [km271.cpp](../src/km271.cpp); Befehlsverarbeitung: [mqttCustom.cpp](../src/mqttCustom.cpp); Veröffentlichung und Retain: [message.cpp](../src/message.cpp).
