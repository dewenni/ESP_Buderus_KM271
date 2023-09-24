/*------------------------------------------------------------------------------
| Based on: https://github.com/AR-D-R/ESP32-OTA-File-management
| Modified and extended:
| - changed from SPIFFS to LittleFS
| - changed layout and style
| - add OTA-Progress-Bar with websocket
------------------------------------------------------------------------------*/
#include <basics.h>
#include <webUI.h>
#include <Update.h>
#include <LittleFS.h>
#include <webTools.h>

/* D E C L A R A T I O N S ****************************************************/  
muTimer webTimer = muTimer();
String filelist = "";
const char* PARAM = "file";
size_t content_len;
File file;
bool opened = false;              
int otaProgress = 0;              // OTA-Fortschritt als Rückgabe an Webserver
AsyncWebSocket ws("/ws");         // Websocket
AsyncWebServer* server;           // Pointer to existing Webserver (ESPUI)

/**
 * *******************************************************************
 * @brief   helper function to provide filelist
 * @param   var
 * @return  none
 * *******************************************************************/
String processor_update(const String& var) {
  Serial.println(var);
  if (var == "list") {
    return filelist;
  }
  return String();
}

/**
 * *******************************************************************
 * @brief   WebSocketEvent to provide ota-progress information
 * @param   server, client, type, arg, data, len
 * @return  none
 * *******************************************************************/
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

/**
 * *******************************************************************
 * @brief   function to provide ota-progress information
 * @param   request
 * @return  none
 * *******************************************************************/
void onProgressRequest(AsyncWebServerRequest *request) {
  String progressJson = "{\"progress\":" + String(otaProgress) + "}";
  request->send(200, "application/json", progressJson);
}

/**
 * *******************************************************************
 * @brief   function to process the firmware update
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.println("Update");
    storeData(); // Store data before updating
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/ota");
    request->send(response);
    if (!Update.end(true)) {
      Update.printError(Serial);
    } else {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}

/**
 * *******************************************************************
 * @brief   function to upload files
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    content_len = request->contentLength();
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }

  if (opened == false) {
    opened = true;
    file = LittleFS.open(String("/") + filename, FILE_WRITE);
    if (!file) {
      Serial.println("- failed to open file for writing");
      return;
    }
  }

  if (file.write(data, len) != len) {
    Serial.println("- failed to write");
    return;
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/filesystem");
    request->send(response);
    file.close();
    opened = false;
    Serial.println("---------------");
    Serial.println("Upload complete");
  }
}

/**
 * *******************************************************************
 * @brief   calculate the progress of OTA-Update
 * @param   prg, sz
 * @return  none
 * *******************************************************************/
void printProgress(size_t prg, size_t sz) {
  Serial.printf("Progress: %d%%\n", (prg * 100) / content_len);
  otaProgress = (prg * 100) / content_len; // OTA-Fortschritt für Webserver
  Serial.printf("Progress: %d%%\n", otaProgress);
}


/**
 * *******************************************************************
 * @brief   Generate List of files and expand Table on Webserver
 * @param   none
 * @return  none
 * *******************************************************************/
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  // filelist = "";
  int i = 0;
  String partlist;
  // Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      i++;
      String st_after_symb = String(file.name()).substring(String(file.name()).indexOf("/") + 1);
      partlist +=  String("<tr><td>") + String(i) + String("</td><td>") + String("<a href='") + String(file.name()) + String("'>") + st_after_symb + String("</td><td>") + String(file.size()) + String("</td><td>") + String("<input type='button' class='btndel' onclick=\"deletef('") + String(file.name()) + String("')\" value='X'>") + String("</td></tr>");
      filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size [Byte]</th><th></th></tr>") + partlist + String(" </tbody></table>");
    }
    file = root.openNextFile();
  }
  filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size [Byte]</th><th></th></tr>") + partlist + String(" </tbody></table>");
}

/**
 * *******************************************************************
 * @brief   delete file on filesystem
 * @param   none
 * @return  none
 * *******************************************************************/
void deleteFile(fs::FS &fs, const String& path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

/**
 * *******************************************************************
 * @brief   function to download files
 * @param   request
 * @return  none
 * *******************************************************************/
void notFound(AsyncWebServerRequest *request) {
  if (request->url().startsWith("/")) {
    request->send(LittleFS, request->url(), String(), true);
  } else {
    request->send(404);
  }
}

/**
 * *******************************************************************
 * @brief   Setup for Webserver
 * @param   none
 * @return  none
 * *******************************************************************/
void webToolsSetup() {

  server = ESPUI.server;  // get pointer from existing ESPUI server
  listDir(LittleFS, "/", 0);

  server->on("/ota", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", ota_html);
  });

  server->on("/filesystem", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", FS_html, processor_update);
  });

  server->on("/filelist", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", filelist.c_str());
  });

  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", "Device will reboot in 2 seconds");
    delay(2000);
    ESP.restart();
  });

  server->on("/update", HTTP_POST,
    [](AsyncWebServerRequest * request) {},
    [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpdate(request, filename, index, data, len, final);
  });

  server->on("/doUpload", HTTP_POST, [](AsyncWebServerRequest * request) {
    opened = false;
  },
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpload(request, filename, index, data, len, final);
  });

  server->on("/delete", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM)) {
      inputMessage = "/" + request->getParam(PARAM)->value();
      inputParam = PARAM;

      deleteFile(LittleFS, inputMessage);
      //LittleFS.remove(inputMessage);
      Serial.println("-inputMessage-");
      Serial.print("File=");
      Serial.println(inputMessage);
      Serial.println(" has been deleted");
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(200, "text/plain", "OK");
  });

  server->on("/getOTAProgress", HTTP_GET, onProgressRequest);
  ws.onEvent(onWebSocketEvent);
  server->addHandler(&ws);
  server->onNotFound(notFound);
  Update.onProgress(printProgress);
}

/**
 * *******************************************************************
 * @brief   Cyclic operations for Webserver
 * @param   none
 * @return  none
 * *******************************************************************/
void webToolsCyclic() {

  if (webTimer.cycleTrigger(3000)){
    listDir(LittleFS, "/", 0);
  }

}