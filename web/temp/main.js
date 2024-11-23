let ws;
let heartbeatTimeout;
const maxReconnectDelay = 5000;
const minReconnectDelay = 1000;
let reconnectDelay = minReconnectDelay;

function setupWS() {
  ws = new WebSocket("ws://" + window.location.host + "/ws");

  ws.onopen = function () {
    console.log("WebSocket connected");
    resetHeartbeat();
    hideReloadBar();
    reconnectDelay = minReconnectDelay;
  };

  ws.onclose = function () {
    console.log("WebSocket disconnected. Retrying...");
    attemptReconnect();
    showReloadBar();
  };

  ws.onmessage = function (event) {
    let message = JSON.parse(event.data);
    //console.log(message);
    if (message.type === "redirect") {
      console.log("Redirecting to:", message.url);
      ws.close();
      window.location.href = message.url;
    } else if (message.type === "heartbeat") {
      resetHeartbeat();
    } else if (message.type === "loadConfig") {
      loadConfig();
    } else if (message.type === "updateText") {
      updateText(message);
    } else if (message.type === "setLanguage") {
      setLanguage(message);
    } else if (message.type === "updateJSON") {
      updateJSON(message);
    } else if (message.type === "updateValue") {
      updateValue(message);
    } else if (message.type === "updateState") {
      updateState(message);
    } else if (message.type === "updateSetIcon") {
      updateSetIcon(message);
    } else if (message.type === "hideElement") {
      hideElement(message);
    } else if (message.type === "updateHref") {
      updateHref(message);
    } else if (message.type === "updateBusy") {
      updateBusy(message);
    } else if (message.type === "showElementClass") {
      showElementClass(message);
    } else if (message.type === "logger") {
      logger(message);
    } else if (message.type === "cmdLogClr") {
      cmdLogClr(message);
    } else if (message.type === "otaProgress") {
      otaProgress(message);
    } else if (message.type === "updateDialog") {
      updateDialog(message);
    }
  };
}

// Heartbeat-Timeout zurücksetzen
function resetHeartbeat() {
  clearTimeout(heartbeatTimeout);
  heartbeatTimeout = setTimeout(() => {
    console.warn("No heartbeat received, reconnecting...");
    ws.close();
    showReloadBar();
  }, 5000);
}

function attemptReconnect() {
  setTimeout(() => {
    console.log(`Attempting reconnect in ${reconnectDelay / 1000} seconds...`);
    setupWS();
    reconnectDelay = Math.min(reconnectDelay * 2, maxReconnectDelay);
  }, reconnectDelay);
}

function sendData(elementId, value) {
  if (ws.readyState === WebSocket.OPEN) {
    const message = {
      type: "sendData",
      elementId: elementId,
      value: String(value),
    };
    ws.send(JSON.stringify(message));
  } else {
    console.error("WebSocket is not open. Cannot send data.");
  }
}

function updateText(data) {
  var element = document.getElementById(data.id);
  //console.log(element);
  if (element) {
    if (data.isInput) {
      element.value = data.text;
    } else {
      element.innerHTML = data.text;
    }
  }
}

function setLanguage(data) {
  console.log("Set language to:", data.language);
  localizePage(data.language);
}

function updateJSON(data) {
  Object.keys(data).forEach(function (key) {
    if (key !== "type") {
      // skip first element "type"
      let elementID = key;
      let element = document.getElementById(elementID);
      if (!element) {
        console.error("unknown element:", key);
        return;
      }
      let value = data[key];

      if (element.tagName === "INPUT") {
        if (element.type === "checkbox" || element.type === "radio") {
          element.checked = value === "true";
        } else if (element.type === "range") {
          element.value = value;
          let linkedTextElementId = element.getAttribute("data-value-id");
          if (linkedTextElementId) {
            let linkedTextElement =
              document.getElementById(linkedTextElementId);
            if (linkedTextElement) {
              linkedTextElement.innerHTML = value;
            }
          }
        } else {
          // all other input elements
          element.value = value;
        }
      } else if (element.tagName === "SELECT") {
        element.value = value;
        // check if value is valid
        if (
          !Array.from(element.options).some((option) => option.value === value)
        ) {
          console.warn(
            `Value "${value}" not found in <select> options for element:`,
            key
          );
        }
      } else if (element.tagName === "I") {
        // change icons <i>
        element.className = "svg " + value;
      } else if ("innerHTML" in element) {
        // all other elements with `innerHTML` (<td>, <div>, <span>, ...)
        element.innerHTML = value;
      } else {
        console.error("unhandled element type:", element.tagName);
      }
    }
  });
}

