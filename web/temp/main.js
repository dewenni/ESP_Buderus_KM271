// <<<< functions >>>>>>

// Funktion zum Umschalten der Sichtbarkeit von Elementen
function toggleElementVisibility(className, isVisible) {
  const elements = document.querySelectorAll(`.${className}`);
  elements.forEach((element) => {
    element.style.display = isVisible ? "" : "none";
  });
}

// Funktion zur Initialisierung der Sichtbarkeit basierend auf dem Zustand der Schalter
function initializeVisibilityBasedOnSwitches() {
  document
    .querySelectorAll('input[type="checkbox"][role="switch"]')
    .forEach(function (switchElement) {
      // Zustand des Schalters auswerten und Sichtbarkeit anpassen
      toggleElementVisibility(
        switchElement.getAttribute("hideOpt"),
        switchElement.checked
      );
    });
}

// validation for date and time inputs
const dateInput = document.getElementById("p12_dti_date");
const timeInput = document.getElementById("p12_dti_time");
const submitButton = document.getElementById("p12_dti_btn");
function validateInputs() {
  const isDateValid = dateInput.value !== "";
  const isTimeValid = timeInput.value !== "";
  dateInput.setAttribute("aria-invalid", !isDateValid);
  timeInput.setAttribute("aria-invalid", !isTimeValid);
  submitButton.disabled = !(isDateValid && isTimeValid);
}
dateInput.addEventListener("input", validateInputs);
timeInput.addEventListener("input", validateInputs);

function localizePage(lang = "en") {
  document.querySelectorAll("[data-i18n]").forEach((elem) => {
    const i18nValue = elem.getAttribute("data-i18n");
    const [translationPart, addon] = i18nValue.split("++", 2);
    const matches = translationPart.split(/(\$.+?\$)/).filter(Boolean);
    let text = "";

    for (const match of matches) {
      if (match.startsWith("$") && match.endsWith("$")) {
        text += match.slice(1, -1);
      } else {
        // check if translation key is valid
        if (translations.hasOwnProperty(match)) {
          text += translations[match][lang] || match;
        } else {
          console.error(`translation key "${match}" not found`);
          continue;
        }
      }
    }
    if (addon) {
      text += addon;
    }
    elem.innerText = text;
  });
}

var pingTimeout;
function resetPingTimeout() {
  // Zuvor gesetzten Timeout löschen
  clearTimeout(pingTimeout);

  // Setzt einen neuen Timeout
  pingTimeout = setTimeout(function () {
    console.log("Ping Timeout - Keine Ping-Nachricht empfangen.");
    // Zeige einen "Seite neu laden" Button an
    showReloadBar();
  }, 5000); // 5 Sekunden Wartezeit für den Ping
}

function showReloadBar() {
  document.getElementById("connectionLostBar").style.display = "flex";
}

// send data from web elements to ESP server
function sendData(elementId, value) {
  fetch(`sendData?elementId=${elementId}&value=${encodeURIComponent(value)}`, {
    method: "GET",
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error("Network response was not ok");
      }
      // Behandlung der erfolgreichen Antwort
    })
    .catch((error) => console.error("Fetch error:", error));
}

// <<<< Fetch API (Clinet -> ESP) >>>>>>
document.addEventListener("DOMContentLoaded", function () {
  // call functions on refresh
  resetPingTimeout();
  initializeVisibilityBasedOnSwitches();
  localizePage("de");

  // language selection
  document
    .getElementById("p12_language")
    .addEventListener("change", function () {
      var languageValue = this.value;
      if (languageValue === "1") {
        localizePage("en");
      } else if (languageValue === "0") {
        localizePage("de");
      }
    });

  // Event-Listener für alle Eingabefelder, die bei "blur" sendData aufrufen
  document
    .querySelectorAll(
      'input[type="text"], input[type="password"], input[type="date"], input[type="time"]'
    )
    .forEach(function (input) {
      input.addEventListener("blur", function () {
        sendData(input.id, input.value);
      });
      input.addEventListener("keypress", function (e) {
        if (e.key === "Enter") {
          input.blur(); // Löst "blur" Event aus und sendet Daten
        }
      });
    });

  // Event-Listener für Range Inputs hinzufügen
  document.querySelectorAll('input[type="range"]').forEach(function (slider) {
    slider.addEventListener("change", function () {
      sendData(slider.id, slider.value);
    });
  });

  // Event-Listener für Buttons
  document.querySelectorAll("button").forEach(function (button) {
    button.addEventListener("click", function () {
      sendData(button.id, true);
    });
  });

  // Event-Listener für Schalter
  document
    .querySelectorAll('input[type="checkbox"][role="switch"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
        toggleElementVisibility(
          switchElement.getAttribute("hideOpt"),
          switchElement.checked
        );
      });
    });

  // Event-Listener für Radio
  document
    .querySelectorAll('input[type="radio"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
      });
    });

  // Event-Listener für Select Elemente hinzufügen
  document.querySelectorAll("select").forEach(function (selectElement) {
    selectElement.addEventListener("change", function () {
      sendData(selectElement.id, selectElement.value);
    });
  });

  document.querySelectorAll(".rangeSlider").forEach((slider) => {
    const valueId = slider.getAttribute("data-value-id");
    const valueDisplay = document.getElementById(valueId);
    slider.oninput = () => {
      valueDisplay.textContent = slider.value;
    };
    // Initialwert setzen
    valueDisplay.textContent = slider.value;
  });
});

