// <<<< function for OTA update >>>>>>
var updateActive = false;

function updateOTABar() {
  if (updateActive) {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/getOTAProgress", true);
    xhr.responseType = "json";
    xhr.onload = function () {
      if (xhr.status === 200) {
        const otaProgress = xhr.response.progress;
        document.getElementById("ota_progress_bar").value = otaProgress;
      }
    };
    xhr.send();
  }
}

function ota_sub_fun(obj) {
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, "");
  console.log(fileName);
  document.getElementById("ota_file_input").textContent = fileName; // Geändert von innerHTML zu textContent
  document.getElementById("ota_updateBtn").disabled = false;
  document.getElementById("ota_upload_bar").style.display = "block";
  document.getElementById("ota_progress_bar").style.display = "block";
  document.getElementById("ota_txtUpload").style.display = "block";
  document.getElementById("ota_txtUpdate").style.display = "block";
}

document.querySelector("form").addEventListener("submit", function (e) {
  e.preventDefault();
  document.getElementById("ota_updateBtn").disabled = true;

  var form = document.getElementById("ota_upload_form"); // Geändert von jQuery zu nativem Selektor
  var data = new FormData(form);

  var xhr = new XMLHttpRequest();
  xhr.open("POST", "/update", true);

  xhr.upload.onprogress = function (evt) {
    if (evt.lengthComputable) {
      var per = (evt.loaded / evt.total) * 100;
      document.getElementById("ota_progress").textContent = "Progress: "; // Änderung der Methode
      document.getElementById("ota_progress_bar").value = per;
    }
  };

  xhr.onload = function () {
    if (xhr.status === 200) {
      console.log("success!");
      alert("OTA-Update successful - ESP will restart!");
      setTimeout(function () {
        location.href = "../ota";
      }, 2000);
      updateActive = false;
    } else {
      alert("Upload error");
      setTimeout(function () {
        location.href = "../ota";
      }, 1000);
    }
  };

  xhr.onerror = function () {
    alert("Upload error");
    setTimeout(function () {
      location.href = "../ota";
    }, 1000);
  };

  xhr.send(data);
  updateActive = true;
});

setInterval(updateOTABar, 1000);

// <<<< function for config import/export >>>>>>

// function for download config.json file
function exportConfig() {
  window.location.href = "/config-download";
}

function file_sub_fun(obj) {
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, "");
  console.log(fileName);
  document.getElementById("file_updateBtn").disabled = false;
  document.getElementById("file_upload_bar").style.display = "block";
  document.getElementById("txt_file_upload").style.display = "block";
}

document.querySelector("form").addEventListener("submit", function (e) {
  e.preventDefault();
  document.getElementById("file_updateBtn").disabled = true;
  var form = document.getElementById("file_upload_form");
  var data = new FormData(form);
});
