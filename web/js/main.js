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
      'input[type="text"], input[type="number"], input[type="password"], input[type="date"], input[type="time"]'
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
