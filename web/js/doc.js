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
    .getElementById("p00_ota_failed_btn")
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
