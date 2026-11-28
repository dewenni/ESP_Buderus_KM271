# MQTT reference

[Back to the guide](../README.md) · [Deutsch](MQTT_DE.md)

This reference combines the former `param.txt` information with the current command and topic definitions for firmware v6.1.0. The tables cover all 79 configuration topics, 118 status topics and four error-memory topics. Publication depends on values received from the controller and enabled features.

## Contents

- [Setup and topic structure](#setup-and-topic-structure)
- [Configuration values](#configuration-values)
- [Status values](#status-values)
- [Alarm messages](#alarm-messages)
- [Write commands](#write-commands)
- [Examples](#examples)
- [Home Assistant and sources](#home-assistant-and-sources)

## Setup and topic structure

Enable MQTT under **Settings → Integrations → MQTT** and configure broker, port, credentials and base topic. WebUI and MQTT language are independent. A language change can change topic names and text values; check existing automations. Below, `<base>` means the configured base topic, for example `esp_heizung`.

| Topic | Contents | Retain |
| --- | --- | --- |
| `<base>/config/<name>` | Logamatic configuration, text with units or choice labels | configurable |
| `<base>/status/<name>` | Logamatic measurements and operating states | no |
| `<base>/alarm/<name>` | Four error-memory slots, open and cleared messages | no |
| `<base>/sensor/<name>` | Optional OneWire sensor, temperature in °C as a number without unit | no |
| `<base>/oilcounter` | Hardware counter in hundredths of a litre | yes |
| `<base>/status` | ESP availability: `online` / `offline` (last will); not proof of a Logamatic connection | offline: yes; online: no |
| `<base>/wifi` | JSON: `status`, `rssi`, `signal`, `ip`, `date_time` | no |
| `<base>/eth` | JSON: `status`, `ip`, `link_up`, `link_speed`, `full_duplex`, `date_time` | no |
| `<base>/sysinfo` | JSON: `uptime`, `restart_reason`, `heap`, `flash` | no |
| `<base>/info` | JSON: `burner`, `pump`, `ww_temp`, `boiler_temp` | no |
| `<base>/message` | Command feedback and operational messages | no |
| `<base>/debug` | JSON: `logmode`, `rx_age_s`, `send_cmd_busy`, `sw_version`, `date-time` | no |
| `<base>/debug_message` | Filtered diagnostic telegrams and write diagnostics | no |
| `<base>/undefined_message` | Unknown telegrams | no |

`status` and `status/<name>` are different topics. Numbers use a decimal point. Configuration payloads can contain text such as `21.5 °C`, while temperature status values have no unit suffix. Units in the following status tables explain the number and are not part of its payload.

Configuration values come from received telegrams or are updated as assumed accepted after a successful write acknowledgement. An acknowledgement confirms transport, not independent read-back. Subsequent controller reports override the assumption. A readable value is not automatically writable; use only the command list below.

## Configuration values

Complete topic: `<base>/config/<name>`.

### HC1 configuration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK1_Frost_ab` | `hc1_frost_protection_threshold` | number with `°C` |
| `HK1_Sommer_ab` | `hc1_summer_mode_threshold` | summer / 10…30 °C / winter |
| `HK1_Nachttemperatur` | `hc1_night_temp` | number with `°C` |
| `HK1_Tagtemperatur` | `hc1_day_temp` | number with `°C` |
| `HK1_Betriebsart` | `hc1_operation_mode` | night / day / auto |
| `HK1_Urlaubtemperatur` | `hc1_holiday_temp` | number with `°C` |
| `HK1_Max_Temperatur` | `hc1_max_temp` | number with `°C` |
| `HK1_Auslegung` | `hc1_interpretation` | number with `°C` |
| `HK1_Aufschalttemperatur` | `hc1_switch_on_temperature` | off or 1…10 °C |
| `HK1_Aussenhalt_ab` | `hc1_switch_off_threshold` | number with `°C` |
| `HK1_Absenkungsart` | `hc1_reduction_mode` | off / fixed / room / outdoors |
| `HK1_Heizsystem` | `hc1_heating_system` | off / radiator / - / underfloor |
| `HK1_Temperatur_Offset` | `hc1_temp_offset` | number with `°C` |
| `HK1_Fernbedienung` | `hc1_remotecontrol` | off / on |
| `HK1_Programm` | `hc1_program` | custom / family / early / late / AM / PM / noon / single / senior |
| `HK1_Ferien_Tage` | `hc1_holiday_days` | number with `days` |
| `HK1_Timer01` | `hc1_timer01` | Three formatted switching points; see timer format |
| `HK1_Timer02` | `hc1_timer02` | Three formatted switching points; see timer format |
| `HK1_Timer03` | `hc1_timer03` | Three formatted switching points; see timer format |
| `HK1_Timer04` | `hc1_timer04` | Three formatted switching points; see timer format |
| `HK1_Timer05` | `hc1_timer05` | Three formatted switching points; see timer format |
| `HK1_Timer06` | `hc1_timer06` | Three formatted switching points; see timer format |
| `HK1_Timer07` | `hc1_timer07` | Three formatted switching points; see timer format |
| `HK1_Timer08` | `hc1_timer08` | Three formatted switching points; see timer format |
| `HK1_Timer09` | `hc1_timer09` | Three formatted switching points; see timer format |
| `HK1_Timer10` | `hc1_timer10` | Three formatted switching points; see timer format |
| `HK1_Timer11` | `hc1_timer11` | Three formatted switching points; see timer format |
| `HK1_Timer12` | `hc1_timer12` | Three formatted switching points; see timer format |
| `HK1_Timer13` | `hc1_timer13` | Three formatted switching points; see timer format |
| `HK1_Timer14` | `hc1_timer14` | Three formatted switching points; see timer format |

### HC2 configuration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `HK2_Frost_ab` | `hc2_frost_protection_threshold` | number with `°C` |
| `HK2_Sommer_ab` | `hc2_summer_mode_threshold` | summer / 10…30 °C / winter |
| `HK2_Nachttemperatur` | `hc2_night_temp` | number with `°C` |
| `HK2_Tagtemperatur` | `hc2_day_temp` | number with `°C` |
| `HK2_Betriebsart` | `hc2_operation_mode` | night / day / auto |
| `HK2_Urlaubtemperatur` | `hc2_holiday_temp` | number with `°C` |
| `HK2_Max_Temperatur` | `hc2_max_temp` | number with `°C` |
| `HK2_Auslegung` | `hc2_interpretation` | number with `°C` |
| `HK2_Aufschalttemperatur` | `hc2_switch_on_temperature` | off or 1…10 °C |
| `HK2_Aussenhalt_ab` | `hc2_switch_off_threshold` | number with `°C` |
| `HK2_Absenkungsart` | `hc2_reduction_mode` | off / fixed / room / outdoors |
| `HK2_Heizsystem` | `hc2_heating_system` | off / radiator / - / underfloor |
| `HK2_Temperatur_Offset` | `hc2_temp_offset` | number with `°C` |
| `HK2_Fernbedienung` | `hc2_remotecontrol` | off / on |
| `HK2_Programm` | `hc2_program` | custom / family / early / late / AM / PM / noon / single / senior |
| `HK2_Ferien_Tage` | `hc2_holiday_days` | number with `days` |
| `HK2_Timer01` | `hc2_timer01` | Three formatted switching points; see timer format |
| `HK2_Timer02` | `hc2_timer02` | Three formatted switching points; see timer format |
| `HK2_Timer03` | `hc2_timer03` | Three formatted switching points; see timer format |
| `HK2_Timer04` | `hc2_timer04` | Three formatted switching points; see timer format |
| `HK2_Timer05` | `hc2_timer05` | Three formatted switching points; see timer format |
| `HK2_Timer06` | `hc2_timer06` | Three formatted switching points; see timer format |
| `HK2_Timer07` | `hc2_timer07` | Three formatted switching points; see timer format |
| `HK2_Timer08` | `hc2_timer08` | Three formatted switching points; see timer format |
| `HK2_Timer09` | `hc2_timer09` | Three formatted switching points; see timer format |
| `HK2_Timer10` | `hc2_timer10` | Three formatted switching points; see timer format |
| `HK2_Timer11` | `hc2_timer11` | Three formatted switching points; see timer format |
| `HK2_Timer12` | `hc2_timer12` | Three formatted switching points; see timer format |
| `HK2_Timer13` | `hc2_timer13` | Three formatted switching points; see timer format |
| `HK2_Timer14` | `hc2_timer14` | Three formatted switching points; see timer format |

### Hot water configuration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `WW_Vorrang` | `ww_priority` | off / on |
| `WW_Temperatur` | `ww_temp` | number with `°C` |
| `WW_Betriebsart` | `ww_operation_mode` | night / day / auto |
| `WW_Aufbereitung` | `ww_processing` | off / on |
| `WW_Zirkulation` | `ww_circulation` | off / 1…6 / on |

### Solar configuration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Solar_Betriebsart` | `solar_operation_mode` | night / day / auto |
| `Solar_Aktivierung` | `solar_enabled` | off / on |
| `Solar_Maximaltemperatur` | `solar_temp_max` | number with `°C` |
| `Solar_Minimaltemperatur` | `solar_temp_min` | number with `°C` |

### General controller data configuration

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Gebaeudeart` | `building_type` | light / medium / heavy |
| `Sprache` | `language` | DE / FR / IT / NL / EN / PL |
| `Anzeige` | `display` | auto / boiler / DHW / outdoor |
| `Brennerart` | `burner_type` | 1-stage / 2-stage / modulated |
| `Max_Kesseltemperatur` | `max_boiler_temperature` | number with `°C` |
| `Pumplogik` | `pump_logic_temp` | number with `°C` |
| `Abgastemperaturschwelle` | `exhaust_gas_temperature_threshold` | off or temperature text 50…250 (°C) |
| `Brenner_Min_Modulation` | `burner_min_modulation` | number with `%` |
| `Brenner_Mod_Laufzeit` | `burner_modulation_runtime` | number with `s` |
| `Uhrzeit_Offset` | `time_offset` | signed number with `s/d` |

### Timer format

`HK1_Timer01`…`HK1_Timer14` or `hc1_timer01`…`hc1_timer14` each contain three switching points of the custom program. HC2 has the same 14 blocks. Example in German MQTT language: `SP01: Mo 05:40 (An) | SP02: Mo 22:10 (Aus) | SP03: Di 05:30 (An)`. Day and on/off labels follow MQTT language; unused points are labelled free. Each circuit has 42 switching points.

The WebUI edits these raw data through its weekly program editor. There is no MQTT write command for individual timer blocks; acknowledged changes are published on the existing configuration timer topics.

## Status values

Complete topic: `<base>/status/<name>`.

### HC1 status

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
| `HK1_Vorlaufsolltemperatur` | `hc1_flow_setpoint` | number, °C (no unit in payload) |
| `HK1_Vorlaufisttemperatur` | `hc1_flow_temp` | number, °C (no unit in payload) |
| `HK1_Raumsolltemperatur` | `hc1_room_setpoint` | number, °C (no unit in payload) |
| `HK1_Raumisttemperatur` | `hc1_room_temp` | number, °C (no unit in payload) |
| `HK1_Einschaltoptimierung` | `hc1_on_time_opt_duration` | number, minutes |
| `HK1_Ausschaltoptimierung` | `hc1_off_time_opt_duration` | number, minutes |
| `HK1_Pumpe` | `hc1_pump` | number, % (mixer signed) |
| `HK1_Mischerstellung` | `hc1_mixer` | number, % (mixer signed) |
| `HK1_Heizkennlinie_10_Grad` | `hc1_heat_curve_10C` | number, °C (no unit in payload) |
| `HK1_Heizkennlinie_0_Grad` | `hc1_heat_curve_0C` | number, °C (no unit in payload) |
| `HK1_Heizkennlinie_-10_Grad` | `hc1_heat_curve_-10C` | number, °C (no unit in payload) |

### HC2 status

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
| `HK2_Vorlaufsolltemperatur` | `hc2_flow_setpoint` | number, °C (no unit in payload) |
| `HK2_Vorlaufisttemperatur` | `hc2_flow_temp` | number, °C (no unit in payload) |
| `HK2_Raumsolltemperatur` | `hc2_room_setpoint` | number, °C (no unit in payload) |
| `HK2_Raumisttemperatur` | `hc2_room_temp` | number, °C (no unit in payload) |
| `HK2_Einschaltoptimierung` | `hc2_on_time_opt_duration` | number, minutes |
| `HK2_Ausschaltoptimierung` | `hc2_off_time_opt_duration` | number, minutes |
| `HK2_Pumpe` | `hc2_pump` | number, % (mixer signed) |
| `HK2_Mischerstellung` | `hc2_mixer` | number, % (mixer signed) |
| `HK2_Heizkennlinie_10_Grad` | `hc2_heat_curve_10C` | number, °C (no unit in payload) |
| `HK2_Heizkennlinie_0_Grad` | `hc2_heat_curve_0C` | number, °C (no unit in payload) |
| `HK2_Heizkennlinie_-10_Grad` | `hc2_heat_curve_-10C` | number, °C (no unit in payload) |

### Hot water status

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
| `WW_Solltemperatur` | `ww_setpoint` | number, °C (no unit in payload) |
| `WW_Isttemperatur` | `ww_temp` | number, °C (no unit in payload) |
| `WW_Einschaltoptimierung` | `ww_on_time_opt_duration` | number, minutes |
| `WW_Pumpentyp_Ladepumpe` | `ww_pump_type_charge` | `0` / `1` |
| `WW_Pumpentyp_Zirkulationspumpe` | `ww_pump_type_circulation` | `0` / `1` |
| `WW_Pumpentyp_Absenkung_Solar` | `ww_pump_type_groundwater_solar` | `0` / `1` |

### Solar status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Solar_Ladung` | `solar_load` | integer |
| `Solar_Warmwasser` | `solar_ww` | number, °C (no unit in payload) |
| `Solar_Kollektor` | `solar_collector` | number, °C (no unit in payload) |
| `Solar_Laufzeit` | `solar_runtime` | number, minutes |
| `Solar_9147` | `solar_9147` | integer (Solar_9147: meaning unknown) |

### General controller data status

| Topic DE | Topic EN | Payload |
| --- | --- | --- |
| `Kessel_Vorlaufsolltemperatur` | `boiler_setpoint` | number, °C (no unit in payload) |
| `Kessel_Vorlaufisttemperatur` | `boiler_temp` | number, °C (no unit in payload) |
| `Brenner_Einschalttemperatur` | `boiler_switch_on_temp` | number, °C (no unit in payload) |
| `Brenner_Ausschalttemperatur` | `boiler_switch_off_temp` | number, °C (no unit in payload) |
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
| `Brenner_Ansteuerung` | `burner_control` | raw value: 0=off, 1=stage 1, 4=stage 2 or modulation enabled |
| `Abgastemperatur` | `exhaust_gas_temp` | number, °C (no unit in payload) |
| `Brenner_Laufzeit_Minuten65536` | `burner_runtime_minutes65536` | raw byte; weight 65536 / 256 / 1 minutes for byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Minuten256` | `burner_runtime_minutes256` | raw byte; weight 65536 / 256 / 1 minutes for byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Minuten` | `burner_runtime_minutes` | raw byte; weight 65536 / 256 / 1 minutes for byte 1 / 2 / 3 |
| `Brenner_Laufzeit_Summe` | `burner_runtime_overall` | total runtime, minutes |
| `Oelverbrauch_Gesamt_berechnet` | `oil_consumption_overall_calc` | number, calculated litres; not measured |
| `Aussentemperatur` | `outside_temp` | number, °C (no unit in payload) |
| `Aussentemperatur_gedaempft` | `outside_temp_damped` | number, °C (no unit in payload) |
| `Versionsnummer_VK` | `version_vk` | integer |
| `Versionsnummer_NK` | `version_nk` | integer |
| `Modulkennung` | `module_id` | integer |
| `ERR_Alarmstatus_Abgasfuehler` | `err_alarm_exhaust` | `0` / `1` |
| `ERR_Alarmstatus_02` | `err_alarm_02` | `0` / `1` |
| `ERR_Alarmstatus_Kesselvorlauffuehler` | `err_alarm_boiler_flow_sensor` | `0` / `1` |
| `ERR_Alarmstatus_08` | `err_alarm_08` | `0` / `1` |
| `ERR_Alarmstatus_Brenner` | `err_alarm_burner` | `0` / `1` |
| `ERR_Alarmstatus_20` | `err_alarm_20` | `0` / `1` |
| `ERR_Alarmstatus_HK2-Vorlauffuehler` | `err_alarm_HK2-flow_sensor` | `0` / `1` |
| `ERR_Alarmstatus_80` | `err_alarm_80` | `0` / `1` |

## Alarm messages

Complete topic: `<base>/alarm/<name>`.

| Topic DE | Topic EN |
| --- | --- |
| `Fehlerspeicher1` | `error_buffer1` |
| `Fehlerspeicher2` | `error_buffer2` |
| `Fehlerspeicher3` | `error_buffer3` |
| `Fehlerspeicher4` | `error_buffer4` |

Payloads contain translated fault text and relative time information. `>>` marks occurrence, `<<` clearing. A message without a cleared time is still open in memory; an empty slot is reported as “No error”. There are four slots, not a complete event archive. The `err_alarm_...` topics under `status/` are separate 0/1 fault flags. Unknown bits such as 02/08/20/80 have no confirmed additional meaning.

## Write commands

Read values are not written back to the same topic. Use `<base>/setvalue/...` to set parameters and `<base>/cmd/...` for device commands. Example: `esp_heizung/setvalue/hc1_day_temp` with payload `21.5` when English MQTT topics are selected.

The table lists the suffix **after `<base>/`** and numeric payloads. Use a decimal point for fractional temperatures. Supported text alternatives depend on the selected MQTT language.

| Parameter | DE topic | EN topic | Payload |
| --- | --- | --- | --- |
| HC1: Operating mode | `setvalue/HK1_Betriebsart` | `setvalue/hc1_operation_mode` | 0=night, 1=day, 2=AUTO |
| HC1: Program | `setvalue/HK1_Programm` | `setvalue/hc1_program` | 0=custom, 1=family, 2=early, 3=late, 4=AM, 5=PM, 6=noon, 7=single, 8=senior |
| HC1: Day temperature | `setvalue/HK1_Tagtemperatur` | `setvalue/hc1_day_temp` | 10…30 °C, 0.5 °C steps |
| HC1: Night temperature | `setvalue/HK1_Nachttemperatur` | `setvalue/hc1_night_temp` | 10…30 °C, 0.5 °C steps |
| HC1: Holiday temperature | `setvalue/HK1_Urlaubtemperatur` | `setvalue/hc1_holiday_temp` | 10…30 °C, 0.5 °C steps |
| HC1: Holiday days | `setvalue/HK1_Ferien_Tage` | `setvalue/hc1_holiday_days` | 0…99 days |
| HC1: Design temperature | `setvalue/HK1_Auslegung` | `setvalue/hc1_interpretation` | 30…90 °C, 1 °C steps |
| HC1: Switch-on temperature | `setvalue/HK1_Aufschalttemperatur` | `setvalue/hc1_switch_on_temperature` | 0…10 °C, 1 °C steps |
| HC1: Reduction switch-off threshold | `setvalue/HK1_Aussenhalt_ab` | `setvalue/hc1_switch_off_threshold` | −20…10 °C, 1 °C steps |
| HC1: Frost protection | `setvalue/HK1_Frost_ab` | `setvalue/hc1_frost_protection_threshold` | −20…10 °C, 1 °C steps |
| HC1: Summer threshold | `setvalue/HK1_Sommer_ab` | `setvalue/hc1_summer_mode_threshold` | 9=summer, 10…30 °C=threshold, 31=winter |
| HC1: Reduction mode | `setvalue/HK1_Absenkungsart` | `setvalue/hc1_reduction_mode` | 0=off, 1=fixed, 2=room, 3=outdoors |
| HC1: Maximum temperature | `setvalue/HK1_Max_Temperatur` | `setvalue/hc1_max_temp` | 20…90 °C |
| HC1: Temperature adjustment | `setvalue/HK1_Temperatur_Offset` | `setvalue/hc1_temp_offset` | −5…+5 °C, 0.5 °C steps |
| HC1: Heating system | `setvalue/HK1_Heizsystem` | `setvalue/hc1_heating_system` | 0=off, 1=radiator |
| HC2: Operating mode | `setvalue/HK2_Betriebsart` | `setvalue/hc2_operation_mode` | 0=night, 1=day, 2=AUTO |
| HC2: Program | `setvalue/HK2_Programm` | `setvalue/hc2_program` | 0=custom, 1=family, 2=early, 3=late, 4=AM, 5=PM, 6=noon, 7=single, 8=senior |
| HC2: Day temperature | `setvalue/HK2_Tagtemperatur` | `setvalue/hc2_day_temp` | 10…30 °C, 0.5 °C steps |
| HC2: Night temperature | `setvalue/HK2_Nachttemperatur` | `setvalue/hc2_night_temp` | 10…30 °C, 0.5 °C steps |
| HC2: Holiday temperature | `setvalue/HK2_Urlaubtemperatur` | `setvalue/hc2_holiday_temp` | 10…30 °C, 0.5 °C steps |
| HC2: Holiday days | `setvalue/HK2_Ferien_Tage` | `setvalue/hc2_holiday_days` | 0…99 days |
| HC2: Design temperature | `setvalue/HK2_Auslegung` | `setvalue/hc2_interpretation` | 30…90 °C, 1 °C steps |
| HC2: Switch-on temperature | `setvalue/HK2_Aufschalttemperatur` | `setvalue/hc2_switch_on_temperature` | 0…10 °C, 1 °C steps |
| HC2: Reduction switch-off threshold | `setvalue/HK2_Aussenhalt_ab` | `setvalue/hc2_switch_off_threshold` | −20…10 °C, 1 °C steps |
| HC2: Frost protection | `setvalue/HK2_Frost_ab` | `setvalue/hc2_frost_protection_threshold` | −20…10 °C, 1 °C steps |
| HC2: Summer threshold | `setvalue/HK2_Sommer_ab` | `setvalue/hc2_summer_mode_threshold` | 9=summer, 10…30 °C=threshold, 31=winter |
| HC2: Reduction mode | `setvalue/HK2_Absenkungsart` | `setvalue/hc2_reduction_mode` | 0=off, 1=fixed, 2=room, 3=outdoors |
| HC2: Maximum temperature | `setvalue/HK2_Max_Temperatur` | `setvalue/hc2_max_temp` | 20…90 °C; HC2 underfloor up to 60 °C |
| HC2: Temperature adjustment | `setvalue/HK2_Temperatur_Offset` | `setvalue/hc2_temp_offset` | −5…+5 °C, 0.5 °C steps |
| HC2: Heating system | `setvalue/HK2_Heizsystem` | `setvalue/hc2_heating_system` | 0=off, 1=radiator; HC2 additionally 3=underfloor |
| Hot water: operating mode | `setvalue/WW_Betriebsart` | `setvalue/ww_operation_mode` | 0=night, 1=day, 2=AUTO |
| Hot water: temperature | `setvalue/WW_Temperatur` | `setvalue/ww_temp` | 30…60 °C, 1 °C steps |
| Hot water: circulation | `setvalue/WW_Zirkulation` | `setvalue/ww_circulation` | 0=OFF, 1…6 cycles/hour, 7=ON |
| Hot water: preparation enabled | `setvalue/WW_Aufbereitung` | `setvalue/ww_processing` | 0=off, 1=on |
| Hot water: priority for HC2 | `setvalue/WW_Vorrang` | `setvalue/ww_priority` | 0=off, 1=on |

### Device and diagnostic commands

| Topic suffix | Payload / effect |
| --- | --- |
| `setvalue/setdatetime` | Empty; send the current ESP time to the Logamatic |
| `setvalue/oilcounter` | Non-negative integer, e.g. `12345` for 123.45 L; replaces the counter value |
| `cmd/restart` | Empty; restart the ESP |
| `cmd/debug` | `0` / `1` for debug recording off/on |
| `cmd/setdebugflt` | 11 hex values separated by `_`, with `XX` as a wildcard, e.g. `08_15_XX_XX_XX_XX_XX_XX_XX_XX_XX` |
| `cmd/getdebugflt` | Empty; request the current filter |
| `cmd/simdata` | Empty; send simulation data, only when simulation is enabled |
| `cmd/service` | Expert function: 8 hex values separated by `_` as a raw command; requires verified protocol knowledge |

Do not send commands with Retain: stored commands can execute again after reconnecting. Delete old retained messages on `setvalue`/`cmd` topics at the broker; a new status value on another topic does not overwrite them. Invalid oil-counter payloads such as `offline` are rejected and logged.

The list includes 35 heating configuration commands plus device/diagnostic commands. Numeric input is fully validated; invalid values are rejected. Numeric payloads from the table do not need a unit. Choice labels in the current MQTT language are also accepted for applicable selection commands. Other writes are blocked during a timer request. Queue acceptance is not completed transmission; check the log and `message` feedback.

## Examples

With English MQTT language and base topic `esp_heizung`:

```text
Topic                                              Payload
esp_heizung/setvalue/hc1_day_temp                   21.5
esp_heizung/setvalue/hc2_heating_system             3
esp_heizung/setvalue/ww_processing                  1
esp_heizung/setvalue/oilcounter                     12345
```

The last command sets the hardware counter to 123.45 L, not 12345 L. Its status is published as `12345` on `esp_heizung/oilcounter` as well. The current WebUI input also expects hundredths of a litre, while its actual-value display uses litres. Subscribe to `<base>/#` to observe all device topics; subscribing does not request new measurements.

## Home Assistant and sources

[Home Assistant setup and entity migration](../README.md#home-assistant). Topic names and choice labels: [language.h](../include/language.h); decoding and acknowledgement updates: [km271.cpp](../src/km271.cpp); command handling: [mqttCustom.cpp](../src/mqttCustom.cpp); publishing and retain: [message.cpp](../src/message.cpp).