function updateValue(data) {
  var element = document.getElementById(data.id);
  if (element) {
    if (data.isInput) {
      element.value = data.value;
    }
  }
}

// update switch elements
function updateState(data) {
  var element = document.getElementById(data.id);
  if (element && (element.type === "checkbox" || element.type === "radio")) {
    element.checked = data.state;
    toggleElementVisibility(element.getAttribute("hideOpt"), element.checked);
  }
}

// update add class to element
function updateSetIcon(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.className = "svg " + data.icon;
  }
}

// hide/show element
function hideElement(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.style.display = data.hide ? "none" : "";
  }
}

// update href
function updateHref(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.href = data.href;
  }
}

// update Busy
function updateBusy(data) {
  var element = document.getElementById(data.id);
  if (element) {
    element.setAttribute("aria-busy", data.busy);
  }
}

// hide/show elements based on className
function showElementClass(data) {
  const elements = document.querySelectorAll(`.${data.className}`);
  elements.forEach((element) => {
    element.style.display = data.show ? "inline-flex" : "none";
  });
}

// add log message
function logger(data) {
  var logOutput = document.getElementById("p10_log_output");
  if (data.cmd === "add_log") {
    logOutput.innerHTML = "";
    data.entry.forEach(function (entry) {
      logOutput.innerHTML += entry + "<br>";
    });
  } else if (data.cmd === "clr_log") {
    logOutput.innerHTML = "";
  }
}

// update ota-progress bar
function otaProgress(data) {
  clearTimeout(heartbeatTimeout);
  var progress = data.progress;
  document.getElementById("ota_progress_bar").value = progress;
  document.getElementById(
    "ota_status_txt"
  ).textContent = `Update Progress: ${progress}%`;
}

// close update dialog
function updateDialog(data) {
  var dialog = document.getElementById(data.id);
  if (data.state == "open") {
    dialog.showModal();
  } else if (data.state == "close") {
    dialog.close();
  }
}

// Function for switching the visibility of elements
function toggleElementVisibility(className, isVisible) {
  const elements = document.querySelectorAll(`.${className}`);
  elements.forEach((element) => {
    element.style.display = isVisible ? "" : "none";
  });
}

// Function for initializing the visibility based on the status of the switches
function initializeVisibilityBasedOnSwitches() {
  document
    .querySelectorAll('input[type="checkbox"][role="switch"]')
    .forEach(function (switchElement) {
      // Evaluate the status of the switch and adjust visibility
      toggleElementVisibility(
        switchElement.getAttribute("hideOpt"),
        switchElement.checked
      );
    });
}

// OTA: function is called when the ota-file is selected
function ota_sub_fun(obj) {
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, "");
  console.log(fileName);
  document.getElementById("ota_file_input").textContent = fileName;
  document.getElementById("ota_update_btn").disabled = false;
  document.getElementById("ota_progress_bar").style.display = "block";
  document.getElementById("ota_status_txt").style.display = "block";
}

// CONFIG-FORM: function for download config.json file
function exportConfig() {
  var a = document.createElement("a");
  a.href = "/config-download";
  a.download = "config.json";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
}

