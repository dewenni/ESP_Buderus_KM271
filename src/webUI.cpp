#include <LittleFS.h>
#include <Update.h>
#include <basics.h>
#include <favicon.h>
#include <km271.h>
#include <message.h>
#include <oilmeter.h>
#include <sensor.h>
#include <simulation.h>
#include <stringHelper.h>
#include <webUI.h>
#include <webUIhelper.h>
#include <webUIupdates.h>

/* P R O T O T Y P E S ********************************************************/
void webCallback(const char *elementId, const char *value);

/* D E C L A R A T I O N S ****************************************************/
muTimer heartbeatTimer = muTimer();  // timer to refresh other values
muTimer simulationTimer = muTimer(); // timer to refresh other values
muTimer logReadTimer = muTimer();    // timer to refresh other values
muTimer otaUpdateTimer = muTimer();  // timer to refresh other values

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

s_webui_texts webText;
s_cfg_arrays cfgArrayTexts;

static const char *TAG = "WEB"; // LOG TAG
bool webInitDone = false;
bool simulationInit = false;
char otaMessage[128];
size_t content_len;
const size_t BUFFER_SIZE = 512;
char webCallbackElementID[32];
char webCallbackValue[256];
bool webCallbackAvailable = false;
unsigned long sendCnt = 0;
bool onLoadRequest = false;

void sendHeartbeat() {
  // if (ws.availableForWriteAll()) {
  ws.textAll("{\"type\":\"heartbeat\"}");
  // }
}

