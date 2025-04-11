// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  control: {
    de: "Bedienung",
    en: "Control",
  },
  dashboard: {
    de: "Dashboard",
    en: "Dashboard",
  },
  auto: {
    de: "Auto",
    en: "Auto",
  },
  programs: {
    de: "Programme",
    en: "Programs",
  },
  config: {
    de: "Konfiguration",
    en: "Config",
  },
  status: {
    de: "Status",
    en: "Status",
  },
  op_values: {
    de: "Betriebswerte",
    en: "Operating-States",
  },
  temperatures: {
    de: "Temperaturen",
    en: "Temperatures",
  },
  day: {
    de: "Tag",
    en: "Day",
  },
  night: {
    de: "Nacht",
    en: "Night",
  },
  day_night: {
    de: "Tag/Nacht",
    en: "Day/Night",
  },
  summer_winter: {
    de: "Sommer/Winter",
    en: "Summer/Winter",
  },
  hc1: {
    de: "HK1",
    en: "HC1",
  },
  hc2: {
    de: "HK2",
    en: "HC2",
  },
  ww: {
    de: "WW",
    en: "WW",
  },
  act_value: {
    de: "Istwert",
    en: "Actual Value",
  },
  set_temp: {
    de: "Solltemperatur",
    en: "Set Temperature",
  },
  set_temp_c: {
    de: "Solltemperatur \u00b0C",
    en: "Set Temperature \u00b0C",
  },
  act_temp: {
    de: "Isttemperatur",
    en: "Actual Temperature",
  },
  act_temp_c: {
    de: "Isttemperatur \u00b0C",
    en: "Actual Temperature \u00b0C",
  },
  opmode: {
    de: "Betriebsart",
    en: "Operation Mode",
  },
  error_flags: {
    de: "Fehlermeldungen",
    en: "Error Flags",
  },
  burner: {
    de: "Brenner",
    en: "Burner",
  },
  pump: {
    de: "Umw\u00e4lzpumpe",
    en: "Flow Pump",
  },
  hw: {
    de: "WW",
    en: "HW",
  },
  hot_water: {
    de: "Warmwasser",
    en: "Hot Water",
  },
  burner_temp: {
    de: "Kessel-Vorlauf",
    en: "Boiler Temperature",
  },
  flow: {
    de: "Vorlauf",
    en: "Flow Temperature",
  },
  heating_circuit_1: {
    de: "Heizkreis 1",
    en: "Heating Circuit 1",
  },
  heating_circuit_2: {
    de: "Heizkreis 2",
    en: "Heating Circuit 2",
  },
  general: {
    de: "Allgemeine Werte",
    en: "General Values",
  },
  logamatic_version: {
    de: "Logamatic-Version",
    en: "Logamatic-Version",
  },
  logamatic_modul: {
    de: "Logamatic-Modul",
    en: "Logamatic-Module",
  },
  prg: {
    de: "Programm",
    en: "Program",
  },
  hc_prg_custom: {
    de: "Eigen",
    en: "custom",
  },
  hc_prg_family: {
    de: "Familie",
    en: "family",
  },
  hc_prg_early: {
    de: "Frueh",
    en: "early",
  },
  hc_prg_late: {
    de: "Spaet",
    en: "late",
  },
  hc_prg_am: {
    de: "Vormittag",
    en: "AM",
  },
  hc_prg_pm: {
    de: "Nachmittag",
    en: "PM",
  },
  hc_prg_noon: {
    de: "Mittag",
    en: "noon",
  },
  hc_prg_single: {
    de: "Single",
    en: "single",
  },
  hc_prg_senior: {
    de: "Senior",
    en: "senior",
  },
  info_summer1: {
    de: "Umschalttemperatur zwischen Sommer / Winter",
    en: "Threshold to switch between Summer/Winter",
  },
  info_summer2: {
    de: "9:Sommer | 10..30:Schwelle (\u00b0C) | 31:Winter",
    en: "9:Summer | 10..30:Threshold (\u00b0C) | 31:Winter",
  },
  info_frost: {
    de: "Umschalttemperatur Frostschutz",
    en: "Threshold for Frostprotection",
  },
  info_designtemp: {
    de: "Auslegungstemperatur Heizkennlinie",
    en: "Design Temperature for heating curve",
  },
  info_switchoff: {
    de: "Umschaltschwelle f\u00fcr Absenkung Aussenhalt",
    en: "Threshold for reduction mode",
  },
  info_wwtemp: {
    de: "Solltemperatur f\u00fcr Warmwasser",
    en: "Setpoint for Hot Water",
  },
  info_unit_c: {
    de: "Einheit: \u00b0C",
    en: "Unit: \u00b0C",
  },
  info_ww_pump_circ1: {
    de: "Anzahl der Zyklen pro Stunde",
    en: "count of operation cycles per hour",
  },
  info_ww_pump_circ2: {
    de: "0:AUS | 1..6: Zyklen/Stunde | 7:EIN",
    en: "0:OFF | 1..6: cycles/hour | 7:ON",
  },
  oilmeter: {
    de: "\u00d6lz\u00e4hler",
    en: "Oil-Meter",
  },
  set: {
    de: "setzen",
    en: "set",
  },
  button_ntp: {
    de: "schreibe NTP-Datum/Zeit auf Logamatic",
    en: "write NTP-date/time to Logamatic",
  },
  button_dti: {
    de: "schreibe Datum/Zeit auf Logamatic",
    en: "write date/time to Logamatic",
  },
  alarm: {
    de: "Alarme",
    en: "Alarms",
  },
  alarm_info: {
    de: "Alarme + Info",
    en: "Alarms + Info",
  },
  alarminfo: {
    de: "letzte Alarm Meldungen",
    en: "latest Alarm Messages",
  },
  operation: {
    de: "Betrieb",
    en: "Operation",
  },
  lifetimes: {
    de: "Laufzeiten",
    en: "Runtimes",
  },
  limits: {
    de: "Grenzwerte",
    en: "Limits",
  },
  datetime: {
    de: "Datum und Uhrzeit",
    en: "Date and Time",
  },
  logamatic: {
    de: "Logamatic",
    en: "Logamatic",
  },
  act_date: {
    de: "aktuelles Datum (auf ESP)",
    en: "actual date (on ESP)",
  },
  act_time: {
    de: "aktuelle Uhrzeit (auf ESP)",
    en: "actual time (on ESP)",
  },
  ntp_auto_sync: {
    de: "schreibe ntp Zeit nach jedem PowerOn",
    en: "write ntp time on PowerOn",
  },
  set_date: {
    de: "Datum setzen",
    en: "set date",
  },
  set_time: {
    de: "Uhrzeit setzen",
    en: "set time",
  },
  gpio: {
    de: "GPIO-Zuweisung",
    en: "GPIO-Settings",
  },
  led_wifi: {
    de: "LED-WiFi",
    en: "LED-WiFi",
  },
  led_heartbeat: {
    de: "LED-Heartbeat",
    en: "LED-Heartbeat",
  },
  led_logmode: {
    de: "LED-Logmode",
    en: "LED-Logmode",
  },
  led_oilcounter: {
    de: "LED-\u00d6lz\u00e4hler",
    en: "LED-Oilcounter",
  },
  trig_oilcounter: {
    de: "Impuls-\u00d6lz\u00e4hler",
    en: "Trigger-Oilcounter",
  },
  km271_tx: {
    de: "KM271-TX",
    en: "KM271-TX",
  },
  km271_rx: {
    de: "KM271-RX",
    en: "KM271-RX",
  },
  oil_hardware: {
    de: "\u00d6lz\u00e4hler Hardware",
    en: "Oil Meter Hardware",
  },
  oil_virtual: {
    de: "\u00d6lz\u00e4hler virtuell",
    en: "Oil Meter virtual",
  },
  oil_par1_kg_h: {
    de: "Verbrauch Kg/h",
    en: "consumption Kg/h",
  },
  oil_par2_kg_l: {
    de: "\u00d6l-Dichte Kg/L",
    en: "oil density Kg/L",
  },
  pulse_per_liter: {
    de: "Impule pro Liter",
    en: "pulses per liter",
  },
  debounce_time: {
    de: "Entprellzeit (ms)",
    en: "debounce time (ms)",
  },
  virt_calc_offset: {
    de: "Berechnungskorrekturwert für virtuellen Verbrauch",
    en: "Calculation Correction Value for Virtual Consumption",
  },
  oil_unit_info_set: {
    de: "Die Eingabe erfolgt als Ganzzahl in 100stel Liter\n(12345 = 123,45 Liter)",
    en: "The input is a whole number in 100th of a liter\n(12345 = 123.45 liters)",
  },
  temp_out: {
    de: "Au\u00dfentemperatur",
    en: "Temperature outdoor",
  },
  temp_out_act: {
    de: "aktuell \u00b0C",
    en: "actually \u00b0C",
  },
  temp_out_dmp: {
    de: "ged\u00e4mpft \u00b0C",
    en: "damped \u00b0C",
  },
  mqtt_cfg_ret: {
    de: "Config Nachrichten als retain",
    en: "config messages as retain",
  },
  optional_sensor: {
    de: "optionale Sensoren",
    en: "optional sensors",
  },
  sens1: {
    de: "Sensor 1",
    en: "Sensor 1",
  },
  sens2: {
    de: "Sensor 2",
    en: "Sensor 2",
  },
  name: {
    de: "Name",
    en: "Name",
  },
  filter: {
    de: "Filter",
    en: "Filter",
  },
  pushover: {
    de: "Pushover",
    en: "Pushover",
  },
  api_token: {
    de: "API-Token",
    en: "API-Token",
  },
  user_key: {
    de: "User-Key",
    en: "User-Key",
  },
  test_msg: {
    de: "Testnachricht",
    en: "test message",
  },
  reduct_mode_off: {
    de: "Abschalt",
    en: "off",
  },
  reduct_mode_fixed: {
    de: "Reduziert",
    en: "fixed",
  },
  reduct_mode_room: {
    de: "Raumhalt",
    en: "room",
  },
  reduct_mode_outdoors: {
    de: "Aussenhalt",
    en: "outdoors",
  },
  frost_protection_threshold: {
    de: "Frost ab",
    en: "frost protection threshold",
  },
  summer_mode_threshold: {
    de: "Sommer ab",
    en: "summer mode threshold",
  },
  night_temp: {
    de: "Nachttemperatur",
    en: "night temp",
  },
  day_temp: {
    de: "Tagtemperatur",
    en: "day temp",
  },
  operation_mode: {
    de: "Betriebsart",
    en: "operation mode",
  },
  holiday_temp: {
    de: "Urlaubtemperatur",
    en: "holiday temp",
  },
  max_temp: {
    de: "Max Temperatur",
    en: "max temp",
  },
  interpretation: {
    de: "Auslegung",
    en: "interpretation",
  },
  switch_on_temperature: {
    de: "Aufschalttemperatur",
    en: "switch on temperature",
  },
  switch_off_threshold: {
    de: "Aussenhalt ab",
    en: "switch off threshold",
  },
  reduction_mode: {
    de: "Absenkungsart",
    en: "reduction mode",
  },
  heating_system: {
    de: "Heizsystem",
    en: "heating system",
  },
  temp_offset: {
    de: "Temperatur_Offset",
    en: "temp offset",
  },
  remotecontrol: {
    de: "Fernbedienung",
    en: "remote control",
  },
  holiday_days: {
    de: "Ferien_Tage",
    en: "holiday days",
  },
  time_offset: {
    de: "Uhrzeit_Offset",
    en: "time offset",
  },
  priority: {
    de: "Vorrang",
    en: "priority",
  },
  temp: {
    de: "Temperatur",
    en: "temp",
  },
  temp_c: {
    de: "°C",
    en: "°C",
  },
  processing: {
    de: "Aufbereitung",
    en: "processing",
  },
  circulation: {
    de: "Zirkulation",
    en: "circulation",
  },
  building_type: {
    de: "Gebaeudeart",
    en: "building type",
  },
  display: {
    de: "Anzeige",
    en: "display",
  },
  burner_type: {
    de: "Brennerart",
    en: "burner type",
  },
  max_boiler_temperature: {
    de: "Max_Kesseltemperatur",
    en: "max boiler temperature",
  },
  pump_logic_temp: {
    de: "Pumplogik",
    en: "pump logic",
  },
  exhaust_gas_temperature_threshold: {
    de: "Abgastemperaturschwelle",
    en: "exhaust gas temperature threshold",
  },
  burner_min_modulation: {
    de: "Brenner_Min_Modulation",
    en: "burner min modulation",
  },
  burner_modulation_runtime: {
    de: "Brenner_Mod_Laufzeit",
    en: "burner modulation runtime",
  },
  ov1_off_time_optimization: {
    de: "BW1 Ausschaltoptimierung",
    en: "OV1 off time optimization",
  },
  ov1_on_time_optimization: {
    de: "BW1 Einschaltoptimierung",
    en: "OV1 on time optimization",
  },
  ov1_automatic: {
    de: "BW1 Automatik",
    en: "OV1 automatic",
  },
  ov1_ww_priority: {
    de: "BW1 Warmwasservorrang",
    en: "OV1 WW priority",
  },
  ov1_screed_drying: {
    de: "BW1 Estrichtrocknung",
    en: "OV1 screed drying",
  },
  ov1_holiday: {
    de: "BW1 Ferien",
    en: "OV1 holiday",
  },
  ov1_frost_protection: {
    de: "BW1 Frostschutz",
    en: "OV1 frost protection",
  },
  ov2_summer: {
    de: "BW2 Sommer",
    en: "OV2 summer",
  },
  ov2_day: {
    de: "BW2 Tag",
    en: "OV2 day",
  },
  ov2_no_conn_to_remotectrl: {
    de: "BW2 Keine Komm mit FB",
    en: "OV2 no conn to remotectrl",
  },
  ov2_remotectrl_error: {
    de: "BW2 FB fehlerhaft",
    en: "OV2 remotectrl error",
  },
  ov2_failure_flow_sensor: {
    de: "BW2 Fehler Vorlauffuehler",
    en: "OV2 failure flow sensor",
  },
  ov2_flow_at_maximum: {
    de: "BW2 Maximaler Vorlauf",
    en: "OV2 flow at maximum",
  },
  ov2_external_signal_input: {
    de: "BW2 Externer Stoereingang",
    en: "OV2 external signal input",
  },
  flow_setpoint: {
    de: "Vorlaufsolltemperatur",
    en: "Flow setpoint",
  },
  flow_temp: {
    de: "Vorlaufisttemperatur",
    en: "Flow temp",
  },
  room_setpoint: {
    de: "Raumsolltemperatur",
    en: "Room setpoint",
  },
  room_temp: {
    de: "Raumisttemperatur",
    en: "Room temp",
  },
  off_time_opt_duration: {
    de: "Ausschaltoptimierung",
    en: "Off time opt duration",
  },
  mixer: {
    de: "Mischerstellung",
    en: "Mixer",
  },
  heat_curve_10C: {
    de: "Heizkennlinie 10 Grad",
    en: "Heat curve 10C",
  },
  heat_curve_0C: {
    de: "Heizkennlinie 0 Grad",
    en: "Heat curve 0C",
  },
  "heat_curve_-10C": {
    de: "Heizkennlinie -10 Grad",
    en: "Heat curve -10C",
  },
  ov1_auto: {
    de: "BW1 Automatik",
    en: "OV1 auto",
  },
  ov1_disinfection: {
    de: "BW1 Desinfektion",
    en: "OV1 disinfection",
  },
  ov1_reload: {
    de: "BW1 Nachladung",
    en: "OV1 reload",
  },
  ov1_err_disinfection: {
    de: "BW1 Fehler Desinfektion",
    en: "OV1 err disinfection",
  },
  ov1_err_sensor: {
    de: "BW1 Fehler Fuehler",
    en: "OV1 err sensor",
  },
  ov1_stays_cold: {
    de: "BW1 Fehler bleibt kalt",
    en: "OV1 stays cold",
  },
  ov1_err_anode: {
    de: "BW1 Fehler Anode",
    en: "OV1 err anode",
  },
  ov2_load: {
    de: "BW2 Laden",
    en: "OV2 load",
  },
  ov2_manual: {
    de: "BW2 Manuell",
    en: "OV2 manual",
  },
  ov2_reload: {
    de: "BW2 Nachladen",
    en: "OV2 reload",
  },
  ov2_off_time_opt_duration: {
    de: "BW2 Ausschaltoptimierung",
    en: "OV2 off time opt duration",
  },
  ov2_on_time_opt_duration: {
    de: "BW2 Einschaltoptimierung",
    en: "OV2 on time opt duration",
  },
  ov2_hot: {
    de: "BW2 Warm",
    en: "OV2 hot",
  },
  ov2_priority: {
    de: "BW2 Vorrang",
    en: "OV2 priority",
  },
  on_time_opt_duration: {
    de: "Einschaltoptimierung",
    en: "on time opt duration",
  },
  pump_type_charge: {
    de: "Pumpentyp Ladepumpe",
    en: "pump type charge",
  },
  pump_type_circulation: {
    de: "Pumpentyp Zirkulationspumpe",
    en: "pump type circulation",
  },
  pump_type_groundwater_solar: {
    de: "Pumpentyp Absenkung Solar",
    en: "pump type groundwater solar",
  },
  boiler_setpoint: {
    de: "Kessel Vorlaufsolltemperatur",
    en: "Boiler setpoint",
  },
  boiler_temp: {
    de: "Kessel Vorlaufisttemperatur",
    en: "Boiler temp",
  },
  boiler_switch_on_temp: {
    de: "Brenner Einschalttemperatur",
    en: "Boiler switch on temp",
  },
  boiler_switch_off_temp: {
    de: "Brenner Ausschalttemperatur",
    en: "Boiler switch off temp",
  },
  boiler_failure_burner: {
    de: "Kessel Fehler Brennerstörung",
    en: "Boiler failure burner",
  },
  boiler_failure_boiler_sensor: {
    de: "Kessel Fehler Kesselfühler",
    en: "Boiler failure boiler sensor",
  },
  boiler_failure_aux_sensor: {
    de: "Kessel Fehler Zusatzfühler",
    en: "Boiler failure aux sensor",
  },
  boiler_failure_boiler_stays_cold: {
    de: "Kessel Fehler Kessel bleibt kalt",
    en: "Boiler failure boiler stays cold",
  },
  boiler_failure_exhaust_gas_sensor: {
    de: "Kessel Fehler Abgasfühler",
    en: "Boiler failure exhaust gas sensor",
  },
  boiler_failure_exhaust_gas_over_limit: {
    de: "Kessel Fehler Abgas max Grenzwert",
    en: "Boiler failure exhaust gas over limit",
  },
  boiler_failure_safety_chain: {
    de: "Kessel Fehler Sicherungskette",
    en: "Boiler failure safety chain",
  },
  boiler_failure_external: {
    de: "Kessel Fehler Externe Störung",
    en: "Boiler failure external",
  },
  boiler_state_exhaust_gas_test: {
    de: "Kessel Betrieb Abgastest",
    en: "Boiler state exhaust gas test",
  },
  boiler_state_stage1: {
    de: "Kessel Betrieb Betrieb Stufe1",
    en: "Boiler state stage1",
  },
  boiler_state_boiler_protection: {
    de: "Kessel Betrieb Kesselschutz",
    en: "Boiler state boiler protection",
  },
  boiler_state_active: {
    de: "Kessel Betrieb Unter Betrieb",
    en: "Boiler state active",
  },
  boiler_state_performance_free: {
    de: "Kessel Betrieb Leistung frei",
    en: "Boiler state performance free",
  },
  boiler_state_performance_high: {
    de: "Kessel Betrieb Leistung hoch",
    en: "Boiler state performance high",
  },
  boiler_state_stage2: {
    de: "Kessel Betrieb BetriebStufe2",
    en: "Boiler state stage2",
  },
  burner_control: {
    de: "Brenner Ansteuerung",
    en: "Burner control",
  },
  exhaust_gas_temp: {
    de: "Abgastemperatur",
    en: "Exhaust gas temp",
  },
  runtime_years: {
    de: "Laufzeit Jahre",
    en: "runtime years",
  },
  runtime_days: {
    de: "Laufzeit Tage",
    en: "runtime days",
  },
  runtime_hours: {
    de: "Laufzeit Stunden",
    en: "runtime hours",
  },
  runtime_minutes: {
    de: "Laufzeit Minuten",
    en: "runtime minutes",
  },
  outside_temp: {
    de: "Außentemperatur",
    en: "Outside temp",
  },
  outside_temp_damped: {
    de: "Außentemperatur gedämpft",
    en: "Outside temp damped",
  },
  send: {
    de: "senden",
    en: "send",
  },
  log_mode_1: {
    de: "Modus: Alarm",
    en: "Mode: Alarm",
  },
  log_mode_2: {
    de: "Modus: Alarm + Info",
    en: "Modus: Alarm + Info",
  },
  log_mode_3: {
    de: "Modus: Logamatic Werte",
    en: "Mode: Logamatic values",
  },
  log_mode_4: {
    de: "Modus: unbekannte Datagramme",
    en: "Mode: unknown datagramms",
  },
  log_mode_5: {
    de: "Modus: debug Datagramme",
    en: "Mode: debug datagramms",
  },
  log_type_1: {
    de: "Logbuch: SYSTEM",
    en: "Logbook: SYSTEM",
  },
  log_type_2: {
    de: "Logbuch: KM271",
    en: "Logbook: KM271",
  },
  description: {
    de: "Beschreibung",
    en: "description",
  },
  simulation: {
    de: "Simulation",
    en: "Simulation",
  },
  generate_simdata: {
    de: "generiere Simulationsdaten",
    en: "generate simulation data",
  },
  select_template: {
    de: "wähle Vorlage...",
    en: "select template...",
  },
  km271_refresh: {
    de: "empfange Daten...",
    en: "receiving data...",
  },
  timer: {
    de: "Timer",
    en: "timer",
  },
  message: {
    de: "Meldung",
    en: "Message",
  },
  cyclic_send: {
    de: "zyklisches Senden [min]",
    en: "cyclic send [min]",
  },
  solar: {
    de: "Solar",
    en: "Solar",
  },
  on: {
    de: "EIN",
    en: "ON",
  },
  solar_max: {
    de: "Max Solar",
    en: "min Solar",
  },
  solar_min: {
    de: "Min Solar",
    en: "min Solar",
  },
  load: {
    de: "Ladung",
    en: "load",
  },
  collector: {
    de: "Kollektor",
    en: "Collector",
  },
  km271_info: {
    de: "KM271 Information",
    en: "KM271 Information",
  },
  sent: {
    de: "gesendet",
    en: "sent",
  },
  received: {
    de: "empfangen",
    en: "received",
  },
  act_prg: {
    de: "aktives Programm",
    en: "active program",
  },
  man_prg_timer: {
    de: "Timer des manuellen Programms",
    en: "timer of manual program",
  },
};
