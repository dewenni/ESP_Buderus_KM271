// <<<< function for OTA update >>>>>>
function ota_sub_fun(obj) {
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, "");
  console.log(fileName);
  document.getElementById("ota_file_input").textContent = fileName; // Geändert von innerHTML zu textContent
  document.getElementById("ota_update_btn").disabled = false;
  document.getElementById("ota_progress_bar").style.display = "block";
  document.getElementById("ota_status_txt").style.display = "block";
}

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

// <<<< function for config >>>>>>

// function for download config.json file
function exportConfig() {
  window.location.href = "/config-download";
}
// function to activate import button and show status
function file_sub_fun(obj) {
  document.getElementById("file_upload_btn").disabled = false;
  document.getElementById("upload_status_txt").style.display = "block";
}

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
