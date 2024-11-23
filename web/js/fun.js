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
