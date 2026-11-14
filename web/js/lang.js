// --------------------------------------
// localization texts
// --------------------------------------
const user_translations = {
  info_git_installed: {"de": "Installiert:", "en": "Installed:"},
  info_git_release: {"de": "Letztes Release:", "en": "Latest release:"},
  info_git_idle: {"de": "Prüfe, welche Version auf GitHub verfügbar ist.", "en": "Check which version is available on GitHub."},
  info_git_checking: {"de": "Version wird geprüft …", "en": "Checking version …"},
  info_git_ready: {"de": "Eine andere Version ist verfügbar.", "en": "A different version is available."},
  info_git_newer: {"de": "Eine neuere Version ist verfügbar.", "en": "A newer version is available."},
  info_git_older: {"de": "Das Release ist älter als die installierte Version.", "en": "This release is older than the installed version."},
  info_git_current: {"de": "Bereits aktuell.", "en": "Already up to date."},
  info_git_error: {"de": "Prüfung fehlgeschlagen. Bitte erneut versuchen.", "en": "Check failed. Please try again."},
  info_git_noasset: {"de": "Keine passende Firmware für dieses Gerät im Release gefunden.", "en": "No matching firmware for this device was found in the release."},
  info_git_install: {"de": "Version installieren", "en": "Install version"},
  info_git_downgrade: {"de": "Ältere Version installieren", "en": "Install older version"},
  info_git_confirm: {"de": "Diese Version installieren? Der ESP wird anschließend neu gestartet.", "en": "Install this version? The ESP will restart afterwards."},
  info_git_confirm_older: {"de": "Diese Version ist älter als die installierte Firmware. Trotzdem installieren?", "en": "This version is older than the installed firmware. Install anyway?"},
  info_git_updating: {"de": "Installation läuft", "en": "Installing"},
  info_git_done: {"de": "Installation erfolgreich. Neustart bestätigen.", "en": "Installation successful. Confirm restart."},
  info_git_failed: {"de": "Installation fehlgeschlagen. Bitte vor einem weiteren Versuch erneut prüfen.", "en": "Installation failed. Check again before retrying."},
  info_git_offline: {"de": "Verbindung unterbrochen. Bitte nach dem Wiederverbinden erneut prüfen.", "en": "Connection lost. Check again after reconnecting."},

  info_update_local: { de: "Lokale Datei", en: "Local file" },
  info_update_local_hint: { de: "Eine Firmware-Datei von deinem Computer auf den ESP übertragen.", en: "Upload a firmware file from your computer to the ESP." },
  info_update_github: { de: "Von GitHub", en: "From GitHub" },
  info_update_github_hint: { de: "Eine veröffentlichte Version direkt aus dem Projekt herunterladen.", en: "Download a published version directly from the project." },
  info_install_file: { de: "Datei installieren", en: "Install file" },
  ui_reload: { de: "Seite neu laden", en: "Reload page" },
  info_firmware_used: { de: "Firmware-Speicher belegt", en: "Firmware storage used" },
  info_ram_free: { de: "RAM frei", en: "Free RAM" },
  info_largest_free_block: { de: "Größter freier Speicherblock", en: "Largest free memory block" },
  info_minimum_free_ram: { de: "Niedrigster freier RAM seit Neustart", en: "Lowest free RAM since restart" },
  info_memory_used: { de: "belegt", en: "used" },

  info_log_recording: { de: "Aufzeichnung", en: "Recording" },
  info_firmware_update: { de: "Firmware-Update", en: "Firmware update" },
  info_configuration: { de: "Konfiguration", en: "Configuration" },
  info_check_github: { de: "GitHub-Version prüfen", en: "Check GitHub version" },
  info_import: { de: "Importieren", en: "Import" },
  info_export: { de: "Exportieren", en: "Export" },
  info_show: { de: "Anzeigen", en: "Show" },

  info_ethernet: { de: "Ethernet", en: "Ethernet" },
  info_current_values: { de: "Aktuelle Werte", en: "Current values" },
  info_operating_states: { de: "Betriebszustände", en: "Operating states" },
  info_monitoring: { de: "Überwachung", en: "Monitoring" },
  info_memory: { de: "Speicher", en: "Memory" },
  info_circuit_subtitle: { de: "Konfiguration und Betriebszustände", en: "Configuration and operating states" },
  info_ww_subtitle: { de: "Speicher und Warmwasserbereitung", en: "Cylinder and hot water preparation" },
  info_burner_subtitle: { de: "Kessel, Betrieb und Überwachung", en: "Boiler, operation and monitoring" },
  info_solar_subtitle: { de: "Temperaturen und Anlagenzustand", en: "Temperatures and system status" },
  info_general_subtitle: { de: "Übergreifende Einstellungen der Logamatic", en: "General Logamatic settings" },
  info_alarm_subtitle: { de: "Meldungen der Logamatic", en: "Logamatic messages" },
  info_tools_subtitle: { de: "Firmware und Konfigurationsverwaltung", en: "Firmware and configuration management" },
  info_log_subtitle: { de: "Ereignisse und Fehlersuche", en: "Events and troubleshooting" },
  info_alarm_memory: { de: "Meldespeicher", en: "Message memory" },
  info_alarm_hint: { de: "Die Meldungen können auch bereits quittierte Störungen enthalten. Ein Eintrag bedeutet nicht automatisch, dass die Störung aktuell anliegt.", en: "Messages may include previously acknowledged faults. An entry does not necessarily mean the fault is currently active." },
  info_mqtt_error_hint: { de: "Der letzte Fehler ist gespeichert und beschreibt nicht zwingend den aktuellen Verbindungszustand.", en: "The last error is stored and does not necessarily describe the current connection state." },
  info_years: { de: "Jahre", en: "years" },
  info_days: { de: "Tage", en: "days" },
  info_hours: { de: "Std.", en: "hrs" },
  info_minutes: { de: "Min.", en: "min" },
  info_firmware_file: { de: "Firmware-Datei", en: "Firmware file" },
  info_config_file: { de: "Konfigurationsdatei", en: "Configuration file" },
  info_no_file: { de: "Keine Datei ausgewählt", en: "No file selected" },
  info_log_source: { de: "Logbuch", en: "Log" },
  info_log_level: { de: "Level", en: "Level" },
  info_log_order: { de: "Reihenfolge", en: "Order" },
  info_solar_status: { de: "Status 9147", en: "Status 9147" },

  control_help: { de: "Hilfe", en: "Help" },
  control_summer_threshold: { de: "Sommergrenze", en: "Summer threshold" },
  control_frost_threshold: { de: "Frostschutz", en: "Frost protection" },
  control_design_temp: { de: "Auslegungstemperatur", en: "Design temperature" },
  control_help_hc_mode: { de: "Nacht und Tag wählen dauerhaft den jeweiligen Betrieb. Auto wechselt anhand des an der Logamatic gewählten Heizprogramms zwischen Tag- und Nachtbetrieb.", en: "Night and Day select the respective operating mode continuously. Auto switches between day and night operation according to the heating program selected on the Logamatic." },
  control_help_program: { de: "Wählt das Zeitprogramm der Logamatic für den Automatikbetrieb. „Eigen“ verwendet das individuelle Programm der Regelung; die übrigen Einträge sind deren vordefinierte Programme. Die Schaltzeiten werden hier nicht bearbeitet.", en: "Selects the Logamatic schedule for automatic operation. Custom uses the controller’s individual program; the other entries are its predefined programs. Switching times are not edited here." },
  control_help_holiday_days: { de: "Dauer des Urlaubsbetriebs: 0–99 Tage. Die Logamatic verringert den Wert täglich um einen Tag. 0 bedeutet keinen laufenden Urlaubszeitraum. Die zugehörige Temperatur wird separat eingestellt.", en: "Holiday operation duration: 0–99 days. The Logamatic decreases the value by one each day. 0 means no active holiday period. Its temperature is set separately." },
  control_help_reduction: { de: "Bestimmt die Betriebsweise während der Absenkzeiten: Abschalt, Reduziert, Raumhalt oder Aussenhalt. Raumhalt berücksichtigt die Raumtemperatur, Aussenhalt die Außentemperatur. Die tatsächliche Wirkung hängt von der Ausstattung und Konfiguration der Logamatic ab.", en: "Selects operation during setback periods: Off, Reduced, Room hold or Outdoor hold. Room hold uses room temperature; Outdoor hold uses outdoor temperature. Actual behavior depends on the Logamatic equipment and configuration." },
  control_help_day: { de: "Raum-Solltemperatur für den Tagbetrieb: 10–30 °C in Schritten von 0,5 °C. Es handelt sich nicht um die Vorlauftemperatur. Der Wert beeinflusst die Heizungsregelung auch über die Heizkennlinie.", en: "Room temperature setpoint for day operation: 10–30 °C in 0.5 °C steps. This is not the flow temperature. The value also affects heating control through the heating curve." },
  control_help_night: { de: "Raum-Solltemperatur für den Nacht- bzw. reduzierten Betrieb: 10–30 °C in Schritten von 0,5 °C. Ob und wie geheizt wird, hängt zusätzlich von der Absenkungsart ab.", en: "Room temperature setpoint for night or reduced operation: 10–30 °C in 0.5 °C steps. Heating behavior also depends on the selected reduction mode." },
  control_help_holiday_temp: { de: "Raum-Solltemperatur während des eingestellten Urlaubszeitraums: 10–30 °C in Schritten von 0,5 °C. Die Dauer wird über Urlaubstage festgelegt.", en: "Room temperature setpoint during the selected holiday period: 10–30 °C in 0.5 °C steps. Set its duration using Holiday days." },
  control_help_summer: { de: "Außentemperaturgrenze für die Sommer-/Winterumschaltung. 10–30 °C: temperaturabhängige Umschaltung. Die Randstellungen sind Sonderwerte: 9 = dauerhaft Sommer, 31 = dauerhaft Winter.", en: "Outdoor temperature threshold for summer/winter switching. 10–30 °C: temperature-dependent switching. The end positions are special values: 9 = permanent summer, 31 = permanent winter." },
  control_help_frost: { de: "Temperaturschwelle für den Frostschutz des Heizkreises: −20 bis +10 °C in Schritten von 1 °C.", en: "Temperature threshold for heating-circuit frost protection: −20 to +10 °C in 1 °C steps." },
  control_help_design: { de: "Auslegungstemperatur der Heizkennlinie: 30–90 °C in Schritten von 1 °C. Sie beschreibt die Auslegung des Vorlaufs und ist keine Raum-Solltemperatur.", en: "Heating curve design temperature: 30–90 °C in 1 °C steps. It describes the flow temperature design point, not a room temperature setpoint." },
  control_help_switch_on: { de: "Aufschalttemperatur des Heizkreises: 0–10 °C in Schritten von 1 °C. Dieser Parameter wird direkt an die Logamatic übertragen; seine genaue Wirkung richtet sich nach der verwendeten Regelung und deren Konfiguration.", en: "Heating-circuit switch-on temperature: 0–10 °C in 1 °C steps. This parameter is sent directly to the Logamatic; its exact effect depends on the controller and its configuration." },
  control_help_switch_off: { de: "Außentemperaturschwelle für die Absenkungsart Aussenhalt: −20 bis +10 °C in Schritten von 1 °C. Der Wert gehört zur außentemperaturabhängigen Absenkung.", en: "Outdoor temperature threshold for Outdoor hold reduction: −20 to +10 °C in 1 °C steps. This value belongs to outdoor-temperature-dependent setback operation." },
  control_help_ww_mode: { de: "Wählt Nacht-, Tag- oder Automatikbetrieb der Warmwasserbereitung. Auto nutzt die zeitabhängige Freigabe der Logamatic. Die genaue Freigabe ist von deren Konfiguration abhängig.", en: "Selects night, day or automatic operation for hot water. Auto uses the Logamatic’s timed enable. The exact enable behavior depends on its configuration." },
  control_help_ww_temp: { de: "Solltemperatur für das Warmwasser: 30–60 °C in Schritten von 1 °C. Die aktuelle Speichertemperatur wird separat im Dashboard angezeigt.", en: "Hot-water setpoint: 30–60 °C in 1 °C steps. The current cylinder temperature is displayed separately on the dashboard." },
  control_help_circulation: { de: "Ansteuerung der Warmwasser-Zirkulationspumpe. 0 = AUS; 1–6 = Anzahl der Zyklen pro Stunde; 7 = EIN. Die Zahlen 1–6 sind keine Temperaturwerte.", en: "Hot-water circulation pump control. 0 = OFF; 1–6 = cycles per hour; 7 = ON. Values 1–6 are not temperatures." },
  control_help_oil_feedback: { de: "„Setzen“ überträgt den eingegebenen Zählerstand. Der Istwert daneben zeigt den vom ESP bestätigten Stand in Litern; er wird erst mit dessen Rückmeldung aktualisiert.", en: "Set sends the entered meter reading. The actual value beside it shows the reading confirmed by the ESP in liters and updates only after its response." },

  control_subtitle: { de: "Betriebsarten und Temperaturen", en: "Operating modes and temperatures" },
  control_oilmeter_set: { de: "Zählerstand setzen", en: "Set meter reading" },
  summer: { de: "Sommer", en: "Summer" },
  winter: { de: "Winter", en: "Winter" },
  ha_mqtt_hint: { de: "Benötigt eine aktivierte und konfigurierte MQTT-Verbindung.", en: "Requires an enabled and configured MQTT connection." },
  ha_mqtt_disabled: { de: "MQTT ist derzeit deaktiviert.", en: "MQTT is currently disabled." },
  ha_mqtt_open: { de: "MQTT-Einstellungen öffnen", en: "Open MQTT settings" },
  ui_connected: { de: "Verbunden", en: "Connected" },
  ui_disconnected: { de: "Verbindung unterbrochen – verbinde erneut …", en: "Disconnected – reconnecting …" },
  dashboard_subtitle: { de: "Heizung im Überblick", en: "Heating at a glance" },
  system_subtitle: { de: "Verbindungen und Gerät", en: "Connections and device" },
  settings_subtitle: { de: "Konfiguration", en: "Configuration" },
  chart_target: { de: "Soll", en: "Target" },
  chart_actual: { de: "Ist", en: "Actual" },
  chart_current: { de: "Aktuell", en: "Current" },
  chart_damped: { de: "Gedämpft", en: "Damped" },
  chart_wait: { de: "Warte auf gültige Daten …", en: "Waiting for valid data …" },
  chart_off: { de: "AUS", en: "OFF" },
  chart_gap: { de: "Keine Messdaten in diesem Zeitraum", en: "No measurements in this interval" },
  chart_burner: { de: "Brenner", en: "Burner" },
  chart_priority: { de: "Warmwasservorrang", en: "Hot water priority" },
  weekly_program_hint: {
    de: "Wochenplan des manuellen Programms. Das aktive Programm wird oben angezeigt.",
    en: "Weekly schedule of the manual program. The active program is shown above.",
  },
  weekly_program_loading: { de: "Schaltzeiten werden geladen …", en: "Loading switch points …" },
  weekly_program_empty: { de: "Keine Schaltzeiten für das manuelle Programm hinterlegt.", en: "No switch points stored for the manual program." },
  weekly_program_invalid: { de: "Schaltzeiten nicht eindeutig darstellbar. Bitte die Schaltpunkte prüfen.", en: "The schedule cannot be displayed unambiguously. Please check the switch points." },
  weekly_program_details: { de: "Schaltpunkte anzeigen", en: "Show switch points" },
  weekly_program_on: { de: "An", en: "On" },
  weekly_program_off: { de: "Aus", en: "Off" },
  service_access: { de: "Service-Zugänge", en: "Service access" },
  service_password: { de: "Service-Passwort", en: "Service password" },
  service_telnet: { de: "Telnet aktivieren", en: "Enable Telnet" },
  service_hint: {
    de: "Zentrales Passwort für Service Zugänge wie z.B. Telnet und Remote-OTA (ArduinoOTA). Ohne Passwort bleiben beide gesperrt. Im Setup-Modus gilt admin / km271-2026!.",
    en: "Central password for service access such as Telnet and Remote-OTA (ArduinoOTA). Without a password, both remain locked. In setup mode, admin / km271-2026! applies.",
  },
  wifi_ssid_search: { de: "Suche WLAN", en: "Find Wi-Fi" },
  wifi_ssid_title: { de: "WLAN auswählen", en: "Select Wi-Fi network" },
  wifi_ssid_select: {
    de: "Verfügbare WLAN-Netze",
    en: "Available Wi-Fi networks",
  },
  wifi_ssid_hint: {
    de: "Versteckte WLAN-Namen können manuell eingegeben werden. Bei einem anderen WLAN-Namen wird die feste BSSID zurückgesetzt.",
    en: "Hidden Wi-Fi names can be entered manually. Selecting a different Wi-Fi name clears the fixed BSSID.",
  },
  wifi_ssid_choose: {
    de: "Bitte WLAN auswählen",
    en: "Select a Wi-Fi network",
  },
  wifi_ssid_found: {
    de: "WLAN auswählen und mit Übernehmen bestätigen.",
    en: "Select a network and confirm with Apply.",
  },
  wifi_ssid_empty: {
    de: "Keine WLAN-Netze gefunden.",
    en: "No Wi-Fi networks found.",
  },
  wifi_ap_bssid: {
    de: "Access Point (BSSID, optional)",
    en: "Access point (BSSID, optional)",
  },
  wifi_ap_search: { de: "Suche AP", en: "Find AP" },
  wifi_ap_hint: {
    de: "Leer = automatische Auswahl. Mit fester BSSID erfolgt kein Wechsel zu anderen Access Points. Gilt ab der nächsten WLAN-Verbindung oder nach einem Neustart.",
    en: "Empty = automatic selection. A fixed BSSID prevents switching to other access points. Applies on the next Wi-Fi connection or after a restart.",
  },
  wifi_ap_title: { de: "Access Point auswählen", en: "Select access point" },
  wifi_ap_select: {
    de: "Verfügbare Access Points",
    en: "Available access points",
  },
  wifi_ap_retry: { de: "Erneut suchen", en: "Scan again" },
  wifi_ap_apply: { de: "Übernehmen", en: "Apply" },
  wifi_ap_auto: { de: "Automatisch auswählen", en: "Select automatically" },
  wifi_ap_current: { de: "aktuell verbunden", en: "currently connected" },
  wifi_ap_channel: { de: "Kanal", en: "Channel" },
  wifi_ap_not_found: { de: "nicht gefunden", en: "not found" },
  wifi_ap_scanning: { de: "Suche läuft…", en: "Scanning…" },
  wifi_ap_found: {
    de: "Access Point auswählen und mit Übernehmen bestätigen.",
    en: "Select an access point and confirm with Apply.",
  },
  wifi_ap_empty: {
    de: "Keine Access Points für diesen WLAN-Namen gefunden.",
    en: "No access points found for this Wi-Fi name.",
  },
  wifi_ap_invalid_ssid: {
    de: "Bitte zuerst einen WLAN-Namen mit 1 bis 32 Byte eingeben.",
    en: "Enter a Wi-Fi name of 1 to 32 bytes first.",
  },
  wifi_ap_failed: {
    de: "Die Suche ist fehlgeschlagen. Bitte erneut versuchen.",
    en: "The scan failed. Please try again.",
  },
  wifi_ap_busy: {
    de: "Das Gerät führt bereits eine Suche oder ein Update aus. Bitte später erneut versuchen.",
    en: "The device is already scanning or updating. Please try again later.",
  },
  wifi_ap_ssid_changed: {
    de: "Der WLAN-Name wurde geändert. Bitte den Dialog schließen und erneut suchen.",
    en: "The Wi-Fi name has changed. Close this dialog and scan again.",
  },
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
  act_value: {
    de: "Istwert",
    en: "Actual Value",
  },
  set_temp: {
    de: "Solltemperatur",
    en: "Set Temperature",
  },
  act_temp: {
    de: "Isttemperatur",
    en: "Actual Temperature",
  },
  opmode: {
    de: "Betriebsart",
    en: "Operation Mode",
  },
  burner: {
    de: "Brenner",
    en: "Burner",
  },
  pump: {
    de: "Umw\u00e4lzpumpe",
    en: "Flow Pump",
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
  info_unit_c: {
    de: "\u00b0C",
    en: "\u00b0C",
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
  mqtt_cfg_ret: {
    de: "Config Nachrichten als retain",
    en: "config messages as retain",
  },
  mqtt_connection_test: {
    de: "Verbindung testen",
    en: "Test connection",
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
    de: "Urlaubstage",
    en: "Holiday days",
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
    de: "Ausschaltoptimierung",
    en: "off time optimization",
  },
  ov1_on_time_optimization: {
    de: "Einschaltoptimierung",
    en: "on time optimization",
  },
  ov1_automatic: {
    de: "Automatik",
    en: "automatic",
  },
  ov1_ww_priority: {
    de: "Warmwasservorrang",
    en: "WW priority",
  },
  ov1_screed_drying: {
    de: "Estrichtrocknung",
    en: "screed drying",
  },
  ov1_holiday: {
    de: "Ferien",
    en: "holiday",
  },
  ov1_frost_protection: {
    de: "Frostschutz",
    en: "frost protection",
  },
  ov2_summer: {
    de: "Sommer",
    en: "summer",
  },
  ov2_day: {
    de: "Tag",
    en: "day",
  },
  ov2_no_conn_to_remotectrl: { de: "Keine Verbindung zur Fernbedienung", en: "No connection to remote control" },
  ov2_remotectrl_error: { de: "Fernbedienung fehlerhaft", en: "Remote control fault" },
  ov2_failure_flow_sensor: { de: "Vorlauffühler fehlerhaft", en: "Flow sensor fault" },
  ov2_flow_at_maximum: {
    de: "Maximaler Vorlauf",
    en: "flow at maximum",
  },
  ov2_external_signal_input: { de: "Externer Störeingang", en: "External fault input" },
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
    de: "Automatik",
    en: "auto",
  },
  ov1_disinfection: {
    de: "Desinfektion",
    en: "disinfection",
  },
  ov1_reload: {
    de: "Nachladung",
    en: "reload",
  },
  ov1_err_disinfection: { de: "Desinfektion fehlerhaft", en: "Disinfection fault" },
  ov1_err_sensor: { de: "Temperaturfühler fehlerhaft", en: "Temperature sensor fault" },
  ov1_stays_cold: { de: "Speicher bleibt kalt", en: "Cylinder stays cold" },
  ov1_err_anode: { de: "Anodenfehler", en: "Anode fault" },
  ov2_load: {
    de: "Laden",
    en: "load",
  },
  ov2_manual: {
    de: "Manuell",
    en: "manual",
  },
  ov2_reload: {
    de: "Nachladen",
    en: "reload",
  },
  ov2_off_time_opt_duration: {
    de: "Ausschaltoptimierung",
    en: "off time opt duration",
  },
  ov2_on_time_opt_duration: {
    de: "Einschaltoptimierung",
    en: "on time opt duration",
  },
  ov2_hot: {
    de: "Warm",
    en: "hot",
  },
  ov2_priority: {
    de: "Vorrang",
    en: "priority",
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
    de: "Aktives Programm",
    en: "Active program",
  },
};
