// --------------------------------------
// --------------------------------------
document.addEventListener("DOMContentLoaded", function () {
  // call user functions
  initLogType();
  loadSimulatedData();

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

  // refresh log when tab10 is clicked
  document.querySelectorAll('a[data-tab="tab10"]').forEach(function (el) {
    el.addEventListener("click", function (e) {
      e.preventDefault();
      console.log("refresh log");
      sendData("refresh_log", "");
    });
  });
});
