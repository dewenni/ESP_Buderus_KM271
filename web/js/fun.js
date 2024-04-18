// --------------------------------------
// js functions
// --------------------------------------

// Function timeout for server-side events
function resetPingTimer() {
  clearTimeout(pingTimer);
  pingTimer = setTimeout(function () {
    console.log("Ping timeout, attempting to reconnect...");
    evtSource.close();
    attemptReconnect();
    showReloadBar();
  }, 5000);
}

// Function to reconnect after timeout
function attemptReconnect() {
  setTimeout(setupSSE, 5000);
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
  a.download = "";
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
