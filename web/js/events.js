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
      element.style.display = data.hide ? "none" : "";
    }
  },
  false
);

// update href
evtSource.addEventListener(
  "updateHref",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      element.href = data.href;
    }
  },
  false
);

// update Busy
evtSource.addEventListener(
  "updateBusy",
  function (e) {
    var data = JSON.parse(e.data);
    var element = document.getElementById(data.elementID);
    if (element) {
      element.setAttribute("aria-busy", data.busy);
    }
  },
  false
);

// hide/show elements based on className
evtSource.addEventListener("showElementClass", (event) => {
  const data = JSON.parse(event.data);
  const elements = document.querySelectorAll(`.${data.className}`);
  elements.forEach((element) => {
    element.style.display = data.show ? "inline-flex" : "none";
  });
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

// JSON message for grouped messages
evtSource.addEventListener(
  "updateJSON",
  function (event) {
    var updates = JSON.parse(event.data);
    updates.forEach(function (update) {
      var element = document.getElementById(update.i);

      if (element) {
        switch (update.t) {
          case "v":
            element.value = update.v;
            break;
          case "c":
            element.checked = update.v;
            toggleElementVisibility(
              element.getAttribute("hideOpt"),
              element.checked
            );
            break;
          case "l":
            element.innerHTML = update.v;
            break;
          case "i":
            element.className = "svg " + update.v;
            break;
          default:
            console.error("unknown typ:", update.t);
        }
      } else {
        console.error("element not found:", update.i);
      }
    });
  },
  false
);

// update ota-progress bar
evtSource.addEventListener(
  "ota-progress",
  function (e) {
    var progress = parseInt(e.data.replace("Progress: ", ""), 10);
    document.getElementById("ota_progress_bar").value = progress;
    document.getElementById(
      "ota_status_txt"
    ).textContent = `Update Progress: ${progress}%`;
  },
  false
);

// close update dialog
evtSource.addEventListener(
  "updateDialog",
  function (e) {
    var data = JSON.parse(e.data);
    var dialog = document.getElementById(data.elementID);
    if (data.state == "open") {
      dialog.showModal();
    } else if (data.state == "close") {
      dialog.close();
    }
  },
  false
);
