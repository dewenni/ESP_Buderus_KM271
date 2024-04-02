#include <LittleFS.h>
#include <Update.h>
#include <basics.h>
#include <favicon.h>
#include <km271.h>
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
muTimer connectionTimer = muTimer(); // timer to refresh other values
muTimer simulationTimer = muTimer(); // timer to refresh other values
muTimer logReadTimer = muTimer();    // timer to refresh other values
muTimer otaUpdateTimer = muTimer();  // timer to refresh other values

AsyncWebServer server(80);
AsyncEventSource events("/events");

s_webui_texts webText;
s_cfg_arrays cfgArrayTexts;

bool clientConnected = false;
bool webInitDone = false;
bool simulationInit = false;
char otaMessage[128];
size_t content_len;
const size_t BUFFER_SIZE = 512;
char webCallbackElementID[32];
char webCallbackValue[256];
bool webCallbackAvailable = false;

void sendWebUpdate(const char *message, const char *event) { events.send(message, event, millis()); }

void handleWebClientData(AsyncWebServerRequest *request) {
  if (request->hasParam("elementId") && request->hasParam("value")) {
    snprintf(webCallbackElementID, sizeof(webCallbackElementID), "%s", request->getParam("elementId")->value().c_str());
    snprintf(webCallbackValue, sizeof(webCallbackValue), "%s", request->getParam("value")->value().c_str());
    webCallbackAvailable = true;
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Invalid Request");
  }
}

void setLanguage(const char *language) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"language\":\"%s\"}", language);
  sendWebUpdate(message, "setLanguage");
}

void updateWebJSON(const char *JSON) { events.send(JSON, "updateJSON", millis()); }

void updateWebText(const char *elementID, const char *text, bool isInput) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%s\",\"isInput\":%s}", elementID, text, isInput ? "true" : "false");
  sendWebUpdate(message, "updateText");
}

void updateWebTextInt(const char *elementID, long value, bool isInput) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%ld\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");
  sendWebUpdate(message, "updateText");
}

void updateWebTextFloat(const char *elementID, double value, bool isInput, int decimals) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%.1f\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");
  if (decimals == 0)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%.0f\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");
  else if (decimals == 2)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%.2f\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");
  else if (decimals == 3)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%.3f\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");
  else
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"text\":\"%.1f\",\"isInput\":%s}", elementID, value, isInput ? "true" : "false");

  sendWebUpdate(message, "updateText");
}

void updateWebState(const char *elementID, bool state) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"state\":%s}", elementID, state ? "true" : "false");
  sendWebUpdate(message, "updateState");
}

void updateWebValueStr(const char *elementID, const char *value) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%s\"}", elementID, value);
  sendWebUpdate(message, "updateValue");
}

void updateWebValueInt(const char *elementID, long value) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%ld\"}", elementID, value);
  sendWebUpdate(message, "updateValue");
}

void updateWebValueFloat(const char *elementID, double value, int decimals) {
  char message[BUFFER_SIZE];
  if (decimals == 0)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%.0f\"}", elementID, value);
  else if (decimals == 2)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%.2f\"}", elementID, value);
  else if (decimals == 3)
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%.3f\"}", elementID, value);
  else
    snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"value\":\"%.1f\"}", elementID, value);

  sendWebUpdate(message, "updateValue");
}

void showElementClass(const char *className, bool show) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"className\":\"%s\",\"show\":%s}", className, show ? "true" : "false");
  sendWebUpdate(message, "showElementClass");
}

void hideElementId(const char *elementID, bool hide) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"hide\":%s}", elementID, hide ? "true" : "false");
  sendWebUpdate(message, "hideElement");
}

void updateWebDialog(const char *elementID, const char *state) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"state\":\"%s\"}", elementID, state);
  sendWebUpdate(message, "updateDialog");
}

void updateWebSetIcon(const char *elementID, const char *icon) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"elementID\":\"%s\",\"icon\":\"%s\"}", elementID, icon);
  sendWebUpdate(message, "updateSetIcon");
}