// <<<< Server-Side-Events (Client <- ESP) >>>>>>
var evtSource = new EventSource("/events");

// Event Listener für den Reload-Button
document
  .getElementById("p99_reloadButton")
  .addEventListener("click", function () {
    window.location.reload();
  });

// Listener für Ping-Nachrichten
evtSource.addEventListener(
  "ping",
  function (e) {
    console.log("Ping-Nachricht empfangen");
    resetPingTimeout(); // Setzt den Timeout zurück, wenn ein Ping empfangen wird
  },
  false
);

// Aktualisierung für Texte
evtSource.addEventListener(
  "updateText",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      if (data.isInput) {
        element.value = data.text; // Aktualisiert den `value` für Eingabeelemente
      } else {
        element.innerHTML = data.text; // Aktualisiert den `innerHTML` für andere Elemente
      }
    }
  },
  false
);

// Aktualisierung für Switches
evtSource.addEventListener(
  "updateState",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID); // Direkter Zugriff über getElementById
    if (element && (element.type === "checkbox" || element.type === "radio")) {
      element.checked = data.state;
      toggleElementVisibility(element.getAttribute("hideOpt"), element.checked);
    }
  },
  false
);

// Aktualisierung für element.value
evtSource.addEventListener(
  "updateValue",
  function (e) {
    var data = JSON.parse(e.data);
    var selectElement = document.getElementById(data.elementID);
    if (selectElement) {
      selectElement.value = data.value;
    }
  },
  false
);

// Elemente ein/ausblenden
evtSource.addEventListener(
  "enableElement",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      if (data.enable) {
        element.style.display = ""; // Element einblenden
      } else {
        element.style.display = "none"; // Element ausblenden
      }
    }
  },
  false
);

// Elemente ein/ausblenden
evtSource.addEventListener(
  "hideElements",
  function (e) {
    var data = JSON.parse(e.data);
    const elements = document.querySelectorAll(data.elements);
    elements.forEach((el) => {
      if (data.enable) {
        el.classList.add("hidden");
      } else {
        el.classList.remove("hidden");
      }
    });
  },
  false
);

evtSource.addEventListener("hideElementClass", (event) => {
  // Parsen des JSON-Strings aus dem Event-Daten
  const data = JSON.parse(event.data);
  const { className, hide } = data;
  console.log("class: " + className + " visibility " + hide);
  toggleElementVisibility(className, hide);
});

// set language
evtSource.addEventListener(
  "setLanguage",
  function (e) {
    var data = JSON.parse(e.data);
    localizePage(data.language);
  },
  false
);

evtSource.addEventListener(
  "add_log",
  function (event) {
    var logOutput = document.getElementById("p10_log_output");
    logOutput.innerHTML += event.data + "<br>";
  },
  false
);

evtSource.addEventListener(
  "clr_log",
  function (event) {
    var logOutput = document.getElementById("p10_log_output");
    logOutput.innerHTML = "";
  },
  false
);