// CONFIG-FORM: function to activate import button and show status
function file_sub_fun(obj) {
  document.getElementById("file_upload_btn").disabled = false;
  document.getElementById("upload_status_txt").style.display = "block";
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

// localization of web texts
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

// to show reload bar if connection is lost
function showReloadBar() {
  document.getElementById("connectionLostBar").style.display = "flex";
}

// to hide reload bar if connection is lost
function hideReloadBar() {
  document.getElementById("connectionLostBar").style.display = "none";
}

// update elements based on config.json file
function updateUI(
  config,
  prefix = "cfg",
  ignoreKeys = ["debug", "webUI_enable"]
) {
  for (const key in config) {
    if (config.hasOwnProperty(key)) {
      // Prüfen, ob der aktuelle Schlüssel ignoriert werden soll
      if (ignoreKeys.includes(key)) {
        console.log("config update - key ignored: " + key);
        continue;
      }

      const value = config[key];
      const elementId = `${prefix}_${key}`;

      // Überprüfen, ob das aktuelle Element ein verschachteltes Objekt ist
      if (typeof value === "object" && value !== null) {
        // Rekursiv weiter ins Objekt gehen, mit dem erweiterten Prefix
        updateUI(value, elementId, ignoreKeys);
      } else {
        // UI-Element mit dem zusammengesetzten ID suchen
        const element = document.getElementById(elementId);
        if (element) {
          // Unterscheide die Art des HTML-Elements
          if (element.type === "checkbox") {
            // Setze das "checked"-Attribut für Checkboxen
            element.checked = value === true;
            toggleElementVisibility(
              element.getAttribute("hideOpt"),
              element.checked
            );
          } else if (element.type === "radio") {
            // Setze das "checked"-Attribut für Radio-Buttons
            element.checked = element.value === value.toString();
          } else {
            // Setze das "value"-Attribut für andere Eingabetypen (z.B. text, number)
            element.value = value;
          }
        } else {
          console.error("config update - elementId not found: " + elementId);
        }
      }
    }
  }
}

// Config bei Seitenaufruf laden und UI aktualisieren
async function loadConfig() {
  try {
    const response = await fetch("/config.json");
    if (!response.ok)
      throw new Error("Fehler beim Abrufen der Konfigurationsdaten");

    const config = await response.json();

    // UI-Elemente basierend auf der geladenen config.json aktualisieren
    updateUI(config);
  } catch (error) {
    console.error("Error loading config:", error);
  }
}

// --------------------------------------
// --------------------------------------
document.addEventListener("DOMContentLoaded", function () {
  // call functions on refresh
  setupWS();
  initializeVisibilityBasedOnSwitches();
  localizePage("de");
  loadConfig();

  // Event Listener for Reload-Button
  document
    .getElementById("p99_reloadButton")
    .addEventListener("click", function () {
      window.location.reload();
    });

  // VERSION: is called when version dialog is opened
  document.getElementById("p00_version").addEventListener("click", function () {
    document.getElementById("version_dialog").showModal();
    sendData("check_git_version", "");
  });

  // VERSION: close version dialog on button click
  document
    .getElementById("close_version_Dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("version_dialog").close();
    });

  // OTA: close ota-failed dialog on button click
  document
    .getElementById("p11_ota_failed_btn")
    .addEventListener("click", function () {
      document.getElementById("ota_update_failed_dialog").close();
    });

  // OTA: send form data to server
  document
    .getElementById("ota_upload_form")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      document.getElementById("ota_status_txt").textContent = "Uploading...";
      var form = document.getElementById("ota_upload_form");
      var formData = new FormData(form);
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/update", true);
      xhr.send(formData);
    });

  // CONFIG: send form data for config file upload
  document
    .getElementById("file_upload_form")
    .addEventListener("submit", function (event) {
      event.preventDefault();
      var form = document.getElementById("file_upload_form");
      var formData = new FormData(form);
      var xhr = new XMLHttpRequest();
      xhr.open("POST", "/config-upload", true);
      xhr.send(formData);
    });

  // CONFIG: open dialog to show config.json
  document
    .getElementById("p11_file_show_btn")
    .addEventListener("click", function () {
      const dialog = document.getElementById("open_config_dialog");
      dialog.showModal();
      fetch("/config.json")
        .then((response) => response.json())
        .then((data) => {
          document.getElementById("config_output").textContent = JSON.stringify(
            data,
            null,
            2
          );
        })
        .catch((error) => console.error("error loading data:", error));
    });

  // CONFIG: close dialog to show config.json
  document
    .getElementById("p11_config_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("open_config_dialog").close();
    });

  // NTP: open dialog to show ntp timezones
  document
    .getElementById("p12_ntp_open_dialog_btn")
    .addEventListener("click", function () {
      fetch("/gzip_ntp")
        .then((response) => response.text())
        .then((data) => {
          document.getElementById("p12_ntp_help_output").innerHTML = data;
          document.getElementById("p12_ntp_dialog").showModal();
        })
        .catch((error) => console.error("error loading data:", error));
    });

  // NTP: close dialog to show ntp timezones
  document
    .getElementById("p12_ntp_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_ntp_dialog").close();
    });

  // ETH: open dialog to show w5500 configuration
  document
    .getElementById("p12_eth_open_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_eth_dialog").showModal();
    });

  // ETH: close dialog to show w5500 configuration
  document
    .getElementById("p12_eth_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_eth_dialog").close();
    });

  // GPIO: open dialog to show GPIO configuration
  document
    .getElementById("p12_gpio_open_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_gpio_dialog").showModal();
    });

  // GPIO: close dialog to show GPIO configuration
  document
    .getElementById("p12_gpio_close_dialog_btn")
    .addEventListener("click", function () {
      document.getElementById("p12_gpio_dialog").close();
    });

  // control for Tab-Menu
  document.querySelectorAll(".nav-list a").forEach((tab) => {
    tab.onclick = function (e) {
      e.preventDefault();
      document
        .querySelectorAll(".nav-list a")
        .forEach((t) => t.classList.remove("active"));
      document
        .querySelectorAll(".tab-content")
        .forEach((content) => content.classList.remove("active"));

      const activeTab = this.getAttribute("data-tab");
      this.classList.add("active");
      document.getElementById(activeTab).classList.add("active");
    };
  });

  // language selection
  document.getElementById("cfg_lang").addEventListener("change", function () {
    var languageValue = this.value;
    if (languageValue === "1") {
      localizePage("en");
    } else if (languageValue === "0") {
      localizePage("de");
    }
  });

  // event listener for all input fields that call sendData on "blur"
  document
    .querySelectorAll(
      'input[type="text"], input[type="number"], input[type="password"], input[type="date"], input[type="time"]'
    )
    .forEach(function (input) {
      input.addEventListener("blur", function () {
        sendData(input.id, input.value);
      });
      input.addEventListener("keypress", function (e) {
        if (e.key === "Enter") {
          input.blur(); // Triggers "blur" event and sends data
        }
      });
    });

  // Event-Listener for Range Inputs (Slider)
  document.querySelectorAll('input[type="range"]').forEach(function (slider) {
    slider.addEventListener("change", function () {
      sendData(slider.id, slider.value);
    });
  });

  // Event-Listener for Slider labels
  document.querySelectorAll(".rangeSlider").forEach((slider) => {
    const valueId = slider.getAttribute("data-value-id");
    const valueDisplay = document.getElementById(valueId);
    slider.oninput = () => {
      valueDisplay.textContent = slider.value;
    };
    // set initial value
    valueDisplay.textContent = slider.value;
  });

  // Event-Listener for Buttons
  document.querySelectorAll("button").forEach(function (button) {
    button.addEventListener("click", function () {
      sendData(button.id, true);
    });
  });

  // Event-Listener for Switches
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

  // Event-Listener for Radio
  document
    .querySelectorAll('input[type="radio"]')
    .forEach(function (switchElement) {
      switchElement.addEventListener("change", function () {
        sendData(switchElement.id, switchElement.checked);
      });
    });

  // Event-Listener for Select Elements
  document.querySelectorAll("select").forEach(function (selectElement) {
    selectElement.addEventListener("change", function () {
      sendData(selectElement.id, selectElement.value);
    });
  });

  // Event-Listener for pushover test-message
  document
    .getElementById("p12_pushover_test_btn")
    .addEventListener("click", function () {
      var text = document.getElementById("p12_pushover_test_msg").value;
      sendData("p12_pushover_test_msg_cmd", text);
    });

  // Event-Listener for oilcounter set value
  document
    .getElementById("p02_oilmeter_btn")
    .addEventListener("click", function () {
      var text = document.getElementById("p02_oilmeter_set").value;
      sendData("p02_oilmeter_set_cmd", text);
    });
});

