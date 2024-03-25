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

document.querySelector("form").addEventListener("submit", function (e) {
  e.preventDefault();
  document.getElementById("ota_status_txt").disabled = true;
  document.getElementById("ota_status_txt").textContent = "Uploading...";

  var form = document.getElementById("ota_upload_form");
  var data = new FormData(form);

  fetch("/update", {
    method: "POST",
    body: data,
  })
    .then((response) => {
      if (response.ok) {
        return response.text();
      }
      throw new Error("Network response was not ok.");
    })
    .then((data) => {
      console.log(data);
      document.getElementById("ota_status_txt").textContent = "Upload Complete";
    })
    .catch((error) => {
      console.error(
        "There has been a problem with your fetch operation:",
        error
      );
      document.getElementById("ota_status_txt").textContent = "Upload Failed";
    });

  var source = new EventSource("/events");
  source.addEventListener(
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
  source.addEventListener(
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
});

document
  .getElementById("p11_ota_failed_btn")
  .addEventListener("click", function () {
    document.getElementById("ota_update_failed_dialog").close();
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

    xhr.onload = function () {
      if (xhr.status === 200) {
        document.getElementById("upload_status_txt").style.display = "block";
        document.getElementById("upload_status_txt").innerText =
          "Upload erfolgreich";
      } else {
        document.getElementById("upload_status_txt").style.display = "block";
        document.getElementById("upload_status_txt").innerText =
          "Upload fehlgeschlagen";
      }
    };
    xhr.send(formData);
  });