// <<< START Sidebar Script >>>
let sidebar = document.querySelector(".sidebar");
let closeBtn = document.querySelector("#btn");

  closeBtn.addEventListener("click", ()=>{
    sidebar.classList.toggle("open");
    menuBtnChange();//calling the function(optional)
  });

  // Event-Listener für Tab-Menü
  document.querySelectorAll('.nav-list a').forEach(tab => {
    tab.onclick = function(e) {
        e.preventDefault();
        document.querySelectorAll('.nav-list a').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(content => content.classList.remove('active'));

        const activeTab = this.getAttribute('data-tab');
        this.classList.add('active');
        document.getElementById(activeTab).classList.add('active');
    };
  });

  // following are the code to change sidebar button(optional)
  function menuBtnChange() {
   if(sidebar.classList.contains("open")){
     closeBtn.classList.replace("bx-menu", "bx-menu-alt-right");//replacing the iocns class
   }else {
     closeBtn.classList.replace("bx-menu-alt-right","bx-menu");//replacing the iocns class
   }
  }

  // Funktion zum Umschalten des Menüs basierend auf der Fensterbreite
function toggleMenuBasedOnWidth() {
  const screenWidth = window.innerWidth;
  const threshold = 1024; // Schwellenwert für die Fensterbreite

  if(screenWidth > threshold) {
    // Fenster ist schmal, Menü einklappen
      sidebar.classList.add("open");
      menuBtnChange(); // Optional, ändert den Button
  } else {
    // Fenster ist breit, Menü ausklappen
      sidebar.classList.remove("open");
      menuBtnChange(); // Optional, ändert den Button
  }
}

// Event-Listener für Fenstergrößenänderung hinzufügen
window.addEventListener("resize", toggleMenuBasedOnWidth);

