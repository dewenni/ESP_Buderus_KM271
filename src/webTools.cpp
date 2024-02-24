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
#include <message.h>

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

int counter = 0;
/**
 * *******************************************************************
 * @brief   helper function to provide filelist
 * @param   var
 * @return  none
 * *******************************************************************/
String processor_update(const String& var) {
  msgLn(var.c_str());
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
    msgLn("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    msgLn("WebSocket client disconnected");
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
    msgLn("Update");
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
      msgLn("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
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
    msgLn("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    msgLn(" - not a directory");
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
 * @brief   function to upload files
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index)
  {
    content_len = request->contentLength();
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }

  if (opened == false) {
    opened = true;
    file = LittleFS.open(String("/") + filename, FILE_WRITE);
    if (!file)
    {
      msgLn("- failed to open file for writing");
      return;
    }
  }

  if (file.write(data, len) != len) {
    msgLn("- failed to write");
    return;
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Upload complete");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/filesystem");
    request->send(response);
    file.close();
    opened = false;
    msgLn("---------------");
    msgLn("Upload complete");
    listDir(LittleFS, "/", 0);
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
 * @brief   delete file on filesystem
 * @param   none
 * @return  none
 * *******************************************************************/
void deleteFile(fs::FS &fs, const String& path) {
  if (fs.remove(path)) {
    msgLn("- file deleted");
  } else {
    msgLn("- delete failed");
  }
  listDir(LittleFS, "/", 0);
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
  
  server->on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(401, "text/html", "<script>window.location.href = '../';</script>");
  });

  server->on("/ota", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", ota_html);
  });

  server->on("/filesystem", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", FS_html, processor_update);
  });

  server->on("/filelist", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", filelist.c_str());
  });

  server->on("/logger", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/html", [](uint8_t *buffer, size_t maxLen, size_t index) {
      static int line = 0;
      // read data from buffer to Chunk
      if (line < MAX_LOG_LINES) {
          int bytesToWrite = 0;
          int lineIndex;
          // reverse reading
          if (config.log.order==1){
            lineIndex = (logData.lastLine - line - 1) % MAX_LOG_LINES;
          }
          else {
            if (logData.buffer[logData.lastLine][0]=='\0'){
              // buffer is not full - start reading at element index 0
              lineIndex = line % MAX_LOG_LINES;
            }
            else {
              // buffer is full - start reading at element index "logData.lastLine"
              lineIndex = (logData.lastLine + line) % MAX_LOG_LINES;
            }
          }
          // check buffer overflow
          if (lineIndex < 0)
          {
            lineIndex += MAX_LOG_LINES;
          }
          if (lineIndex >= MAX_LOG_LINES)
          {
            lineIndex = 0;
          }
          // add header
          if (line == 0){
            bytesToWrite = sprintf((char *)buffer, "<!DOCTYPE html><html class=\"HTML\"><head><style>body{background:#444857;font-family:monospace,monospace;color:#ffffff;line-height:150%%;}</style></head><body>%s<br>", logData.buffer[lineIndex]);
            line++;
            return bytesToWrite;
          }
          // add footer
          else if (line == MAX_LOG_LINES-1){
            bytesToWrite = sprintf((char*)buffer, "%s</body></html>", logData.buffer[lineIndex]);
            line++;
            return bytesToWrite;
          }
          // add messages
          else {
            if (logData.buffer[lineIndex][0]!='\0'){
              bytesToWrite = sprintf((char*)buffer, "%s<br>", logData.buffer[lineIndex]);
              line++;
              return bytesToWrite;
            }
            else {
              bytesToWrite = sprintf((char*)buffer, "</body></html>");
              line = MAX_LOG_LINES;
              return bytesToWrite;
            }
          }       
      }
      line = 0;
      return 0;
		});
   	request->send(response); 
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
      msgLn("-inputMessage-");
      msg("File=");
      msgLn(inputMessage.c_str());
      msgLn(" has been deleted");
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