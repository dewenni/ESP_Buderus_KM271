/* P R O T O T Y P E S ********************************************************/ 
void webToolsSetup();
void webToolsCyclic();

/* H T M L - S O U R C E C O D E **********************************************/  
const char ota_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html class="HTML">
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
input{background:#f1f1f1;border:0;padding:0 5px}
body{background:#444857;font-family:sans-serif;font-size:14px;color:#ffffff}
#file-input,input{height:44px;border-radius:4px;margin:10px auto;font-size:15px}
#file-input{padding:0 10px;box-sizing:border-box;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;}
#bar,#prgbar,#ota-progress-bar,#ota-progress{background-color:#f1f1f1;border-radius:10px}#bar,#ota-progress{background-color:#ca7125;width:0%;height:10px;}
form{background:#444857;max-width:350px;margin:5px auto;padding:5px; padding-bottom: 5px;border: 1px solid #444857;border-radius:5px;text-align:center; align-content: center;}
.btn2 {background-color: #e29425;box-shadow: 0 0 8px 0 rgba(0, 0, 0, 0.08), 0 0 15px 0 rgba(0, 0, 0, 0.02), 0 0 20px 4px rgba(0, 0, 0, 0.06);color: white;border-radius: 4px;padding: 9px 16px;background-image: none;
text-decoration: none;border: none;letter-spacing:1.25px;cursor: pointer;text-transform:uppercase;font-size:14px;line-height: 18px;display: inline-block;vertical-align: middle;transition-duration: 0.1s;}
.btn2:hover:enabled {background-color: #e29425;box-shadow: 0px 2px 4px -1px rgba(0, 0, 0, 0.2), 0px 4px 5px 0px rgba(0, 0, 0, 0.14), 0px 1px 10px 0px rgba(0,0,0,.12);}
.btn2:visited {color: white;}
.btn2:focus {outline: none;}
.btn2:active {color: white;background-color: #ca7125;}
.btn2:disabled {cursor: default;background: #DDD;}
   html {
     font-family: Roboto, Arial, sans-serif;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; 
       font-family: Arial;
      text-align: center;
      font-weight: normal;
      color: #fafcfc;
     }
    p { font-size: 3.0rem; margin-top: 0;}
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
      font-weight: normal;
      color: #333333;
    }
</style>
</head>
<title>FIRMWARE UPDATE</title>
<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script>
</head><body>
<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>
<input type='file' name='update' id='file' onchange='sub(this)' style=display:none accept=".bin">
<label id='file-input' for='file'>   Choose file...</label>
<input type="submit" id="updateBtn" class="btn2" disabled = "disabled" value="Update">
<br><div id='txtUpload'style=display:none;text-align:left>Upload:</div>
<br><div id='prgbar'style=display:none><div id='bar'></div></div>
<br><div id='txtUpdate'style=display:none;text-align:left>Update:</div>
<br><div id='ota-progress-bar'style=display:none><div id='ota-progress'></div></div>
</form>
<script>
function updateOTABar() {
  $.ajax({
    url: '/getOTAProgress', // Endpoint zum Abrufen des OTA-Fortschritts vom ESP32
    method: 'GET',
    dataType: 'json',
    success: function(data) {
      const otaProgress = data.progress;
      $('#ota-progress').css('width', otaProgress + '%');
    }
  });
}
setInterval(updateOTABar, 1000); 

function sub(obj){
var a = obj.value;
console.log(a);
var fileName = a.replace(/^.*[\\\/]/, '')
console.log(fileName);
document.getElementById('file-input').innerHTML = fileName;
document.getElementById('updateBtn').disabled = false;
document.getElementById('prgbar').style.display = 'block';
document.getElementById('ota-progress-bar').style.display = 'block';
document.getElementById('txtUpload').style.display = 'block';
document.getElementById('txtUpdate').style.display = 'block';
};
$('form').submit(function(e){
  document.getElementById('updateBtn').disabled = "disabled"; 
e.preventDefault();
var form = $('#upload_form')[0];
var data = new FormData(form);
$.ajax({
url: '/update',
type: 'POST',
data: data,
contentType: false,
processData:false,
xhr: function() {
var xhr = new window.XMLHttpRequest();
xhr.upload.addEventListener('progress', function(evt) {
if (evt.lengthComputable) {
var per = evt.loaded / evt.total;
$('#prg').html('Progress:');
$('#bar').css('width',Math.round(per*100) + "%");
}
}, false);
return xhr;
},
success:function(d, s) {
console.log('success!'); 
alert("OTA-Update successful - ESP will restart!");
setTimeout("location.href = '../ota';", 2000);
},
error: function (a, b, c) {
}
});
});
</script></body></html>)rawliteral";

const char FS_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<!-- saved from url=(0031)http://192.168.0.160/filesystem -->
<html lang="en"><head><meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
<style>
input{background:#f1f1f1;border:0;padding:0 5px}
body{background:#444857;font-family:sans-serif;font-size:14px;color:#ffffff}
#file-input,input{height:44px;border-radius:4px;margin:10px auto;font-size:15px}
#file-input{padding:0 10px;box-sizing:border-box;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;}
#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#e29425;width:0;height:10px;}
form{background:#444857;max-width:350px;margin:5px auto;padding:5px; padding-bottom: 5px;border: 1px solid #444857;border-radius:5px;text-align:center; align-content: center;}
.btn2 {background-color: #e29425;box-shadow: 0 0 8px 0 rgba(0, 0, 0, 0.08), 0 0 15px 0 rgba(0, 0, 0, 0.02), 0 0 20px 4px rgba(0, 0, 0, 0.06);color: white;border-radius: 4px;padding: 9px 16px;background-image: none;
text-decoration: none;border: none;letter-spacing:1.25px;cursor: pointer;text-transform:uppercase;font-size:14px;line-height: 18px;display: inline-block;vertical-align: middle;transition-duration: 0.1s;}
.btn2:hover:enabled {background-color: #e29425;box-shadow: 0px 2px 4px -1px rgba(0, 0, 0, 0.2), 0px 4px 5px 0px rgba(0, 0, 0, 0.14), 0px 1px 10px 0px rgba(0,0,0,.12);}
.btn2:visited {color: white;}
.btn2:focus {outline: none;}
.btn2:active {color: white;background-color: #ca7125;}
.btn2:disabled {cursor: default;background: #DDD;}
.btndel {
  border: none;
  color: white;
  padding: 0px 6px;
  height: 25px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 14px;
  margin: 2px 2px;
  cursor: pointer;
   border-radius: 4px;
   background-color: #e03423db;
   transition-duration: 0.1s;
  }
.btndel:hover {background-color: #b02f19;}
.btndel:visited {color: white;}
.btndel:active {color: white;background-color: #db331d;}

   html {
     font-family: Roboto, Arial, sans-serif;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; 
       font-family: Arial;
      text-align: center;
      font-weight: normal;
      color: #fafcfc;
    }
    p { font-size: 3.0rem; margin-top: 0;}
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
      font-weight: normal;
      color: #333333;
    }
a {color: #ffffff;}
table,th, td{font-family: arial;border-collapse: collapse;border: 1px solid #dddddd;}
th, td {padding: 10px; }
tr:nth-child(1) {background-color: #999999;}
th:nth-child(2) {text-align: left;}
td:nth-child(1) {column-width: 30px;text-align: center;}
td:nth-child(2) {column-width: 175px;text-align: left;white-space: nowrap;overflow: hidden;color: white;}
td:nth-child(3) {column-width: 100px;text-align: center;}
td:nth-child(4) {column-width: 30px; text-align: center;}
</style>
<title>File manager</title><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js'></script></head>
<body>
<form method="POST" action="http://192.168.0.160/filesystem#" enctype="multipart/form-data" id="upload_form">
<input type="file" name="update" id="file" onchange="sub(this)" style="display:none">
<label id="file-input" for="file">   Choose file...</label>
<input type="submit" id="updateBtn" class="btn2" disabled="true" value="Upload file">
<br>
<h4 style="text-align: center;">File system content:</h4>
<span id="filelist">%list%</span>
<br>
<div id="prg" style="display:none">Ready to upload</div>
<br><div id="prgbar" style="display:none"><div id="bar"></div></div></form>
<script>

function deletef(h) {
   var xhr = new XMLHttpRequest();
   var fnstring=String("/delete?file=")+h;
   xhr.open("GET", fnstring, true);
   console.log(fnstring);
    xhr.send();
    setTimeout("location.href = '../filesystem';", 3000);
}

function sub(obj){
  var a = obj.value;
  console.log(a);
  var fileName = a.replace(/^.*[\\\/]/, '');
  console.log(fileName);
  document.getElementById('file-input').innerHTML = fileName;
  document.getElementById('updateBtn').disabled = false;
  document.getElementById('prgbar').style.display = 'block';
  document.getElementById('prg').style.display = 'block';
};

$('form').submit(function(e){
document.getElementById('updateBtn').disabled = "disabled";  
e.preventDefault();
var form = $('#upload_form')[0];
var data = new FormData(form);

$.ajax({
url: '/doUpload',
type: 'POST',
data: data,
contentType: false,
processData:false,
xhr: function() {
var xhr = new window.XMLHttpRequest();
xhr.upload.addEventListener('progress', function(evt) {
if (evt.lengthComputable) {
var per = evt.loaded / evt.total;
$('#prg').html('Progress: ' + Math.round(per*100));
$('#bar').css('width', Math.round(per*350)+"px");
}
}, false);
return xhr;
},
success:function(d, s) {
console.log('success!'); 
setTimeout("location.href = '../filesystem';", 3000);
document.getElementById('prgbar').style.display = 'none';
document.getElementById('prg').style.display = 'none';
$('#prg').html('Ready to upload');
$('#bar').css('width:0px');
document.getElementById('file-input').innerHTML = 'Choose file...';
document.getElementById('updateBtn').disabled = true;
},
error: function (a, b, c) {alert("Upload error");setTimeout("location.href = '../filesystem';", 1000);}
});
});
</script></body></html>   
)rawliteral";
