// --------------------------------------
// Server-Side-Events (Client <- ESP)
// --------------------------------------
let pingTimer;
let evtSource;

function setupSSE() {
  evtSource = new EventSource("/events");

  evtSource.addEventListener(
    "open",
    function (e) {
      console.log("Events Connected");
      resetPingTimer();
      hideReloadBar();
    },
    false
  );
  evtSource.addEventListener(
    "error",
    function (e) {
      if (e.target.readyState != EventSource.OPEN) {
        console.log("Events Disconnected");
        evtSource.close();
        attemptReconnect();
      }
    },
    false
  );

  evtSource.addEventListener(
    "ping",
    function (e) {
      console.log("Ping received");
      resetPingTimer();
    },
    false
  );

  // update Text elements
  evtSource.addEventListener(
    "updateText",
    function (e) {
      var data = JSON.parse(e.data);
      var element = document.getElementById(data.id);
      if (element) {
        if (data.isInput) {
          element.value = data.text;
        } else {
          element.innerHTML = data.text;
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
      var element = document.getElementById(data.id);
      if (
        element &&
        (element.type === "checkbox" || element.type === "radio")
      ) {
        element.checked = data.state;
        toggleElementVisibility(
          element.getAttribute("hideOpt"),
          element.checked
        );
      }
    },
    false
  );

  // update element.value
  evtSource.addEventListener(
    "updateValue",
    function (e) {
      var data = JSON.parse(e.data);
      var selectElement = document.getElementById(data.id);
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
      var element = document.getElementById(data.id);
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
      var element = document.getElementById(data.id);
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
      var element = document.getElementById(data.id);
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
      var element = document.getElementById(data.id);
      if (element) {
        element.setAttribute("aria-busy", data.busy);
      }
    },
    false
  );

  // hide/show elements based on className
  evtSource.addEventListener("showElementClass", (event) => {
    console.log("showElementClass");
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
      console.log("set language");
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
      var data = JSON.parse(event.data);
      Object.keys(data).forEach(function (key) {
        let [elementID, typSuffix] = key.split("#");
        let element = document.getElementById(elementID);
        if (!element) {
          console.error("unknown element:", element);
          return;
        }
        let value = data[key];
        switch (typSuffix) {
          case "v": // value
            element.value = value;
            break;
          case "c": // checked
            element.checked = value === "true";
            toggleElementVisibility(
              element.getAttribute("hideOpt"),
              element.checked
            );
            break;
          case "l": // Label = innerHTML
            element.innerHTML = value;
            break;
          case "i": // icon
            element.className = "svg " + value;
            break;
          default:
            console.error("unknown typ:", typSuffix);
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
      var dialog = document.getElementById(data.id);
      if (data.state == "open") {
        dialog.showModal();
      } else if (data.state == "close") {
        dialog.close();
      }
    },
    false
  );

  evtSource.addEventListener(
    "updateTooltip",
    function (e) {
      var data = JSON.parse(e.data);
      const element = document.getElementById(data.id);
      element.setAttribute("data-tooltip", data.tooltip);
    },
    false
  );
}
