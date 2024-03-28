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
size_t content_len;


void handleWebClientData(AsyncWebServerRequest *request) {
  if (request->hasParam("elementId") && request->hasParam("value")) {
    String elementId = request->getParam("elementId")->value();
    String value = request->getParam("value")->value();
    webCallback(elementId.c_str(), value.c_str());
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Invalid Request");
  }
}

void sendWebUpdate(const char *message, const char *event) { events.send(message, event, millis()); }

const size_t BUFFER_SIZE = 512;

void setLanguage(const char *language) {
  char message[BUFFER_SIZE];
  snprintf(message, BUFFER_SIZE, "{\"language\":\"%s\"}", language);
  sendWebUpdate(message, "setLanguage");
}

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
    content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      updateWebText("p11_ota_update_error", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
    updateWebText("p11_ota_update_error", Update.errorString(), false);
    updateWebDialog("ota_update_failed_dialog", "open");
  } else {
    // Sende den Fortschritt über SSE
    int progress = (Update.progress() * 100) / content_len;
    Serial.printf("Progress: %d%%\n", progress);
    char message[32];
    if (otaUpdateTimer.cycleTrigger(200)) {
      snprintf(message, 32, "Progress: %d%%", progress);
      events.send(message, "ota-progress", millis());
    }
  }

  if (final) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)) {
      Update.printError(Serial);
      updateWebText("p11_ota_update_error", Update.errorString(), false);
      updateWebDialog("ota_update_failed_dialog", "open");
    } else {
      char message[32];
      snprintf(message, 32, "Progress: %d%%", 100);
      events.send(message, "ota-progress", millis());
      Serial.println("Update complete");
      Serial.flush();
      updateWebDialog("ota_update_done_dialog", "open");
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
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", gzip_login_html, gzip_login_html_size);
    response->addHeader("Content-Encoding", "gzip");
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
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", gzip_html, gzip_html_size);
    if (!isAuthenticated(request)) {
      request->redirect("/login");
    } else {
      response->addHeader("Content-Encoding", "gzip");
      request->send(response);
    }
  });

  server.on("/gzip_c.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", c_gzip_css, c_gzip_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip_m.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", m_gzip_css, m_gzip_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/js", gzip_js, gzip_js_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/favicon.svg", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "image/svg+xml", faviconSvg); });

  // config.json download
  server.on("/config-download", HTTP_GET,
            [](AsyncWebServerRequest *request) { request->send(LittleFS, "/config.json", "application/octet-stream"); });

  // config.json upload
  server.on(
      "/config-upload", HTTP_POST,
      [](AsyncWebServerRequest *request) {
        // Keine Aktion erforderlich, wenn alles erfolgreich war
        request->send(200, "text/plain", "upload done!");
      },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
        static File uploadFile;

        if (!index) { // Erster Aufruf für neuen Upload
          updateWebText("upload_status_txt", "uploading...", false);
          Serial.printf("UploadStart: %s\n", filename.c_str());
          uploadFile = LittleFS.open("/" + filename, "w");

          if (!uploadFile) {
            Serial.println("Fehler: Datei konnte nicht geöffnet werden.");
            updateWebText("upload_status_txt", "error on file close!", false);
            return; // Früher Rückkehr, um weitere Verarbeitung zu verhindern
          }
        }

        if (len) { // Wenn Daten vorhanden sind, schreibe sie
          uploadFile.write(data, len);
        }

        if (final) {        // Abschluss des Uploads
          if (uploadFile) { // Überprüfe, ob die Datei ordnungsgemäß geöffnet
            // wurde
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
    if (client->lastId()) {
      Serial.printf("Client reconnected!");
    }
    // Send a message to newly connected client
    client->send("ping", NULL, millis(), 5000);
    updateAllElements();
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

  // in simulation mode, load simdata and display simModeBar
  if (simulationTimer.delayOn(SIM_MODE && clientConnected && !simulationInit && !setupMode, 2000)) {
    simulationInit = true;
    showElementClass("simModeBar", true);
    startSimData();
  }

  webInitDone = true; // init done
}