void updateWebLanguage(const char *language) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "setLanguage";
  jsonDoc["language"] = language;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebJSON(JsonDocument &jsonDoc) {
  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebText(const char *id, const char *text, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = text;
  jsonDoc["isInput"] = isInput ? true : false;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebTextInt(const char *id, long value, bool isInput) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["text"] = value;
  jsonDoc["isInput"] = isInput ? true : false;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebLog(const char *entry, const char *cmd) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "logger";
  jsonDoc["cmd"] = cmd;
  jsonDoc["entry"] = entry;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebTextFloat(const char *id, double value, bool isInput, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateText";
  jsonDoc["id"] = id;
  jsonDoc["isInput"] = isInput;

  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["text"] = textBuffer;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebState(const char *id, bool state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateState";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state ? true : false;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebValueStr(const char *id, const char *value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebValueInt(const char *id, long value) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;
  jsonDoc["value"] = value;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebValueFloat(const char *id, double value, int decimals) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateValue";
  jsonDoc["id"] = id;

  char textBuffer[16];
  snprintf(textBuffer, sizeof(textBuffer), "%.*f", decimals, value);
  jsonDoc["value"] = textBuffer;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void showElementClass(const char *className, bool show) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "showElementClass";
  jsonDoc["className"] = className;
  jsonDoc["show"] = show ? true : false;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebHideElement(const char *id, bool hide) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "hideElement";
  jsonDoc["id"] = id;
  jsonDoc["hide"] = hide ? true : false;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebDialog(const char *id, const char *state) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateDialog";
  jsonDoc["id"] = id;
  jsonDoc["state"] = state;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebSetIcon(const char *id, const char *icon) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateSetIcon";
  jsonDoc["id"] = id;
  jsonDoc["icon"] = icon;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebHref(const char *id, const char *href) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateHref";
  jsonDoc["id"] = id;
  jsonDoc["href"] = href;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

void updateWebBusy(const char *id, bool busy) {
  JsonDocument jsonDoc;
  jsonDoc["type"] = "updateBusy";
  jsonDoc["id"] = id;
  jsonDoc["busy"] = busy;

  const size_t len = measureJson(jsonDoc);
  AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
  assert(buffer); // optional: check buffer
  serializeJson(jsonDoc, buffer->get(), len);
  ws.textAll(buffer);
}

/**
 * *******************************************************************
 * @brief   function to process the firmware update
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    MY_LOGI(TAG, "OTA-Update started");
    storeData(); // store data before updating
    snprintf(otaMessage, sizeof(otaMessage), "Start OTA Update: %s", filename.c_str());
    km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
      km271Msg(KM_TYP_MESSAGE, "otaMessage", NULL);
      updateWebText("p11_ota_upd_err", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
      return request->send(400, "text/plain", "OTA could not begin");
    }
  }
  // update in progress
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
    km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
    updateWebText("p11_ota_upd_err", Update.errorString(), false);
    updateWebDialog("ota_update_failed_dialog", "open");
    return request->send(400, "text/plain", "OTA could not begin");
  } else {
    // calculate progress
    int progress = (Update.progress() * 100) / content_len;
    if (otaUpdateTimer.cycleTrigger(200)) {
      JsonDocument jsonDoc;
      jsonDoc["type"] = "otaProgress";
      jsonDoc["progress"] = progress;
      const size_t len = measureJson(jsonDoc);
      AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
      assert(buffer); // optional: check buffer
      serializeJson(jsonDoc, buffer->get(), len);
      ws.textAll(buffer);
    }
  }
  // update done
  if (final) {
    if (!Update.end(true)) {
      Update.printError(Serial);
      snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
      km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
      updateWebText("p11_ota_upd_err", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
      return request->send(400, "text/plain", "Could not end OTA");
    } else {
      JsonDocument jsonDoc;
      jsonDoc["type"] = "otaProgress";
      jsonDoc["progress"] = 100;
      const size_t len = measureJson(jsonDoc);
      AsyncWebSocketMessageBuffer *buffer = ws.makeBuffer(len);
      assert(buffer); // optional: check buffer
      serializeJson(jsonDoc, buffer->get(), len);
      ws.textAll(buffer);

      MY_LOGI(TAG, "OTA Update complete");
      Serial.flush();
      updateWebDialog("ota_update_done_dialog", "open");
      km271Msg(KM_TYP_MESSAGE, "OTA Update finished!", NULL);
      return request->send(200, "text/plain", "OTA Update finished!");
    }
  }
}

bool isAuthenticated(AsyncWebServerRequest *request) {
  if (!config.auth.enable) {
    return true; // if authentication is disabled send true
  }
  String cookieHeader = request->header("Cookie");
  if (cookieHeader.length() > 0) {
    String cookieName = "esp_buderus_km271_auth=";
    int cookiePos = cookieHeader.indexOf(cookieName);
    if (cookiePos != -1) {
      int valueStart = cookiePos + cookieName.length();
      int valueEnd = cookieHeader.indexOf(';', valueStart);
      if (valueEnd == -1) {
        valueEnd = cookieHeader.length();
      }
      String cookieValue = cookieHeader.substring(valueStart, valueEnd);
      if (cookieValue == "1") {
        return true;
      }
    }
  }
  return false;
}

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", gzip_login_html, gzip_login_html_size);
    request->send(response);
    response->addHeader("Content-Encoding", "gzip");
  });

  server.on("/max_ws", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", gzip_max_ws_html, gzip_max_ws_html_size);
    request->send(response);
    response->addHeader("Content-Encoding", "gzip");
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      if ((request->getParam("username", true)->value() == String(config.auth.user) &&
           request->getParam("password", true)->value() == String(config.auth.password)) ||
          (request->getParam("username", true)->value() == "esp-buderus" && request->getParam("password", true)->value() == "km271")) {
        // Erfolgreicher Login, Cookie setzen
        AsyncWebServerResponse *response = request->beginResponse(303); // 303 See Other
        response->addHeader("Location", "/");
        response->addHeader("Set-Cookie", "esp_buderus_km271_auth=1; Path=/; HttpOnly");
        request->send(response);
      } else {
        request->redirect("/login?error");
      }
    } else {
      request->redirect("/login?error");
    }
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(303); // 303 See Other
    response->addHeader("Location", "/login");
    // sets the expiration date of the cookie to a time in the past to delete it
    response->addHeader("Set-Cookie", "esp_buderus_km271_auth=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
    request->send(response);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", gzip_main_html, gzip_main_html_size);
    if (!isAuthenticated(request)) {
      request->redirect("/login");
    } else {
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    }
  });

  server.on("/main.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/css", gzip_css, gzip_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/js", gzip_js, gzip_js_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip_ntp", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", gzip_ntp_html, gzip_ntp_html_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "image/svg+xml", faviconSvg); });

  // config.json download
  server.on("/config-download", HTTP_GET,
            [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/octet-stream"); });

  // Route, um die config.json-Datei zu senden
  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/json"); });

  // config.json upload
  server.on(
      "/config-upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "upload done!"); },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;

        if (!index) { // firs call for upload
          updateWebText("upload_status_txt", "uploading...", false);
          MY_LOGI(TAG, "Upload Start: %s\n", filename.c_str());
          uploadFile = LittleFS.open("/" + filename, "w");

          if (!uploadFile) {
            MY_LOGE(TAG, "Error: file could not be opened");
            updateWebText("upload_status_txt", "error on file close!", false);
            return;
          }
        }

        if (len) { // if there are still data to send...
          uploadFile.write(data, len);
        }

        if (final) {
          if (uploadFile) {
            uploadFile.close();
            MY_LOGI(TAG, "UploadEnd: %s, %u B\n", filename.c_str(), index + len);
            updateWebText("upload_status_txt", "upload done!", false);
          } else {
            updateWebText("upload_status_txt", "error on file close!", false);
          }
        }
      });

  // Route für OTA-Updates
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, handleDoUpdate);

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    (void)len;
    if (type == WS_EVT_CONNECT) {
      if (ws.count() < 2) {
        MY_LOGI(TAG, "web-client connected");
        client->setCloseClientOnQueueFull(false);
        onLoadRequest = true;
      } else {
        MY_LOGI(TAG, "max web-client reached");
        client->text("{\"type\":\"redirect\",\"url\":\"/max_ws\"}");
        client->close(1008, "max client reached");
      }
    } else if (type == WS_EVT_DISCONNECT) {
      MY_LOGI(TAG, "web-client disconnected");
    } else if (type == WS_EVT_ERROR) {
      MY_LOGI(TAG, "web-client error");
    } else if (type == WS_EVT_PONG) {
      // Serial.println("ws pong");
    } else if (type == WS_EVT_DATA) {
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len) {
        if (info->opcode == WS_TEXT) {
          data[len] = 0;
          JsonDocument jsonDoc;
          DeserializationError error = deserializeJson(jsonDoc, data);
          if (error) {
            MY_LOGW(TAG, "Failed to parse WebSocket message as JSON");
            return;
          }
          const char *elementId = jsonDoc["elementId"];
          const char *value = jsonDoc["value"];
          snprintf(webCallbackElementID, sizeof(webCallbackElementID), "%s", elementId);
          snprintf(webCallbackValue, sizeof(webCallbackValue), "%s", value);
          webCallbackAvailable = true;
        }
      }
    }
  });

  server.addHandler(&ws);
  server.begin();

} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICyclic() {

  // ws.cleanupClients();

  if (heartbeatTimer.cycleTrigger(3000)) {
    sendHeartbeat();
  }

  // request for update alle elements
  if (onLoadRequest) {
    updateAllElements();
    onLoadRequest = false;
  }

  // handling of update webUI elements
  webUIupdates();

  // handling of callback infomation
  if (webCallbackAvailable) {
    webCallback(webCallbackElementID, webCallbackValue);
    webCallbackAvailable = false;
  }

  // in simulation mode, load simdata and display simModeBar
  if (simulationTimer.delayOn(config.sim.enable && !simulationInit && !setupMode, 2000)) {
    simulationInit = true;
    showElementClass("simModeBar", true);
    startSimData();
  }

  webInitDone = true; // init done
}