/**
 * *******************************************************************
 * @brief   function to process the firmware update
 * @param   request, filename, index, data, len, final
 * @return  none
 * *******************************************************************/
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    Serial.println("Update");
    storeData(); // store data before updating
    snprintf(otaMessage, sizeof(otaMessage), "Start OTA Update: %s", filename.c_str());
    km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
      km271Msg(KM_TYP_MESSAGE, "otaMessage", NULL);
      updateWebText("p11_ota_update_error", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
    }
  }
  // update in progress
  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
    km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
    updateWebText("p11_ota_update_error", Update.errorString(), false);
    updateWebDialog("ota_update_failed_dialog", "open");
  } else {
    // calculate progress
    int progress = (Update.progress() * 100) / content_len;
    Serial.printf("Progress: %d%%\n", progress);
    char message[32];
    if (otaUpdateTimer.cycleTrigger(200)) {
      snprintf(message, 32, "Progress: %d%%", progress);
      events.send(message, "ota-progress", millis());
    }
  }
  // update done
  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)) {
      Update.printError(Serial);
      snprintf(otaMessage, sizeof(otaMessage), "OTA Update failed: %s", Update.errorString());
      km271Msg(KM_TYP_MESSAGE, otaMessage, NULL);
      updateWebText("p11_ota_update_error", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
    } else {
      char message[32];
      snprintf(message, 32, "Progress: %d%%", 100);
      events.send(message, "ota-progress", millis());
      Serial.println("Update complete");
      Serial.flush();
      updateWebDialog("ota_update_done_dialog", "open");
      km271Msg(KM_TYP_MESSAGE, "OTA Update finished!", NULL);
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
 * @brief   on connect or refresh - force update of all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void onLoadRequest() { updateAllElements(); }

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none
 * @return  none
 * *******************************************************************/
void webUISetup() {

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", login_html, login_html_size);
    response->addHeader("Content-Encoding", "br");
    request->send(response);
  });

  server.on("/login", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("username", true) && request->hasParam("password", true)) {
      if (request->getParam("username", true)->value() == String(config.auth.user) &&
          request->getParam("password", true)->value() == String(config.auth.password)) {
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
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", main_html, main_html_size);
    if (!isAuthenticated(request)) {
      request->redirect("/login");
    } else {
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    }
  });

  server.on("/gzip_c.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", c_css, c_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip_m.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", m_css, m_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip_m.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/js", m_js, m_js_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "image/svg+xml", faviconSvg); });

  // config.json download
  server.on("/config-download", HTTP_GET,
            [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/octet-stream"); });

  // config.json upload
  server.on(
      "/config-upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200, "text/plain", "upload done!"); },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;

        if (!index) { // firs call for upload
          updateWebText("upload_status_txt", "uploading...", false);
          Serial.printf("UploadStart: %s\n", filename.c_str());
          uploadFile = LittleFS.open("/" + filename, "w");

          if (!uploadFile) {
            Serial.println("Fehler: Datei konnte nicht geöffnet werden.");
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
            Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
            updateWebText("upload_status_txt", "upload done!", false);
          } else {
            updateWebText("upload_status_txt", "error on file close!", false);
          }
        }
      });

  // Route für OTA-Updates
  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, handleDoUpdate);

  // message from webClient to server
  server.on("/sendData", HTTP_GET, handleWebClientData);

  // SSE Endpoint
  events.onConnect([](AsyncEventSourceClient *client) {
    // check if it´s a new client or reconnect
    if (client->lastId()) {
      Serial.printf("Client reconnected with lastId %u\n", client->lastId());
    } else {
      Serial.println("New Client connected");
    }

    // send ping as welcome and force web elements update
    client->send("ping", NULL, millis(), 5000);
    onLoadRequest();
    clientConnected = true;
  });

  server.addHandler(&events);
  server.begin();

} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none
 * @return  none
 * *******************************************************************/
void webUICylic() {

  // heartbeat timer for webclient
  if (connectionTimer.cycleTrigger(2000)) {
    events.send("ping", "ping", millis());
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