// Initialprüfung beim Laden der Seite
toggleMenuBasedOnWidth();
// <<< END Sidebar Script >>>
const translations = {
  system: {
    de: "System",
    en: "System",
  },
  control: {
    de: "Bedienung",
    en: "Control",
  },
  dashboard: {
    de: "Dashboard",
    en: "Dashboard",
  },
  boiler: {
    de: "Kessel",
    en: "Boiler",
  },
  auto: {
    de: "Auto",
    en: "Auto",
  },
  automatic: {
    de: "Automatik",
    en: "Automatic",
  },
  opmodes: {
    de: "Betriebsarten",
    en: "Operation Modes",
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
  settings: {
    de: "Einstellungen",
    en: "Settings",
  },
  op_values: {
    de: "Betriebswerte",
    en: "Operating-States",
  },
  temperatures: {
    de: "Temperaturen",
    en: "Temperatures",
  },
  manual: {
    de: "Handbetrieb",
    en: "Manual",
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
  summer: {
    de: "Sommer",
    en: "Summer",
  },
  winter: {
    de: "Winter",
    en: "Winter",
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
  setpoint: {
    de: "Sollwert",
    en: "Setpoint",
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
  on: {
    de: "EIN",
    en: "ON",
  },
  off: {
    de: "AUS",
    en: "OFF",
  },
  ok: {
    de: "OK",
    en: "OK",
  },
  error: {
    de: "FEHLER",
    en: "ERROR",
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
  wifi_info: {
    de: "WiFi-Information",
    en: "WiFi-Informations",
  },
  wifi_ip: {
    de: "IP-Adresse",
    en: "IP-Address",
  },
  wifi_signal: {
    de: "WiFi Signal",
    en: "WiFi Signal",
  },
  wifi_rssi: {
    de: "WiFi Rssi",
    en: "WiFi Rssi",
  },
  heating_circuit_1: {
    de: "Heizkreis 1",
    en: "Heating Circuit 1",
  },
  heating_circuit_2: {
    de: "Heizkreis 2",
    en: "Heating Circuit 2",
  },
  heating_circuit: {
    de: "Heizkreise",
    en: "Heating Circuits",
  },
  general: {
    de: "Allgemeine Werte",
    en: "General Values",
  },
  sw_version: {
    de: "Software-Version",
    en: "Software-Version",
  },
  logamatic_version: {
    de: "Logamatic-Version",
    en: "Logamatic-Version",
  },
  logamatic_modul: {
    de: "Logamatic-Modul",
    en: "Logamatic-Module",
  },
  version_info: {
    de: "Versionsinformationen",
    en: "Version Informations",
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
  info_unit_l: {
    de: "Einheit: Liter",
    en: "Unit: Litre",
  },
  oilmeter_act: {
    de: "\u00d6lz\u00e4hlerstand",
    en: "Oil-Meter value",
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
  voltage: {
    de: "Spannung",
    en: "Voltage",
  },
  esp_heapsize: {
    de: "ESP HeapSize",
    en: "ESP HeapSize",
  },
  esp_freeheap: {
    de: "ESP FreeHeap",
    en: "ESP FreeHeap",
  },
  esp_maxallocheap: {
    de: "ESP MaxAllocHeap",
    en: "ESP MaxAllocHeap",
  },
  esp_minfreeheap: {
    de: "ESP MinFreeHeap",
    en: "ESP MinFreeHeap",
  },
  esp_flash_usage: {
    de: "ESP Flash usage",
    en: "ESP Flash usage",
  },
  esp_heap_usage: {
    de: "ESP Heap usage",
    en: "ESP Heap usage",
  },
  sysinfo: {
    de: "Systeminformationen",
    en: "System Informations",
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
  message: {
    de: "Meldung",
    en: "Message",
  },
  esp_info: {
    de: "ESP-Info",
    en: "ESP-Info",
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
  date: {
    de: "Datum",
    en: "Date",
  },
  time: {
    de: "Uhrzeit",
    en: "Time",
  },
  logamatic: {
    de: "Logamatic",
    en: "Logamatic",
  },
  wifi: {
    de: "WiFi",
    en: "WiFi",
  },
  ssid: {
    de: "SSID",
    en: "SSID",
  },
  password: {
    de: "Passwort",
    en: "Password",
  },
  user: {
    de: "Benutzer",
    en: "User",
  },
  hostname: {
    de: "Hostname",
    en: "Hostname",
  },
  server: {
    de: "Server",
    en: "Server",
  },
  topic: {
    de: "Topic",
    en: "Topic",
  },
  mqtt: {
    de: "MQTT",
    en: "MQTT",
  },
  port: {
    de: "Port",
    en: "Port",
  },
  ntp: {
    de: "NTP-Server",
    en: "NTP-Server",
  },
  ntp_tz: {
    de: "Time-Zone",
    en: "Time-Zone",
  },
  act_date: {
    de: "aktuelles Datum",
    en: "actual date",
  },
  act_time: {
    de: "aktuelle Uhrzeit",
    en: "actual time",
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
  webui: {
    de: "Webserver",
    en: "Webserver",
  },
  save: {
    de: "Speichern",
    en: "save",
  },
  restart: {
    de: "Neustart",
    en: "restart",
  },
  save_restart: {
    de: "Speichern und Neustart",
    en: "save and restart",
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
  wifi_ap_info_1: {
    de: "\\n\u26a0\ufe0f um den ESP in den Accesspoint Mode zu versetzen \u26a0\ufe0f",
    en: "\\n\u26a0\ufe0f to set the ESP in Accesspoint Mode \u26a0\ufe0f",
  },
  wifi_ap_info_2: {
    de: "Reset dr\u00fccken und nach 5s erneut Reset dr\u00fccken",
    en: "press reset and after 5s press reset again",
  },
  predefine: {
    de: "Voreinstellung",
    en: "Predefine",
  },
  ota: {
    de: "OTA Firmware Update",
    en: "OTA Firmware Update",
  },
  filemgn: {
    de: "Dateimanager",
    en: "Filemanager",
  },
  tools: {
    de: "Tools",
    en: "Tools",
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
  activate: {
    de: "Aktivieren",
    en: "activate",
  },
  ip_settings: {
    de: "IP-Einstellungen",
    en: "IP settings",
  },
  ip_adr: {
    de: "IP-Adresse",
    en: "IP-Address",
  },
  ip_subnet: {
    de: "Subnetz",
    en: "Subnet",
  },
  ip_gateway: {
    de: "Gateway",
    en: "Gateway",
  },
  ip_dns: {
    de: "DNS-Server",
    en: "DNS-Server",
  },
  access: {
    de: "Zugangskontrolle",
    en: "Authentication",
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
  test: {
    de: "Test",
    en: "test",
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
  logout: {
    de: "ausloggen",
    en: "logout",
  },
  logger: {
    de: "Logbuch",
    en: "Logger",
  },
  save: {
    de: "Speichern",
    en: "save",
  },
  restart: {
    de: "Neustart",
    en: "restart",
  },
  clear: {
    de: "L\u00f6schen",
    en: "clear",
  },
  refresh: {
    de: "Aktualisieren",
    en: "refresh",
  },
  lifetime: {
    de: "Laufzeit",
    en: "Runtime",
  },
  restart_reason: {
    de: "Neustart Grund",
    en: "restart reason",
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
  program: {
    de: "Programm",
    en: "program",
  },
  holiday_days: {
    de: "Ferien_Tage",
    en: "holiday days",
  },
  timer: {
    de: "Timer",
    en: "timer",
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
  language: {
    de: "Sprache",
    en: "language",
  },
  german: {
    de: "Deutsch",
    en: "german",
  },
  english: {
    de: "Englisch",
    en: "english",
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
  ov1_manual: {
    de: "BW1 Manuell",
    en: "OV1 manual",
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
  on_time_opt_duration: {
    de: "Einschaltoptimierung",
    en: "On time opt duration",
  },
  off_time_opt_duration: {
    de: "Ausschaltoptimierung",
    en: "Off time opt duration",
  },
  pump: {
    de: "Pumpe",
    en: "Pump",
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
  ov1_holiday: {
    de: "BW1 Ferien",
    en: "OV1 holiday",
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
  ov2_day: {
    de: "BW2 Tag",
    en: "OV2 day",
  },
  ov2_hot: {
    de: "BW2 Warm",
    en: "OV2 hot",
  },
  ov2_priority: {
    de: "BW2 Vorrang",
    en: "OV2 priority",
  },
  setpoint: {
    de: "Solltemperatur",
    en: "setpoint",
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
  burner_runtime_minutes65536: {
    de: "Brenner Laufzeit Minuten65536",
    en: "Burner runtime minutes65536",
  },
  burner_runtime_minutes256: {
    de: "Brenner Laufzeit Minuten256",
    en: "Burner runtime minutes256",
  },
  burner_runtime_minutes: {
    de: "Brenner Laufzeit Minuten",
    en: "Burner runtime minutes",
  },
  burner_runtime_overall: {
    de: "Brenner Laufzeit Summe",
    en: "Burner runtime overall",
  },
  oil_consumption_overall_calc: {
    de: "Ölverbrauch Gesamt berechnet",
    en: "Oil consumption overall calc",
  },
  outside_temp: {
    de: "Außentemperatur",
    en: "Outside temp",
  },
  outside_temp_damped: {
    de: "Außentemperatur gedämpft",
    en: "Outside temp damped",
  },
  version_vk: {
    de: "Versionsnummer VK",
    en: "Version VK",
  },
  version_nk: {
    de: "Versionsnummer NK",
    en: "Version NK",
  },
  module_id: {
    de: "Modulkennung",
    en: "Module id",
  },
  err_alarm_exhaust: {
    de: "ERR Alarmstatus Abgasfühler",
    en: "ERR alarm exhaust",
  },
  err_alarm_02: {
    de: "ERR Alarmstatus 02",
    en: "ERR alarm 02",
  },
  err_alarm_boiler_flow_sensor: {
    de: "ERR Alarmstatus Kesselvorlauffühler",
    en: "ERR alarm boiler flow sensor",
  },
  err_alarm_08: {
    de: "ERR Alarmstatus 08",
    en: "ERR alarm 08",
  },
  err_alarm_burner: {
    de: "ERR Alarmstatus Brenner",
    en: "ERR alarm burner",
  },
  err_alarm_20: {
    de: "ERR Alarmstatus 20",
    en: "ERR alarm 20",
  },
  err_alarm_hk2_flow_sensor: {
    de: "ERR Alarmstatus HK2-Vorlauffuehler",
    en: "ERR alarm status HK2 flow sensor",
  },
  err_alarm_80: {
    de: "ERR Alarmstatus 80",
    en: "ERR alarm status 80",
  },
  error_buffer1: {
    de: "Fehlerspeicher1",
    en: "Error buffer1",
  },
  error_buffer2: {
    de: "Fehlerspeicher2",
    en: "Error buffer2",
  },
  error_buffer3: {
    de: "Fehlerspeicher3",
    en: "Error buffer3",
  },
  error_buffer4: {
    de: "Fehlerspeicher4",
    en: "Error buffer4",
  },
  lost_connection: {
    de: "Verbindung unterbrochen!",
    en: "lost connection",
  },
  send: {
    de: "senden",
    en: "send",
  },
  upload: {
    de: "hochladen",
    en: "upload",
  },
  update: {
    de: "Update",
    en: "update",
  },
  upload_ready: {
    de: "bereit für Upload",
    en: "Ready to upload",
  },
  file_content: {
    de: "Dateiübersicht",
    en: "File system content",
  },
};


// <<<< function for OTA update >>>>>> 
var updateActive = false;

function updateOTABar() {
    if (updateActive) {
        var xhr = new XMLHttpRequest();
        xhr.open('GET', '/getOTAProgress', true);
        xhr.responseType = 'json';
        xhr.onload = function() {
            if (xhr.status === 200) {
                const otaProgress = xhr.response.progress;
                document.getElementById('ota_progress_bar').value = otaProgress;
            }
        };
        xhr.send();
    }
}

function ota_sub_fun(obj) {
    var a = obj.value;
    console.log(a);
    var fileName = a.replace(/^.*[\\\/]/, '');
    console.log(fileName);
    document.getElementById('ota_file_input').textContent = fileName; // Geändert von innerHTML zu textContent
    document.getElementById('ota_updateBtn').disabled = false;
    document.getElementById('ota_upload_bar').style.display = 'block';
    document.getElementById('ota_progress_bar').style.display = 'block';
    document.getElementById('ota_txtUpload').style.display = 'block';
    document.getElementById('ota_txtUpdate').style.display = 'block';
}

document.querySelector('form').addEventListener('submit', function(e) {
    e.preventDefault();
    document.getElementById('ota_updateBtn').disabled = true;

    var form = document.getElementById('ota_upload_form'); // Geändert von jQuery zu nativem Selektor
    var data = new FormData(form);

    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/update', true);

    xhr.upload.onprogress = function(evt) {
        if (evt.lengthComputable) {
            var per = (evt.loaded / evt.total) * 100;
            document.getElementById('ota_progress').textContent = 'Progress: '; // Änderung der Methode
            document.getElementById('ota_progress_bar').value = per;
        }
    };

    xhr.onload = function() {
        if (xhr.status === 200) {
            console.log('success!');
            alert("OTA-Update successful - ESP will restart!");
            setTimeout(function() { location.href = '../ota'; }, 2000);
            updateActive = false;
        } else {
            alert("Upload error");
            setTimeout(function() { location.href = '../ota'; }, 1000);
        }
    };

    xhr.onerror = function() {
        alert("Upload error");
        setTimeout(function() { location.href = '../ota'; }, 1000);
    };

    xhr.send(data);
    updateActive = true;
});

setInterval(updateOTABar, 1000);


// <<<< function for File Upload >>>>>> 
function deletef(h) {
    var xhr = new XMLHttpRequest();
    var fnstring = "/delete?file=" + h;
    xhr.open("GET", fnstring, true);
    console.log(fnstring);
    xhr.send();
    xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 300) {
            // Erfolgreiche Antwort vom Server
            setTimeout(function() { location.href = '../filesystem'; }, 3000);
        } else {
            // Server hat einen Fehler zurückgegeben
            console.error('Deletion failed:', xhr.responseText);
        }
    };
}

function file_sub_fun(obj) {
    var a = obj.value;
    console.log(a);
    var fileName = a.replace(/^.*[\\\/]/, '');
    console.log(fileName);
    //document.getElementById('file_input').innerHTML = fileName;
    document.getElementById('file_updateBtn').disabled = false;
    document.getElementById('file_upload_bar').style.display = 'block';
    document.getElementById('txt_file_upload').style.display = 'block';
};

document.querySelector('form').addEventListener('submit', function(e) {
    e.preventDefault();
    document.getElementById('file_updateBtn').disabled = true;
    var form = document.getElementById('file_upload_form');
    var data = new FormData(form);

    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/doUpload', true);

    xhr.upload.onprogress = function(event) {
        if (event.lengthComputable) {
            var percentComplete = (event.loaded / event.total) * 100;
            console.log('Progress:', percentComplete);
            var progressBar = document.getElementById('file_upload_bar');
            progressBar.value = percentComplete;
            document.getElementById('file_progress').innerHTML = 'Progress: ' + Math.round(percentComplete) + '%';
        }
    };

    xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 300) {
            // Erfolgreiche Antwort vom Server
            console.log('Upload successful!');
            setTimeout(function() { location.href = '../filesystem'; }, 3000);
        } else {
            // Server hat einen Fehler zurückgegeben
            alert("Upload error");
            setTimeout(function() { location.href = '../filesystem'; }, 1000);
        }
    };

    xhr.onerror = function() {
        alert("Upload error");
        setTimeout(function() { location.href = '../filesystem'; }, 1000);
    };

    xhr.send(data);
});
