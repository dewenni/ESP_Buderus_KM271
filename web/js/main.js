// <<<< functions >>>>>>

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
  clearTimeout(pingTimeout);
  pingTimeout = setTimeout(function () {
    console.log("Ping Timeout - Keine Ping-Nachricht empfangen.");
    showReloadBar();
  }, 5000); // 5 seconds waiting time for the ping
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

  // vent listener for all input fields that call sendData on "blur"
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
});

// <<<< Server-Side-Events (Client <- ESP) >>>>>>
var evtSource = new EventSource("/events");

// Event Listener for Reload-Button
document
  .getElementById("p99_reloadButton")
  .addEventListener("click", function () {
    window.location.reload();
  });

// Listener for Ping-Message
evtSource.addEventListener(
  "ping",
  function (e) {
    console.log("Ping-Nachricht empfangen");
    resetPingTimeout();
  },
  false
);

// update Text elements
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

// update switch elements
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

// update element.value
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

// update add class to element
evtSource.addEventListener(
  "updateSetIcon",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      element.className = "svg " + data.icon;
    }
  },
  false
);

// hide/show element
evtSource.addEventListener(
  "hideElement",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      style.display = data.hide ? "" : "none";
    }
  },
  false
);

// hide/show elements based on className
evtSource.addEventListener("hideElementClass", (event) => {
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

// add log message
evtSource.addEventListener(
  "add_log",
  function (event) {
    var logOutput = document.getElementById("p10_log_output");
    logOutput.innerHTML += event.data + "<br>";
  },
  false
);

// clear log
evtSource.addEventListener(
  "clr_log",
  function (event) {
    var logOutput = document.getElementById("p10_log_output");
    logOutput.innerHTML = "";
  },
  false
);