// --------------------------------------
// localization texts
// --------------------------------------
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
  ok: {
    de: "OK",
    en: "OK",
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
  network_info: {
    de: "Netzwerk-Information",
    en: "Network-Informations",
  },
  wifi_ip: {
    de: "WiFi IP-Adresse",
    en: "WiFi IP-Address",
  },
  wifi_signal: {
    de: "WiFi Signal",
    en: "WiFi Signal",
  },
  wifi_rssi: {
    de: "WiFi Rssi",
    en: "WiFi Rssi",
  },
  eth_ip: {
    de: "ETH IP-Adresse",
    en: "ETH IP-Address",
  },
  eth_status: {
    de: "ETH Status",
    en: "ETH Status",
  },
  static_ip: {
    de: "feste IP-Adresse",
    en: "static ip address",
  },
  eth_link_speed: {
    de: "ETH Geschwindigkeit",
    en: "ETH Link Speed",
  },
  eth_full_duplex: {
    de: "ETH Vollduplex",
    en: "ETH Full Duplex",
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
  restart: {
    de: "Neustart",
    en: "restart",
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
  ota: {
    de: "OTA Firmware Update",
    en: "OTA Firmware Update",
  },
  config_mgn: {
    de: "Konfiguration import/export",
    en: "configuration import/export",
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
  logger: {
    de: "Logbuch",
    en: "Logger",
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
  lost_connection: {
    de: "Verbindung unterbrochen!",
    en: "lost connection",
  },
  send: {
    de: "senden",
    en: "send",
  },
  sort_up: {
    de: "aufsteigend \u2B06",
    en: "ascending \u2B06",
  },
  sort_down: {
    de: "absteigend \u2B07",
    en: "descending \u2B07",
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
  log_mode_6: {
    de: "Modus: SystemLog",
    en: "Mode: SystemLog",
  },
  import: {
    de: "import (config.json)",
    en: "import (config.json)",
  },
  export: {
    de: "export (config.json)",
    en: "export (config.json)",
  },
  open: {
    de: "öffne (config.json)",
    en: "open (config.json)",
  },
  update_done: {
    de: "Update erfolgreich!",
    en: "Update sucessfull!",
  },
  update_failed: {
    de: "Update nicht erfolgreich!",
    en: "Update not sucessfull!",
  },
  update_ready: {
    de: "Update bereit!",
    en: "Update ready!",
  },
  update_info: {
    de: "ESP muss neu gestartet werden um das Update zu übernehmen.",
    en: "ESP must be restarted to apply the update",
  },
  error_text: {
    de: "Fehler:",
    en: "Error:",
  },
  update_error_info: {
    de: "Bitte das Update erneut ausführen!",
    en: "Please run the update again!",
  },
  import_ready: {
    de: "Import bereit!",
    en: "import ready!",
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
  close: {
    de: "schließen",
    en: "close",
  },
  act_version: {
    de: "aktuelle Version",
    en: "actual version",
  },
  github_version: {
    de: "neustes Release",
    en: "newest release",
  },
  select_template: {
    de: "wähle Vorlage...",
    en: "select template...",
  },
  km271_refresh: {
    de: "empfange Daten...",
    en: "receiving data...",
  },
  mqtt_info: {
    de: "MQTT-Informationen",
    en: "MQTT information",
  },
  last_error: {
    de: "letzter Fehler",
    en: "last error",
  },
  connection: {
    de: "Verbindung",
    en: "connection",
  },
  state: {
    de: "Zustand",
    en: "State",
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
};

