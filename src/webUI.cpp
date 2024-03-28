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

AsyncWebServer server(80);
AsyncEventSource events("/events");

bool clientConnected = false;
bool webInitDone = false;
bool updateSettingsElementsDone = false;
bool oilmeterInit = false;
bool simulationInit = false;
long oilcounter, oilcounterOld;
double oilcounterVirtOld = 0.0;
int UpdateCnt50ms = 0;
int UpdateCnt1s = 0;
size_t content_len;

s_km271_status *pkmStatus;
s_km271_status kmStatusCpy;
s_km271_config_str *pkmConfigStr;
s_km271_config_num *pkmConfigNum;
s_km271_alarm_str *pkmAlarmStr;

s_webui_texts webText;
s_cfg_arrays cfgArrayTexts;

char tmpMessage[300] = {'\0'};
char pushoverMessage[300] = {'\0'};
long oilmeterSetValue;
tm dti;
uint8_t webElementUpdateCnt = 0;
unsigned int KmCfgHash[kmConfig_Hash_SIZE] = {0};
unsigned int KmAlarmHash[4] = {0};

bool authenticated = false;

/* P R O T O T Y P E S ********************************************************/
void webCallback(const char *elementId, const char *value);
void updateAllElements();
void updateSettingsElements();

/* D E C L A R A T I O N S ****************************************************/
muTimer refreshTimer1 = muTimer();   // timer to refresh other values
muTimer refreshTimer2 = muTimer();   // timer to refresh other values
muTimer refreshTimer3 = muTimer();   // timer to refresh other values
muTimer connectionTimer = muTimer(); // timer to refresh other values
muTimer simulationTimer = muTimer(); // timer to refresh other values
muTimer logReadTimer = muTimer();    // timer to refresh other values
muTimer otaUpdateTimer = muTimer();  // timer to refresh other values

/**
 * *******************************************************************
 * @brief   format build date/time information
 * @param   date input string
 * @return  hash value
 * *******************************************************************/
void getBuildDateTime(char *formatted_date) {
  // Monatsnamen für die Umwandlung in Zahlen
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char month_text[4] = {0};
  int day, year;
  // Extrahiere den Monat, Tag und Jahr aus dem __DATE__ String
  sscanf(__DATE__, "%s %d %d", month_text, &day, &year);

  // Finde den Monat im Array und konvertiere ihn in eine Zahl
  int month = 0;
  for (int i = 0; i < 12; i++) {
    if (strcmp(month_text, months[i]) == 0) {
      month = i + 1;
      break;
    }
  }

  // Format das Datum als "DD-MM-YYYY"
  sprintf(formatted_date, "%02d.%02d.%d - %s", day, month, year, __TIME__);
}

/**
 * *******************************************************************
 * @brief   create ON/OFF String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char *onOffString(uint8_t value) {
  static char ret_str[64];
  if (value != 0)
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ON[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OFF[config.lang]);

  return ret_str;
}
/**
 * *******************************************************************
 * @brief   create ERROR/OK String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char *errOkString(uint8_t value) {
  static char ret_str[64];
  if (value != 0)
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ERROR[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OK[config.lang]);

  return ret_str;
}

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
  if (!config.auth.enable){
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

  pkmConfigStr = km271GetConfigStringsAdr();
  pkmConfigNum = km271GetConfigValueAdr();
  pkmStatus = km271GetStatusValueAdr();
  pkmAlarmStr = km271GetAlarmMsgAdr();

} // END SETUP

/**
 * *******************************************************************
 * @brief   callback function for web elements
 * @param   elementID
 * @param   value
 * @return  none
 * *******************************************************************/
void webCallback(const char *elementId, const char *value) {
  msg("Received - Element ID: ");
  msg(elementId);
  msg(", Value: ");
  msgLn(value);

  // HC1-OPMODE
  if (strcmp(elementId, "p02_hc1_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 0);
  } else if (strcmp(elementId, "p02_hc1_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 1);
  } else if (strcmp(elementId, "p02_hc1_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, 2);
  }

  // HC2-OPMODE
  if (strcmp(elementId, "p02_hc2_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 0);
  } else if (strcmp(elementId, "p02_hc2_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 1);
  } else if (strcmp(elementId, "p02_hc2_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, 2);
  }

  // WW-OPMODE
  if (strcmp(elementId, "p02_ww_opmode_night") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 0);
  } else if (strcmp(elementId, "p02_ww_opmode_day") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 1);
  } else if (strcmp(elementId, "p02_ww_opmode_auto") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, 2);
  }

  // HC1-Program
  if (strcmp(elementId, "p02_hc1_prg") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, atoi(value));
  }

  // HC2-Program
  if (strcmp(elementId, "p02_hc2_prg") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, atoi(value));
  }

  // HC1-Reduction-Mode
  if (strcmp(elementId, "p02_hc1_reduct_mode") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_REDUCTION_MODE, atoi(value));
  }

  // HC2-Reduction-Mode
  if (strcmp(elementId, "p02_hc2_reduct_mode") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_REDUCTION_MODE, atoi(value));
  }

  // HC1-Frost Threshold
  if (strcmp(elementId, "p02_hc1_frost_protection_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, atoi(value));
  }

  // HC1-Summer Threshold
  if (strcmp(elementId, "p02_hc1_summer_mode_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SUMMER, atoi(value));
  }

  // HC2-Frost Threshold
  if (strcmp(elementId, "p02_hc2_frost_protection_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, atoi(value));
  }

  // HC2-Summer Threshold
  if (strcmp(elementId, "p02_hc2_summer_mode_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SUMMER, atoi(value));
  }

  // HC1-DesignTemp
  if (strcmp(elementId, "p02_hc1_interpretation") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, atoi(value));
  }

  // HC2-DesignTemp
  if (strcmp(elementId, "p02_hc2_interpretation") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, atoi(value));
  }

  // HC1-SwitchOffTemp
  if (strcmp(elementId, "p02_hc1_switch_off_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, atoi(value));
  }

  // HC2-SwitchOffTemp
  if (strcmp(elementId, "p02_hc2_switch_off_threshold") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, atoi(value));
  }

  // HC1-Holiday days
  if (strcmp(elementId, "p02_hc1_holiday_days") == 0) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, atoi(value));
  }

  // HC2-Holiday days
  if (strcmp(elementId, "p02_hc2_holiday_days") == 0) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, atoi(value));
  }

  // WW-Temp
  if (strcmp(elementId, "p02_ww_temp") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, atoi(value));
  }

  // WW-Pump Cycles
  if (strcmp(elementId, "p02_ww_circulation") == 0) {
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, atoi(value));
  }

  // store input of Oilcounter value
  if (strcmp(elementId, "p02_oilmeter_set") == 0) {
    oilmeterSetValue = strtol(value, NULL, 10);
  }

  // Set new Oilcounter value
  if (strcmp(elementId, "p02_oilmeter_btn") == 0) {
    cmdSetOilmeter(oilmeterSetValue);
  }

  // WiFi
  if (strcmp(elementId, "p12_wifi_hostname") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.hostname), value);
  }
  if (strcmp(elementId, "p12_wifi_ssid") == 0) {
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), value);
  }
  if (strcmp(elementId, "p12_wifi_password") == 0) {
    snprintf(config.wifi.password, sizeof(config.wifi.password), value);
  }

  // IP-Settings
  if (strcmp(elementId, "p12_ip_enable") == 0) {
    config.ip.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_ip_adr") == 0) {
    snprintf(config.ip.ipaddress, sizeof(config.ip.ipaddress), value);
  }
  if (strcmp(elementId, "p12_ip_subnet") == 0) {
    snprintf(config.ip.subnet, sizeof(config.ip.subnet), value);
  }
  if (strcmp(elementId, "p12_ip_gateway") == 0) {
    snprintf(config.ip.gateway, sizeof(config.ip.gateway), value);
  }
  if (strcmp(elementId, "p12_ip_dns") == 0) {
    snprintf(config.ip.dns, sizeof(config.ip.dns), value);
  }

  // Authentication
  if (strcmp(elementId, "p12_access_enable") == 0) {
    config.auth.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_access_user") == 0) {
    snprintf(config.auth.user, sizeof(config.auth.user), value);
  }
  if (strcmp(elementId, "p12_access_password") == 0) {
    snprintf(config.auth.password, sizeof(config.auth.password), value);
  }

  // NTP-Server
  if (strcmp(elementId, "p12_ntp_enable") == 0) {
    config.ip.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_ntp_server") == 0) {
    snprintf(config.ntp.server, sizeof(config.ntp.server), value);
  }
  if (strcmp(elementId, "p12_ntp_tz") == 0) {
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), value);
  }

  // set manual date for Logamatic
  if (strcmp(elementId, "p12_dti_date") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    /* ---------------- INFO ---------------------------------
    dti.tm_year + 1900  // years since 1900
    dti.tm_mon + 1      // January = 0 (!)
    dti.tm_mday         // day of month
    dti.tm_hour         // hours since midnight  0-23
    dti.tm_min          // minutes after the hour  0-59
    dti.tm_sec          // seconds after the minute  0-61*
    dti.tm_wday         // days since Sunday 0-6
    dti.tm_isdst        // Daylight Saving Time flag
    --------------------------------------------------------- */
    // get date
    strncpy(tmp1, value, sizeof(tmp1));
    // extract year
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 4);
    dti.tm_year = atoi(tmp2) - 1900;
    // extract month
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 5, 2);
    dti.tm_mon = atoi(tmp2) - 1;
    // extract day
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 8, 2);
    dti.tm_mday = atoi(tmp2);
    // calculate day of week
    int d = dti.tm_mday;        // Day     1-31
    int m = dti.tm_mon + 1;     // Month   1-12
    int y = dti.tm_year + 1900; // Year    2022
    dti.tm_wday = (d += m < 3 ? y-- : y - 2,
                   23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7; // calculate day of week
  }
  // get time
  if (strcmp(elementId, "p12_dti_time") == 0) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    strncpy(tmp1, value, sizeof(tmp1));
    // extract hour
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 2);
    dti.tm_hour = atoi(tmp2);
    // extract minutes
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1 + 3, 2);
    dti.tm_min = atoi(tmp2);
  }
  // set date and time on Logamatic
  if (strcmp(elementId, "p12_dti_btn") == 0) {
    km271SetDateTimeDTI(dti);
    // TODO: check
  }

  // MQTT
  if (strcmp(elementId, "p12_mqtt_enable") == 0) {
    config.mqtt.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_mqtt_cfg_ret") == 0) {
    config.mqtt.config_retain = stringToBool(value);
  }
  if (strcmp(elementId, "p12_mqtt_server") == 0) {
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), value);
  }
  if (strcmp(elementId, "p12_mqtt_port") == 0) {
    config.mqtt.port = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_mqtt_topic") == 0) {
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), value);
  }
  if (strcmp(elementId, "p12_mqtt_user") == 0) {
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), value);
  }
  if (strcmp(elementId, "p12_mqtt_password") == 0) {
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), value);
  }
  if (strcmp(elementId, "p12_mqtt_language") == 0) {
    config.mqtt.lang = strtoul(value, NULL, 10);
  }

  // Pushover
  if (strcmp(elementId, "p12_pushover_enable") == 0) {
    config.pushover.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_pushover_api_token") == 0) {
    snprintf(config.pushover.token, sizeof(config.pushover.token), value);
  }
  if (strcmp(elementId, "p12_pushover_user_key") == 0) {
    snprintf(config.pushover.user_key, sizeof(config.pushover.user_key), value);
  }
  if (strcmp(elementId, "p12_pushover_filter") == 0) {
    config.pushover.filter = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_pushover_test_msg") == 0) {
    snprintf(pushoverMessage, sizeof(pushoverMessage), value);
  }
  if (strcmp(elementId, "p12_pushover_test_btn") == 0) {
    addPushoverMsg(pushoverMessage);
  }

  // Logamatic
  if (strcmp(elementId, "p12_hc1_enable") == 0) {
    config.km271.use_hc1 = stringToBool(value);
  }
  if (strcmp(elementId, "p12_hc2_enable") == 0) {
    config.km271.use_hc2 = stringToBool(value);
  }
  if (strcmp(elementId, "p12_hw_enable") == 0) {
    config.km271.use_ww = stringToBool(value);
  }
  if (strcmp(elementId, "p12_alarm_enable") == 0) {
    config.km271.use_alarmMsg = stringToBool(value);
  }

  // Hardware
  if (strcmp(elementId, "p12_gpio_km271_rx") == 0) {
    config.gpio.km271_RX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_km271_tx") == 0) {
    config.gpio.km271_TX = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_heartbeat") == 0) {
    config.gpio.led_heartbeat = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_logmode") == 0) {
    config.gpio.led_logmode = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_led_wifi") == 0) {
    config.gpio.led_oilcounter = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_gpio_trig_oilcounter") == 0) {
    config.gpio.trigger_oilcounter = strtoul(value, NULL, 10);
  }

  // Oil-Meter
  if (strcmp(elementId, "p12_oil_hardware_enable") == 0) {
    config.oilmeter.use_hardware_meter = stringToBool(value);
  }
  if (strcmp(elementId, "p12_oil_virtual_enable") == 0) {
    config.oilmeter.use_virtual_meter = stringToBool(value);
  }
  if (strcmp(elementId, "p12_oil_par1_kg_h") == 0) {
    config.oilmeter.consumption_kg_h = strtof(value, NULL);
  }
  if (strcmp(elementId, "p12_oil_par2_kg_l") == 0) {
    config.oilmeter.oil_density_kg_l = strtof(value, NULL);
  }

  // Optional Sensor
  if (strcmp(elementId, "p12_sens1_enable") == 0) {
    config.sensor.ch1_enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_sens1_name") == 0) {
    snprintf(config.sensor.ch1_name, sizeof(config.sensor.ch1_name), value);
  }
  if (strcmp(elementId, "p12_sens1_description") == 0) {
    snprintf(config.sensor.ch1_description, sizeof(config.sensor.ch1_description), value);
  }
  if (strcmp(elementId, "p12_sens1_gpio") == 0) {
    config.sensor.ch1_gpio = strtoul(value, NULL, 10);
  }
  if (strcmp(elementId, "p12_sens2_enable") == 0) {
    config.sensor.ch2_enable = stringToBool(value);
  }
  if (strcmp(elementId, "p12_sens2_name") == 0) {
    snprintf(config.sensor.ch2_name, sizeof(config.sensor.ch2_name), value);
  }
  if (strcmp(elementId, "p12_sens2_description") == 0) {
    snprintf(config.sensor.ch2_description, sizeof(config.sensor.ch2_description), value);
  }
  if (strcmp(elementId, "p12_sens2_gpio") == 0) {
    config.sensor.ch2_gpio = strtoul(value, NULL, 10);
  }

  // Language
  if (strcmp(elementId, "p12_language") == 0) {
    config.lang = strtoul(value, NULL, 10);
  }

  // Buttons
  if (strcmp(elementId, "p12_btn_restart") == 0) {
    storeData();
    ESP.restart();
  }

  // Logger
  if (strcmp(elementId, "p10_log_enable") == 0) {
    config.log.enable = stringToBool(value);
  }
  if (strcmp(elementId, "p10_log_mode") == 0) {
    config.log.filter = strtoul(value, NULL, 10);
    clearLogBuffer();
    sendWebUpdate("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_order") == 0) {
    config.log.order = strtoul(value, NULL, 10);
    sendWebUpdate("", "clr_log"); // clear log
    webReadLogBuffer();
  }
  if (strcmp(elementId, "p10_log_clr_btn") == 0) {
    clearLogBuffer();
    sendWebUpdate("", "clr_log"); // clear log
  }
  if (strcmp(elementId, "p10_log_refresh_btn") == 0) {
    webReadLogBuffer();
  }

  // OTA-Confirm
  if (strcmp(elementId, "p11_ota_confirm_btn") == 0) {
    updateWebDialog("ota_update_done_dialog", "close");
    storeData();
    ESP.restart();
  }
}

/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAllElements() {

  // set compare structures to a "random" value to force updates
  memset((void *)&kmStatusCpy, 111, sizeof(s_km271_status));

  // reset hash values to force updates
  memset((void *)KmCfgHash, 0, sizeof(KmCfgHash));
  memset((void *)KmAlarmHash, 0, sizeof(KmAlarmHash));

  updateSettingsElementsDone = false; // update all settings values
  oilmeterInit = false;               // update oil-meter values

  if (setupMode) {
    showElementClass("setupModeBar", true);
  }
}

/**
 * *******************************************************************
 * @brief   update Logger output
 * @param   none
 * @return  none
 * *******************************************************************/
int logLine, logIdx = 0;
bool logReadEnable = false;
void webReadLogBuffer() {
  logReadEnable = true;
  logLine = 0;
  logIdx = 0;
}
void webReadLogBufferCyclic() {
  if (!logReadEnable) {
    return;
  }
  if (logLine == 0 && logData.lastLine == 0) {
    // log empty
    logReadEnable = false;
    return;
  }
  if (config.log.order == 1) {
    logIdx = (logData.lastLine - logLine - 1) % MAX_LOG_LINES;
  } else {
    if (logData.buffer[logData.lastLine][0] == '\0') {
      // buffer is not full - start reading at element index 0
      logIdx = logLine % MAX_LOG_LINES;
    } else {
      // buffer is full - start reading at element index "logData.lastLine"
      logIdx = (logData.lastLine + logLine) % MAX_LOG_LINES;
    }
  }
  if (logIdx < 0) {
    logIdx += MAX_LOG_LINES;
  }
  if (logIdx >= MAX_LOG_LINES) {
    logIdx = 0;
  }
  if (logLine == 0) {
    sendWebUpdate("", "clr_log");                     // clear log
    sendWebUpdate(logData.buffer[logIdx], "add_log"); // add first log element
    logLine++;
  } else if (logLine == MAX_LOG_LINES - 1) {
    // end
    return;
  } else {
    if (logData.buffer[logIdx][0] != '\0') {
      sendWebUpdate(logData.buffer[logIdx], "add_log"); // add next log element
      logLine++;
    } else {
      // no more entries
      logReadEnable = false;
      return;
    }
  }
}

/**
 * *******************************************************************
 * @brief   update Sensor informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSensorElements() {

  if (config.sensor.ch1_enable) {
    updateWebText("p01_sens1_name", config.sensor.ch1_name, false);
    updateWebTextInt("p01_sens1_value", sensor.ch1_temp, false);
    updateWebText("p01_sens1_description", config.sensor.ch1_description, false);
  }

  if (config.sensor.ch2_enable) {
    updateWebText("p01_sens2_name", config.sensor.ch2_name, false);
    updateWebTextInt("p01_sens2_value", sensor.ch2_temp, false);
    updateWebText("p01_sens2_description", config.sensor.ch2_description, false);
  }
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateOilmeterElements() {

  if (config.oilmeter.use_hardware_meter) {
    oilcounter = getOilmeter();
    if (!oilmeterInit || oilcounter != oilcounterOld) {
      oilcounterOld = oilcounter;

      // Oilmeter value in controlTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f", float(oilcounter) / 100);
      updateWebText("p02_oilmeter_act", tmpMessage, false);
      snprintf(tmpMessage, sizeof(tmpMessage), "%lu", (oilcounter));
      updateWebText("p02_oilmeter_set", tmpMessage, true);

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(oilcounter) / 100);
      updateWebText("p01_hw_oilmeter", tmpMessage, false);
    }
  } else if (config.oilmeter.use_virtual_meter) {
    if (!oilmeterInit || kmStatusCpy.BurnerCalcOilConsumption != oilcounterVirtOld) {
      oilcounterVirtOld = kmStatusCpy.BurnerCalcOilConsumption;

      // Oilmeter value in dashboardTab
      snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L", float(oilcounter) / 100);
      updateWebText("p01_v_oilmeter", tmpMessage, false);
    }
  }
  oilmeterInit = true;
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSettingsElements() {

  switch (webElementUpdateCnt) {
  case 0:
    updateWebText("p12_wifi_hostname", config.wifi.hostname, true);
    break;
  case 1:
    updateWebText("p12_wifi_ssid", config.wifi.ssid, true);
    break;
  case 2:
    updateWebText("p12_wifi_password", config.wifi.password, true);
    break;
  case 3:
    updateWebState("p12_ip_enable", config.ip.enable);
    break;
  case 4:
    updateWebText("p12_ip_adr", config.ip.ipaddress, true);
    break;
  case 5:
    updateWebText("p12_ip_subnet", config.ip.subnet, true);
    break;
  case 6:
    updateWebText("p12_ip_gateway", config.ip.gateway, true);
    break;
  case 7:
    updateWebText("p12_ip_dns", config.ip.dns, true);
    break;
  case 8:
    updateWebState("p12_access_enable", config.auth.enable);
    break;
  case 9:
    updateWebText("p12_access_user", config.auth.user, true);
    break;
  case 10:
    updateWebText("p12_access_password", config.auth.password, true);
    break;
  case 11:
    updateWebState("p12_mqtt_enable", config.mqtt.enable);
    break;
  case 12:
    updateWebText("p12_mqtt_server", config.mqtt.server, true);
    break;
  case 13:
    updateWebValueInt("p12_mqtt_port", config.mqtt.port);
    break;
  case 14:
    updateWebText("p12_mqtt_user", config.mqtt.user, true);
    break;
  case 15:
    updateWebText("p12_mqtt_password", config.mqtt.password, true);
    break;
  case 16:
    updateWebText("p12_mqtt_topic", config.mqtt.topic, true);
    break;
  case 17:
    updateWebValueInt("p12_mqtt_language", config.mqtt.lang);
    break;
  case 18:
    updateWebState("p12_pushover_enable", config.pushover.enable);
    break;
  case 19:
    updateWebText("p12_pushover_api_token", config.pushover.token, true);
    break;
  case 20:
    updateWebText("p12_pushover_user_key", config.pushover.user_key, true);
    break;
  case 21:
    updateWebValueInt("p12_pushover_filter", config.pushover.filter);
    break;
  case 22:
    updateWebState("p12_hc1_enable", config.km271.use_hc1);
    break;
  case 23:
    updateWebState("p12_hc2_enable", config.km271.use_hc2);
    break;
  case 24:
    updateWebState("p12_hw_enable", config.km271.use_ww);
    break;
  case 25:
    updateWebState("p12_alarm_enable", config.km271.use_alarmMsg);
    break;
  case 26:
    updateWebValueInt("p12_gpio_km271_rx", config.gpio.km271_RX);
    break;
  case 27:
    updateWebValueInt("p12_gpio_km271_tx", config.gpio.km271_TX);
    break;
  case 28:
    updateWebValueInt("p12_gpio_led_heartbeat", config.gpio.led_heartbeat);
    break;
  case 29:
    updateWebValueInt("p12_gpio_led_logmode", config.gpio.led_logmode);
    break;
  case 30:
    updateWebValueInt("p12_gpio_led_wifi", config.gpio.led_wifi);
    break;
  case 31:
    updateWebValueInt("p12_gpio_led_oilcounter", config.gpio.led_oilcounter);
    break;
  case 32:
    updateWebValueInt("p12_gpio_trig_oilcounter", config.gpio.trigger_oilcounter);
    break;
  case 33:
    updateWebState("p12_oil_hardware_enable", config.oilmeter.use_hardware_meter);
    break;
  case 34:
    updateWebState("p12_oil_virtual_enable", config.oilmeter.use_virtual_meter);
    break;
  case 35:
    updateWebValueFloat("p12_oil_par1_kg_h", config.oilmeter.consumption_kg_h, 3);
    break;
  case 36:
    updateWebValueFloat("p12_oil_par2_kg_l", config.oilmeter.oil_density_kg_l, 3);
    break;
  case 37:
    updateWebState("p12_sens1_enable", config.sensor.ch1_enable);
    break;
  case 38:
    updateWebText("p12_sens1_name", config.sensor.ch1_name, true);
    break;
  case 39:
    updateWebText("p12_sens1_description", config.sensor.ch1_description, true);
    break;
  case 40:
    updateWebValueInt("p12_sens1_gpio", config.sensor.ch1_gpio);
    break;
  case 41:
    updateWebState("p12_sens2_enable", config.sensor.ch2_enable);
    break;
  case 42:
    updateWebText("p12_sens2_name", config.sensor.ch2_name, true);
    break;
  case 43:
    updateWebText("p12_sens2_description", config.sensor.ch2_description, true);
    break;
  case 44:
    updateWebValueInt("p12_sens2_gpio", config.sensor.ch2_gpio);
    break;
  case 45:
    updateWebValueInt("p12_language", config.lang);
    break;
  case 46:
    updateWebState("p10_log_enable", config.log.enable);
    break;
  case 47:
    updateWebValueInt("p10_log_mode", config.log.filter);
    break;
  case 48:
    updateWebValueInt("p10_log_order", config.log.order);
    break;
  case 49:
    updateWebState("p12_ntp_enable", config.ntp.enable);
    break;
  case 50:
    updateWebText("p12_ntp_server", config.ntp.server, true);
    break;
  case 51:
    updateWebText("p12_ntp_tz", config.ntp.tz, true);
    break;
  case 52:
    updateSettingsElementsDone = true;
    break;
  default:
    webElementUpdateCnt = -1;
    break;
  }
  webElementUpdateCnt = (webElementUpdateCnt + 1) % 53;
}

/**
 * *******************************************************************
 * @brief   update System informations
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfoElements() {

  // WiFi
  updateWebText("p09_wifi_ip", wifi.ipAddress, false);
  updateWebTextInt("p09_wifi_signal", wifi.signal, false);
  updateWebTextInt("p09_wifi_rssi", wifi.rssi, false);

  // Version informations
  updateWebText("p00_version", VERSION, false);
  updateWebText("p09_sw_version", VERSION, false);

  getBuildDateTime(tmpMessage);
  updateWebText("p09_sw_date", tmpMessage, false);

  // ESP informations
  updateWebTextFloat("p09_esp_flash_usage", (float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace(), false, 0);
  updateWebTextFloat("p09_esp_heap_usage", (float)(ESP.getHeapSize() - ESP.getFreeHeap()) * 100 / ESP.getHeapSize(), false, 0);
  updateWebTextFloat("p09_esp_maxallocheap", (float)ESP.getMaxAllocHeap() / 1000.0, false, 0);
  updateWebTextFloat("p09_esp_minfreeheap", (float)ESP.getMinFreeHeap() / 1000.0, false, 0);

  // Uptime and restart reason
  char uptimeStr[64];
  getUptime(uptimeStr, sizeof(uptimeStr));
  updateWebText("p09_uptime", uptimeStr, false);
  char restartReason[64];
  getRestartReason(restartReason, sizeof(restartReason));
  updateWebText("p09_restart_reason", restartReason, false);

  // actual date and time
  updateWebText("p12_ntp_date", getDateStringWeb(), true);
  updateWebText("p12_ntp_time", getTimeString(), true);
}

/**
 * *******************************************************************
 * @brief   update Alarm messages of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271AlarmElements() {

  if (strDiff(&KmAlarmHash[0], pkmAlarmStr->alarm1)) {
    updateWebText("p08_err_msg1", pkmAlarmStr->alarm1, false);
  }
  if (strDiff(&KmAlarmHash[1], pkmAlarmStr->alarm2)) {
    updateWebText("p08_err_msg2", pkmAlarmStr->alarm2, false);
  }
  if (strDiff(&KmAlarmHash[2], pkmAlarmStr->alarm3)) {
    updateWebText("p08_err_msg3", pkmAlarmStr->alarm3, false);
  }
  if (strDiff(&KmAlarmHash[3], pkmAlarmStr->alarm4)) {
    updateWebText("p08_err_msg4", pkmAlarmStr->alarm4, false);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271ConfigElements() {

  // heating circuit 1 configuration
  if (config.km271.use_hc1) {
    if (strDiff(&KmCfgHash[hc1_frost_protection_threshold], pkmConfigStr->hc1_frost_protection_threshold)) {
      updateWebValueInt("p02_hc1_frost_protection_threshold", pkmConfigNum->hc1_frost_protection_threshold);
      updateWebTextInt("p02_hc1_frost_protection_threshold_txt", pkmConfigNum->hc1_frost_protection_threshold, false);
      updateWebText("p03_hc1_frost_protection_threshold", pkmConfigStr->hc1_frost_protection_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_summer_mode_threshold], pkmConfigStr->hc1_summer_mode_threshold)) {
      updateWebValueInt("p02_hc1_summer_mode_threshold", pkmConfigNum->hc1_summer_mode_threshold);
      updateWebTextInt("p02_hc1_summer_mode_threshold_txt", pkmConfigNum->hc1_summer_mode_threshold, false);
      updateWebText("p03_hc1_summer_mode_threshold", pkmConfigStr->hc1_summer_mode_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_night_temp], pkmConfigStr->hc1_night_temp)) {
      updateWebText("p03_hc1_night_temp", pkmConfigStr->hc1_night_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_day_temp], pkmConfigStr->hc1_day_temp)) {
      updateWebText("p03_hc1_day_temp", pkmConfigStr->hc1_day_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_operation_mode], pkmConfigStr->hc1_operation_mode)) {
      updateWebState("p02_hc1_opmode_night", pkmConfigNum->hc1_operation_mode == 0 ? true : false);
      updateWebState("p02_hc1_opmode_day", pkmConfigNum->hc1_operation_mode == 1 ? true : false);
      updateWebState("p02_hc1_opmode_auto", pkmConfigNum->hc1_operation_mode == 2 ? true : false);
      updateWebText("p03_hc1_operation_mode", pkmConfigStr->hc1_operation_mode, false);
    } else if (strDiff(&KmCfgHash[hc1_holiday_temp], pkmConfigStr->hc1_holiday_temp)) {
      updateWebText("p03_hc1_holiday_temp", pkmConfigStr->hc1_holiday_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_max_temp], pkmConfigStr->hc1_max_temp)) {
      updateWebText("p03_hc1_max_temp", pkmConfigStr->hc1_max_temp, false);
    } else if (strDiff(&KmCfgHash[hc1_interpretation], pkmConfigStr->hc1_interpretation)) {
      updateWebValueInt("p02_hc1_interpretation", pkmConfigNum->hc1_interpretation);
      updateWebTextInt("p02_hc1_interpretation_txt", pkmConfigNum->hc1_interpretation, false);
      updateWebText("p03_hc1_interpretation", pkmConfigStr->hc1_interpretation, false);
    } else if (strDiff(&KmCfgHash[hc1_switch_on_temperature], pkmConfigStr->hc1_switch_on_temperature)) {
      updateWebText("p03_hc1_switch_on_temperature", pkmConfigStr->hc1_switch_on_temperature, false);
    } else if (strDiff(&KmCfgHash[hc1_switch_off_threshold], pkmConfigStr->hc1_switch_off_threshold)) {
      updateWebValueInt("p02_hc1_switch_off_threshold", pkmConfigNum->hc1_switch_off_threshold);
      updateWebTextInt("p02_hc1_switch_off_threshold_txt", pkmConfigNum->hc1_switch_off_threshold, false);
      updateWebText("p03_hc1_switch_off_threshold", pkmConfigStr->hc1_switch_off_threshold, false);
    } else if (strDiff(&KmCfgHash[hc1_reduction_mode], pkmConfigStr->hc1_reduction_mode)) {
      updateWebText("p03_hc1_reduction_mode", pkmConfigStr->hc1_reduction_mode, false);
      updateWebValueInt("p02_hc1_reduct_mode", pkmConfigNum->hc1_reduction_mode);
    } else if (strDiff(&KmCfgHash[hc1_heating_system], pkmConfigStr->hc1_heating_system)) {
      updateWebText("p03_hc1_heating_system", pkmConfigStr->hc1_heating_system, false);
    } else if (strDiff(&KmCfgHash[hc1_temp_offset], pkmConfigStr->hc1_temp_offset)) {
      updateWebText("p03_hc1_temp_offset", pkmConfigStr->hc1_temp_offset, false);
    } else if (strDiff(&KmCfgHash[hc1_remotecontrol], pkmConfigStr->hc1_remotecontrol)) {
      updateWebText("p03_hc1_remotecontrol", pkmConfigStr->hc1_remotecontrol, false);
    } else if (strDiff(&KmCfgHash[hc1_program], pkmConfigStr->hc1_program)) {
      updateWebValueInt("p02_hc1_prg", pkmConfigNum->hc1_program);
    } else if (strDiff(&KmCfgHash[hc1_holiday_days], pkmConfigStr->hc1_holiday_days)) {
      updateWebValueInt("p02_hc1_holiday_days", pkmConfigNum->hc1_holiday_days);
    } else if (strDiff(&KmCfgHash[hc1_timer01], pkmConfigStr->hc1_timer01)) {
      updateWebText("p03_hc1_timer01", pkmConfigStr->hc1_timer01, false);
    } else if (strDiff(&KmCfgHash[hc1_timer02], pkmConfigStr->hc1_timer02)) {
      updateWebText("p03_hc1_timer02", pkmConfigStr->hc1_timer02, false);
    } else if (strDiff(&KmCfgHash[hc1_timer03], pkmConfigStr->hc1_timer03)) {
      updateWebText("p03_hc1_timer03", pkmConfigStr->hc1_timer03, false);
    } else if (strDiff(&KmCfgHash[hc1_timer04], pkmConfigStr->hc1_timer04)) {
      updateWebText("p03_hc1_timer04", pkmConfigStr->hc1_timer04, false);
    } else if (strDiff(&KmCfgHash[hc1_timer05], pkmConfigStr->hc1_timer05)) {
      updateWebText("p03_hc1_timer05", pkmConfigStr->hc1_timer05, false);
    } else if (strDiff(&KmCfgHash[hc1_timer06], pkmConfigStr->hc1_timer06)) {
      updateWebText("p03_hc1_timer06", pkmConfigStr->hc1_timer06, false);
    } else if (strDiff(&KmCfgHash[hc1_timer07], pkmConfigStr->hc1_timer07)) {
      updateWebText("p03_hc1_timer07", pkmConfigStr->hc1_timer07, false);
    } else if (strDiff(&KmCfgHash[hc1_timer08], pkmConfigStr->hc1_timer08)) {
      updateWebText("p03_hc1_timer08", pkmConfigStr->hc1_timer08, false);
    } else if (strDiff(&KmCfgHash[hc1_timer09], pkmConfigStr->hc1_timer09)) {
      updateWebText("p03_hc1_timer09", pkmConfigStr->hc1_timer09, false);
    } else if (strDiff(&KmCfgHash[hc1_timer10], pkmConfigStr->hc1_timer10)) {
      updateWebText("p03_hc1_timer10", pkmConfigStr->hc1_timer10, false);
    } else if (strDiff(&KmCfgHash[hc1_timer11], pkmConfigStr->hc1_timer11)) {
      updateWebText("p03_hc1_timer11", pkmConfigStr->hc1_timer11, false);
    } else if (strDiff(&KmCfgHash[hc1_timer12], pkmConfigStr->hc1_timer12)) {
      updateWebText("p03_hc1_timer12", pkmConfigStr->hc1_timer12, false);
    } else if (strDiff(&KmCfgHash[hc1_timer13], pkmConfigStr->hc1_timer13)) {
      updateWebText("p03_hc1_timer13", pkmConfigStr->hc1_timer13, false);
    } else if (strDiff(&KmCfgHash[hc1_timer14], pkmConfigStr->hc1_timer14)) {
      updateWebText("p03_hc1_timer14", pkmConfigStr->hc1_timer14, false);
    }
  }

  // heating circuit 2 configuration
  if (config.km271.use_hc2) {
    if (strDiff(&KmCfgHash[hc2_frost_protection_threshold], pkmConfigStr->hc2_frost_protection_threshold)) {
      updateWebValueInt("p02_hc2_frost_protection_threshold", pkmConfigNum->hc2_frost_protection_threshold);
      updateWebTextInt("p04_hc2_frost_protection_threshold_txt", pkmConfigNum->hc2_frost_protection_threshold, false);
      updateWebText("p04_hc2_frost_protection_threshold", pkmConfigStr->hc2_frost_protection_threshold, false);
    } else if (strDiff(&KmCfgHash[hc2_summer_mode_threshold], pkmConfigStr->hc2_summer_mode_threshold)) {
      updateWebValueInt("p02_hc2_summer_mode_threshold", pkmConfigNum->hc2_summer_mode_threshold);
      updateWebTextInt("p04_hc2_summer_mode_threshold_txt", pkmConfigNum->hc2_summer_mode_threshold, false);
      updateWebText("p04_hc2_summer_mode_threshold", pkmConfigStr->hc2_summer_mode_threshold, false);
    }

    else if (strDiff(&KmCfgHash[hc2_night_temp], pkmConfigStr->hc2_night_temp)) {
      updateWebText("p04_hc2_night_temp", pkmConfigStr->hc2_night_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_day_temp], pkmConfigStr->hc2_day_temp)) {
      updateWebText("p04_hc2_day_temp", pkmConfigStr->hc2_day_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_operation_mode], pkmConfigStr->hc2_operation_mode)) {
      updateWebState("p02_hc2_opmode_night", pkmConfigNum->hc2_operation_mode == 0 ? true : false);
      updateWebState("p02_hc2_opmode_day", pkmConfigNum->hc2_operation_mode == 1 ? true : false);
      updateWebState("p02_hc2_opmode_auto", pkmConfigNum->hc2_operation_mode == 2 ? true : false);
      updateWebText("p04_hc2_operation_mode", pkmConfigStr->hc2_operation_mode, false);
    } else if (strDiff(&KmCfgHash[hc2_holiday_temp], pkmConfigStr->hc2_holiday_temp)) {
      updateWebText("p04_hc2_holiday_temp", pkmConfigStr->hc2_holiday_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_max_temp], pkmConfigStr->hc2_max_temp)) {
      updateWebText("p04_hc2_max_temp", pkmConfigStr->hc2_max_temp, false);
    } else if (strDiff(&KmCfgHash[hc2_interpretation], pkmConfigStr->hc2_interpretation)) {
      updateWebValueInt("p02_hc2_interpretation", pkmConfigNum->hc2_interpretation);
      updateWebTextInt("p02_hc2_interpretation_txt", pkmConfigNum->hc2_interpretation, false);
      updateWebText("p04_hc2_interpretation", pkmConfigStr->hc2_interpretation, false);
    } else if (strDiff(&KmCfgHash[hc2_switch_on_temperature], pkmConfigStr->hc2_switch_on_temperature)) {
      updateWebText("p04_hc2_switch_on_temperature", pkmConfigStr->hc2_switch_on_temperature, false);
    } else if (strDiff(&KmCfgHash[hc2_switch_off_threshold], pkmConfigStr->hc2_switch_off_threshold)) {
      updateWebValueInt("p02_hc2_switch_off_threshold", pkmConfigNum->hc2_switch_off_threshold);
      updateWebTextInt("p02_hc2_switch_off_threshold_txt", pkmConfigNum->hc2_switch_off_threshold, false);
      updateWebText("p04_hc2_switch_off_threshold", pkmConfigStr->hc2_switch_off_threshold, false);
    } else if (strDiff(&KmCfgHash[hc2_reduction_mode], pkmConfigStr->hc2_reduction_mode)) {
      updateWebText("p04_hc2_reduction_mode", pkmConfigStr->hc2_reduction_mode, false);
      updateWebValueInt("p02_hc2_reduct_mode", pkmConfigNum->hc2_reduction_mode);
    } else if (strDiff(&KmCfgHash[hc2_heating_system], pkmConfigStr->hc2_heating_system)) {
      updateWebText("p04_hc2_heating_system", pkmConfigStr->hc2_heating_system, false);
    } else if (strDiff(&KmCfgHash[hc2_temp_offset], pkmConfigStr->hc2_temp_offset)) {
      updateWebText("p04_hc2_temp_offset", pkmConfigStr->hc2_temp_offset, false);
    } else if (strDiff(&KmCfgHash[hc2_remotecontrol], pkmConfigStr->hc2_remotecontrol)) {
      updateWebText("p04_hc2_remotecontrol", pkmConfigStr->hc2_remotecontrol, false);
    } else if (strDiff(&KmCfgHash[hc2_program], pkmConfigStr->hc2_program)) {
      updateWebValueInt("p02_hc2_prg", pkmConfigNum->hc2_program);
    } else if (strDiff(&KmCfgHash[hc2_holiday_days], pkmConfigStr->hc2_holiday_days)) {
      updateWebValueInt("p04_hc2_holiday_days", pkmConfigNum->hc2_holiday_days);
    } else if (strDiff(&KmCfgHash[hc2_timer01], pkmConfigStr->hc2_timer01)) {
      updateWebText("p04_hc2_timer01", pkmConfigStr->hc2_timer01, false);
    } else if (strDiff(&KmCfgHash[hc2_timer02], pkmConfigStr->hc2_timer02)) {
      updateWebText("p04_hc2_timer02", pkmConfigStr->hc2_timer02, false);
    } else if (strDiff(&KmCfgHash[hc2_timer03], pkmConfigStr->hc2_timer03)) {
      updateWebText("p04_hc2_timer03", pkmConfigStr->hc2_timer03, false);
    } else if (strDiff(&KmCfgHash[hc2_timer04], pkmConfigStr->hc2_timer04)) {
      updateWebText("p04_hc2_timer04", pkmConfigStr->hc2_timer04, false);
    } else if (strDiff(&KmCfgHash[hc2_timer05], pkmConfigStr->hc2_timer05)) {
      updateWebText("p04_hc2_timer05", pkmConfigStr->hc2_timer05, false);
    } else if (strDiff(&KmCfgHash[hc2_timer06], pkmConfigStr->hc2_timer06)) {
      updateWebText("p04_hc2_timer06", pkmConfigStr->hc2_timer06, false);
    } else if (strDiff(&KmCfgHash[hc2_timer07], pkmConfigStr->hc2_timer07)) {
      updateWebText("p04_hc2_timer07", pkmConfigStr->hc2_timer07, false);
    } else if (strDiff(&KmCfgHash[hc2_timer08], pkmConfigStr->hc2_timer08)) {
      updateWebText("p04_hc2_timer08", pkmConfigStr->hc2_timer08, false);
    } else if (strDiff(&KmCfgHash[hc2_timer09], pkmConfigStr->hc2_timer09)) {
      updateWebText("p04_hc2_timer09", pkmConfigStr->hc2_timer09, false);
    } else if (strDiff(&KmCfgHash[hc2_timer10], pkmConfigStr->hc2_timer10)) {
      updateWebText("p04_hc2_timer10", pkmConfigStr->hc2_timer10, false);
    } else if (strDiff(&KmCfgHash[hc2_timer11], pkmConfigStr->hc2_timer11)) {
      updateWebText("p04_hc2_timer11", pkmConfigStr->hc2_timer11, false);
    } else if (strDiff(&KmCfgHash[hc2_timer12], pkmConfigStr->hc2_timer12)) {
      updateWebText("p04_hc2_timer12", pkmConfigStr->hc2_timer12, false);
    } else if (strDiff(&KmCfgHash[hc2_timer13], pkmConfigStr->hc2_timer13)) {
      updateWebText("p04_hc2_timer13", pkmConfigStr->hc2_timer13, false);
    } else if (strDiff(&KmCfgHash[hc2_timer14], pkmConfigStr->hc2_timer14)) {
      updateWebText("p04_hc2_timer14", pkmConfigStr->hc2_timer14, false);
    }
  }

  // hot-water config values
  if (config.km271.use_ww) {
    if (strDiff(&KmCfgHash[ww_priority], pkmConfigStr->ww_priority)) {
      updateWebText("p05_hw_priority", pkmConfigStr->ww_priority, false);
    } else if (strDiff(&KmCfgHash[ww_temp], pkmConfigStr->ww_temp)) {
      updateWebValueInt("p02_ww_temp", pkmConfigNum->ww_temp);
      updateWebTextInt("p02_ww_temp_txt", pkmConfigNum->ww_temp, false);
      updateWebText("p05_hw_temp", pkmConfigStr->ww_temp, false);
    } else if (strDiff(&KmCfgHash[ww_operation_mode], pkmConfigStr->ww_operation_mode)) {
      updateWebState("p02_ww_opmode_night", pkmConfigNum->ww_operation_mode == 0 ? true : false);
      updateWebState("p02_ww_opmode_day", pkmConfigNum->ww_operation_mode == 1 ? true : false);
      updateWebState("p02_ww_opmode_auto", pkmConfigNum->ww_operation_mode == 2 ? true : false);
      updateWebText("p05_hw_operation_mode", pkmConfigStr->ww_operation_mode, false);
    } else if (strDiff(&KmCfgHash[ww_processing], pkmConfigStr->ww_processing)) {
      updateWebText("p05_hw_processing", pkmConfigStr->ww_processing, false);
    } else if (strDiff(&KmCfgHash[ww_circulation], pkmConfigStr->ww_circulation)) {
      updateWebValueInt("p02_ww_circulation", pkmConfigNum->ww_circulation);
      updateWebTextInt("p02_ww_circulation_txt", pkmConfigNum->ww_circulation, false);
      updateWebText("p05_hw_circulation", pkmConfigStr->ww_circulation, false);
    }
  }

  // general config values
  if (strDiff(&KmCfgHash[language], pkmConfigStr->language)) {
    updateWebText("p07_language", pkmConfigStr->language, false);
  } else if (strDiff(&KmCfgHash[display], pkmConfigStr->display)) {
    updateWebText("p07_display", pkmConfigStr->display, false);
  } else if (strDiff(&KmCfgHash[burner_type], pkmConfigStr->burner_type)) {
    updateWebText("p07_burner_type", pkmConfigStr->burner_type, false);
  } else if (strDiff(&KmCfgHash[max_boiler_temperature], pkmConfigStr->max_boiler_temperature)) {
    updateWebText("p06_max_boiler_temperature", pkmConfigStr->max_boiler_temperature, false);
  } else if (strDiff(&KmCfgHash[pump_logic_temp], pkmConfigStr->pump_logic_temp)) {
    updateWebText("p07_pump_logic_temp", pkmConfigStr->pump_logic_temp, false);
  } else if (strDiff(&KmCfgHash[exhaust_gas_temperature_threshold], pkmConfigStr->exhaust_gas_temperature_threshold)) {
    updateWebText("p07_exhaust_gas_temperature_threshold", pkmConfigStr->exhaust_gas_temperature_threshold, false);
  } else if (strDiff(&KmCfgHash[burner_min_modulation], pkmConfigStr->burner_min_modulation)) {
    updateWebText("p07_burner_min_modulation", pkmConfigStr->burner_min_modulation, false);
  } else if (strDiff(&KmCfgHash[burner_modulation_runtime], pkmConfigStr->burner_modulation_runtime)) {
    updateWebText("p07_burner_modulation_runtime", pkmConfigStr->burner_modulation_runtime, false);
  } else if (strDiff(&KmCfgHash[building_type], pkmConfigStr->building_type)) {
    updateWebText("p07_building_type", pkmConfigStr->building_type, false);
  } else if (strDiff(&KmCfgHash[time_offset], pkmConfigStr->time_offset)) {
    updateWebText("p07_time_offset", pkmConfigStr->time_offset, false);
  }
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none
 * @return  none
 * *******************************************************************/
void updateKm271StatusElements() {

  // heating circuit 1 values
  if (config.km271.use_hc1) {
    if (kmStatusCpy.HC1_OperatingStates_1 != pkmStatus->HC1_OperatingStates_1) {
      kmStatusCpy.HC1_OperatingStates_1 = pkmStatus->HC1_OperatingStates_1;

      // AUTOMATIC / MANUAL (Day/Night)
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        snprintf(tmpMessage, sizeof(tmpMessage), "%s ", webText.AUTOMATIC[config.lang]);
        updateWebSetIcon("p01_hc1_opmode_icon", "i_auto");
      } else {                                               // MANUAL
        if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
          updateWebSetIcon("p01_hc1_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
          updateWebSetIcon("p01_hc1_opmode_icon", "i_manual");
        }
      }
      updateWebText("p01_hc1_opmode", tmpMessage, false);

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc1_summer_winter",
                      (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc1_summer_winter_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc1_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]),
            false);
        updateWebSetIcon("p01_hc1_summer_winter_icon",
                         (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc1_summer_mode_threshold ? "i_summer" : "i_winter"));
      }

      updateWebText("p03_hc1_ov1_off_time_optimization", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)), false);
      updateWebText("p03_hc1_ov1_on_time_optimization", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)), false);
      updateWebText("p03_hc1_ov1_automatic", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)), false);
      updateWebText("p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)), false);
      updateWebText("p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)), false);
      updateWebText("p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)), false);
      updateWebText("p03_hc1_ov1_frost_protection", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)), false);
    } else if (kmStatusCpy.HC1_OperatingStates_2 != pkmStatus->HC1_OperatingStates_2) {
      kmStatusCpy.HC1_OperatingStates_2 = pkmStatus->HC1_OperatingStates_2;

      // Day / Night
      updateWebText("p01_hc1_day_night", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]),
                    false);
      updateWebSetIcon("p01_hc1_day_night_icon", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? "i_day" : "i_night"));

      updateWebText("p03_hc1_ov2_summer", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)), false);
      updateWebText("p03_hc1_ov2_day", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)), false);
      updateWebText("p03_hc1_ov2_no_conn_to_remotectrl", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)), false);
      updateWebText("p03_hc1_ov2_remotectrl_error", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)), false);
      updateWebText("p03_hc1_ov2_failure_flow_sensor", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)), false);
      updateWebText("p03_hc1_ov2_flow_at_maximum", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)), false);
      updateWebText("p03_hc1_ov2_external_signal_input", errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)), false);
    } else if (kmStatusCpy.HC1_HeatingForwardTargetTemp != pkmStatus->HC1_HeatingForwardTargetTemp) {
      kmStatusCpy.HC1_HeatingForwardTargetTemp = pkmStatus->HC1_HeatingForwardTargetTemp;
      updateWebTextInt("p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
      updateWebTextInt("p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    } else if (kmStatusCpy.HC1_HeatingForwardActualTemp != pkmStatus->HC1_HeatingForwardActualTemp) {
      kmStatusCpy.HC1_HeatingForwardActualTemp = pkmStatus->HC1_HeatingForwardActualTemp;
      updateWebTextInt("p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
      updateWebTextInt("p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    } else if (kmStatusCpy.HC1_RoomTargetTemp != pkmStatus->HC1_RoomTargetTemp) {
      kmStatusCpy.HC1_RoomTargetTemp = pkmStatus->HC1_RoomTargetTemp;
      updateWebTextInt("p03_hc1_room_setpoint", kmStatusCpy.HC1_RoomTargetTemp, false);
    } else if (kmStatusCpy.HC1_RoomActualTemp != pkmStatus->HC1_RoomActualTemp) {
      kmStatusCpy.HC1_RoomActualTemp = pkmStatus->HC1_RoomActualTemp;
      updateWebTextInt("p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp, false);
    } else if (kmStatusCpy.HC1_SwitchOnOptimizationTime != pkmStatus->HC1_SwitchOnOptimizationTime) {
      kmStatusCpy.HC1_SwitchOnOptimizationTime = pkmStatus->HC1_SwitchOnOptimizationTime;
      updateWebTextInt("p03_hc1_on_time_opt_duration", kmStatusCpy.HC1_SwitchOnOptimizationTime, false);
    } else if (kmStatusCpy.HC1_SwitchOffOptimizationTime != pkmStatus->HC1_SwitchOffOptimizationTime) {
      kmStatusCpy.HC1_SwitchOffOptimizationTime = pkmStatus->HC1_SwitchOffOptimizationTime;
      updateWebTextInt("p03_hc1_off_time_opt_duration", kmStatusCpy.HC1_SwitchOffOptimizationTime, false);
    } else if (kmStatusCpy.HC1_PumpPower != pkmStatus->HC1_PumpPower) {
      kmStatusCpy.HC1_PumpPower = pkmStatus->HC1_PumpPower;
      updateWebText("p01_hc1_pump", (kmStatusCpy.HC1_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false);
      updateWebTextInt("p03_hc1_pump", kmStatusCpy.HC1_PumpPower, false);
    } else if (kmStatusCpy.HC1_MixingValue != pkmStatus->HC1_MixingValue) {
      kmStatusCpy.HC1_MixingValue = pkmStatus->HC1_MixingValue;
      updateWebTextInt("p03_hc1_mixer", kmStatusCpy.HC1_MixingValue, false);
    } else if (kmStatusCpy.HC1_HeatingCurvePlus10 != pkmStatus->HC1_HeatingCurvePlus10) {
      kmStatusCpy.HC1_HeatingCurvePlus10 = pkmStatus->HC1_HeatingCurvePlus10;
      updateWebTextInt("p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10, false);
    } else if (kmStatusCpy.HC1_HeatingCurve0 != pkmStatus->HC1_HeatingCurve0) {
      kmStatusCpy.HC1_HeatingCurve0 = pkmStatus->HC1_HeatingCurve0;
      updateWebTextInt("p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0, false);
    } else if (kmStatusCpy.HC1_HeatingCurveMinus10 != pkmStatus->HC1_HeatingCurveMinus10) {
      kmStatusCpy.HC1_HeatingCurveMinus10 = pkmStatus->HC1_HeatingCurveMinus10;
      updateWebTextInt("p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10, false);
    }
  } // END HC1

  // heating circuit 2 values
  if (config.km271.use_hc2) {
    if (kmStatusCpy.HC2_OperatingStates_1 != pkmStatus->HC2_OperatingStates_1) {
      kmStatusCpy.HC2_OperatingStates_1 = pkmStatus->HC2_OperatingStates_1;
      // HC2-Operating State
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        snprintf(tmpMessage, sizeof(tmpMessage), "%s", webText.AUTOMATIC[config.lang]);
        updateWebSetIcon("p01_hc2_opmode_icon", "i_auto");
      } else {                                               // MANUAL
        if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
          updateWebSetIcon("p01_hc2_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
          updateWebSetIcon("p01_hc2_opmode_icon", "i_manual");
        }
      }
      updateWebText("p01_hc2_opmode", tmpMessage, false);

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) { // AUTOMATIC
        updateWebText("p01_hc2_summer_winter",
                      (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
        updateWebSetIcon("p01_hc2_summer_winter_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? "i_summer" : "i_winter"));
      } else { // generate status from actual temperature and summer threshold
        updateWebText(
            "p01_hc2_summer_winter",
            (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]),
            false);
        updateWebSetIcon("p01_hc2_summer_winter_icon",
                         (kmStatusCpy.OutsideDampedTemp > pkmConfigNum->hc2_summer_mode_threshold ? "i_summer" : "i_winter"));
      }

      updateWebText("p04_hc2_ov1_off_time_optimization", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)), false);
      updateWebText("p04_hc2_ov1_on_time_optimization", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)), false);
      updateWebText("p04_hc2_ov1_automatic", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)), false);
      updateWebText("p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)), false);
      updateWebText("p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)), false);
      updateWebText("p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)), false);
      updateWebText("p04_hc2_ov1_frost_protection", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)), false);
    } else if (kmStatusCpy.HC2_OperatingStates_2 != pkmStatus->HC2_OperatingStates_2) {
      kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2;
      // Day / Night
      updateWebText("p01_hc2_day_night", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]),
                    false);
      updateWebSetIcon("p01_hc2_day_night_icon", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? "i_day" : "i_night"));

      updateWebText("p04_hc2_ov2_summer", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)), false);
      updateWebText("p04_hc2_ov2_day", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)), false);
      updateWebText("p04_hc2_ov2_no_conn_to_remotectrl", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)), false);
      updateWebText("p04_hc2_ov2_remotectrl_error", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)), false);
      updateWebText("p04_hc2_ov2_failure_flow_sensor", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)), false);
      updateWebText("p04_hc2_ov2_flow_at_maximum", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)), false);
      updateWebText("p04_hc2_ov2_external_signal_input", errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)), false);
    } else if (kmStatusCpy.HC2_HeatingForwardTargetTemp != pkmStatus->HC2_HeatingForwardTargetTemp) {
      kmStatusCpy.HC2_HeatingForwardTargetTemp = pkmStatus->HC2_HeatingForwardTargetTemp;
      updateWebTextInt("p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
      updateWebTextInt("p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
    } else if (kmStatusCpy.HC2_HeatingForwardActualTemp != pkmStatus->HC2_HeatingForwardActualTemp) {
      kmStatusCpy.HC2_HeatingForwardActualTemp = pkmStatus->HC2_HeatingForwardActualTemp;
      updateWebTextInt("p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
      updateWebTextInt("p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
    } else if (kmStatusCpy.HC2_RoomTargetTemp != pkmStatus->HC2_RoomTargetTemp) {
      kmStatusCpy.HC2_RoomTargetTemp = pkmStatus->HC2_RoomTargetTemp;
      updateWebTextInt("p04_hc2_room_setpoint", kmStatusCpy.HC2_RoomTargetTemp, false);
    } else if (kmStatusCpy.HC2_RoomActualTemp != pkmStatus->HC2_RoomActualTemp) {
      kmStatusCpy.HC2_RoomActualTemp = pkmStatus->HC2_RoomActualTemp;
      updateWebTextInt("p04_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp, false);
    } else if (kmStatusCpy.HC2_SwitchOnOptimizationTime != pkmStatus->HC2_SwitchOnOptimizationTime) {
      kmStatusCpy.HC2_SwitchOnOptimizationTime = pkmStatus->HC2_SwitchOnOptimizationTime;
      updateWebTextInt("p04_hc2_on_time_opt_duration", kmStatusCpy.HC2_SwitchOnOptimizationTime, false);
    } else if (kmStatusCpy.HC2_SwitchOffOptimizationTime != pkmStatus->HC2_SwitchOffOptimizationTime) {
      kmStatusCpy.HC2_SwitchOffOptimizationTime = pkmStatus->HC2_SwitchOffOptimizationTime;
      updateWebTextInt("p04_hc2_off_time_opt_duration", kmStatusCpy.HC2_SwitchOffOptimizationTime, false);
    } else if (kmStatusCpy.HC2_PumpPower != pkmStatus->HC2_PumpPower) {
      kmStatusCpy.HC2_PumpPower = pkmStatus->HC2_PumpPower;
      updateWebText("p01_hc2_pump", (kmStatusCpy.HC2_PumpPower == 0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false);
      updateWebTextInt("p04_hc2_pump", kmStatusCpy.HC2_PumpPower, false);
    } else if (kmStatusCpy.HC2_MixingValue != pkmStatus->HC2_MixingValue) {
      kmStatusCpy.HC2_MixingValue = pkmStatus->HC2_MixingValue;
      updateWebTextInt("p04_hc2_mixer", kmStatusCpy.HC2_MixingValue, false);
    } else if (kmStatusCpy.HC2_HeatingCurvePlus10 != pkmStatus->HC2_HeatingCurvePlus10) {
      kmStatusCpy.HC2_HeatingCurvePlus10 = pkmStatus->HC2_HeatingCurvePlus10;
      updateWebTextInt("p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10, false);
    } else if (kmStatusCpy.HC2_HeatingCurve0 != pkmStatus->HC2_HeatingCurve0) {
      kmStatusCpy.HC2_HeatingCurve0 = pkmStatus->HC2_HeatingCurve0;
      updateWebTextInt("p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0, false);
    } else if (kmStatusCpy.HC2_HeatingCurveMinus10 != pkmStatus->HC2_HeatingCurveMinus10) {
      kmStatusCpy.HC2_HeatingCurveMinus10 = pkmStatus->HC2_HeatingCurveMinus10;
      updateWebTextInt("p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10, false);
    }
  } // END HC2

  // hot-water values
  if (config.km271.use_ww) {
    if (kmStatusCpy.HotWaterOperatingStates_1 != pkmStatus->HotWaterOperatingStates_1) {
      kmStatusCpy.HotWaterOperatingStates_1 = pkmStatus->HotWaterOperatingStates_1;
      // WW-Operating State
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) { // AUTOMATIC
        snprintf(tmpMessage, sizeof(tmpMessage), "%s", webText.AUTOMATIC[config.lang]);
        updateWebSetIcon("p01_ww_opmode_icon", "i_auto");
      } else {                                                   // MANUAL
        if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)) { // DAY
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
          updateWebSetIcon("p01_ww_opmode_icon", "i_manual");
        } else { // NIGHT
          snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
          updateWebSetIcon("p01_ww_opmode_icon", "i_manual");
        }
      }
      updateWebText("p01_ww_opmode", tmpMessage, false);

      updateWebText("p05_hw_ov1_auto", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)), false);
      updateWebText("p05_hw_ov1_disinfection", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)), false);
      updateWebText("p05_hw_ov1_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)), false);
      updateWebText("p05_hw_ov1_holiday", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)), false);
      updateWebText("p05_hw_ov1_err_disinfection", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)), false);
      updateWebText("p05_hw_ov1_err_sensor", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)), false);
      updateWebText("p05_hw_ov1_stays_cold", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)), false);
      updateWebText("p05_hw_ov1_err_anode", errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)), false);
    } else if (kmStatusCpy.HotWaterOperatingStates_2 != pkmStatus->HotWaterOperatingStates_2) {
      kmStatusCpy.HotWaterOperatingStates_2 = pkmStatus->HotWaterOperatingStates_2;
      updateWebText("p05_hw_ov2_load", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)), false);
      updateWebText("p05_hw_ov2_manual", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)), false);
      updateWebText("p05_hw_ov2_reload", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)), false);
      updateWebText("p05_hw_ov2_off_time_opt_duration", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)), false);
      updateWebText("p05_hw_ov2_on_time_opt_duration", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)), false);
      updateWebText("p05_hw_ov2_day", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)), false);
      updateWebText("p05_hw_ov2_hot", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)), false);
      updateWebText("p05_hw_ov2_priority", onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)), false);
    } else if (kmStatusCpy.HotWaterTargetTemp != pkmStatus->HotWaterTargetTemp) {
      kmStatusCpy.HotWaterTargetTemp = pkmStatus->HotWaterTargetTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterTargetTemp);
      updateWebText("p05_hw_set_temp", tmpMessage, false);
      updateWebTextInt("p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp, false);
    } else if (kmStatusCpy.HotWaterActualTemp != pkmStatus->HotWaterActualTemp) {
      kmStatusCpy.HotWaterActualTemp = pkmStatus->HotWaterActualTemp;
      snprintf(tmpMessage, sizeof(tmpMessage), "%hhu °C", kmStatusCpy.HotWaterActualTemp);
      updateWebText("p05_hw_act_temp", tmpMessage, false);
      updateWebTextInt("p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp, false);
    } else if (kmStatusCpy.HotWaterOptimizationTime != pkmStatus->HotWaterOptimizationTime) {
      kmStatusCpy.HotWaterOptimizationTime = pkmStatus->HotWaterOptimizationTime;
      updateWebText("p05_hw_on_time_opt_duration", onOffString(kmStatusCpy.HotWaterOptimizationTime), false);
    } else if (kmStatusCpy.HotWaterPumpStates != pkmStatus->HotWaterPumpStates) {
      kmStatusCpy.HotWaterPumpStates = pkmStatus->HotWaterPumpStates;
      updateWebText("p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)), false);
      updateWebText("p05_hw_pump_type_circulation", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)), false);
      updateWebText("p05_hw_pump_type_groundwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)), false);
    }
  } // END HotWater

  // general values
  if (kmStatusCpy.BoilerForwardTargetTemp != pkmStatus->BoilerForwardTargetTemp) {
    kmStatusCpy.BoilerForwardTargetTemp = pkmStatus->BoilerForwardTargetTemp;
    updateWebTextInt("p06_boiler_setpoint", kmStatusCpy.BoilerForwardTargetTemp, false);
    updateWebTextInt("p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp, false);
  } else if (kmStatusCpy.BoilerForwardActualTemp != pkmStatus->BoilerForwardActualTemp) {
    kmStatusCpy.BoilerForwardActualTemp = pkmStatus->BoilerForwardActualTemp;
    updateWebTextInt("p06_boiler_temp", kmStatusCpy.BoilerForwardActualTemp, false);
    updateWebTextInt("p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp, false);
  } else if (kmStatusCpy.BurnerSwitchOnTemp != pkmStatus->BurnerSwitchOnTemp) {
    kmStatusCpy.BurnerSwitchOnTemp = pkmStatus->BurnerSwitchOnTemp;
    updateWebTextInt("p06_boiler_switch_on_temp", kmStatusCpy.BurnerSwitchOnTemp, false);
  } else if (kmStatusCpy.BurnerSwitchOffTemp != pkmStatus->BurnerSwitchOffTemp) {
    kmStatusCpy.BurnerSwitchOffTemp = pkmStatus->BurnerSwitchOffTemp;
    updateWebTextInt("p06_boiler_switch_off_temp", kmStatusCpy.BurnerSwitchOffTemp, false);
  } else if (kmStatusCpy.BoilerIntegral_1 != pkmStatus->BoilerIntegral_1) {
    kmStatusCpy.BoilerIntegral_1 = pkmStatus->BoilerIntegral_1;
  } else if (kmStatusCpy.BoilerIntegral_2 != pkmStatus->BoilerIntegral_2) {
    kmStatusCpy.BoilerIntegral_2 = pkmStatus->BoilerIntegral_2;
  } else if (kmStatusCpy.BoilerErrorStates != pkmStatus->BoilerErrorStates) {
    kmStatusCpy.BoilerErrorStates = pkmStatus->BoilerErrorStates;

    updateWebText("p06_boiler_failure_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)), false);
    updateWebText("p06_boiler_failure_boiler_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)), false);
    updateWebText("p06_boiler_failure_aux_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)), false);
    updateWebText("p06_boiler_failure_boiler_stays_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)), false);
    updateWebText("p06_boiler_failure_exhaust_gas_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)), false);
    updateWebText("p06_boiler_failure_exhaust_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)), false);
    updateWebText("p06_boiler_failure_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)), false);
    updateWebText("p06_boiler_failure_external", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)), false);
  } else if (kmStatusCpy.BoilerOperatingStates != pkmStatus->BoilerOperatingStates) {
    kmStatusCpy.BoilerOperatingStates = pkmStatus->BoilerOperatingStates;

    updateWebText("p06_boiler_state_exhaust_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)), false);
    updateWebText("p06_boiler_state_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)), false);
    updateWebText("p06_boiler_state_boiler_protection", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)), false);
    updateWebText("p06_boiler_state_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)), false);
    updateWebText("p06_boiler_state_performance_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)), false);
    updateWebText("p06_boiler_state_performance_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)), false);
    updateWebText("p06_boiler_state_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)), false);
  } else if (kmStatusCpy.BurnerStates != pkmStatus->BurnerStates) {
    kmStatusCpy.BurnerStates = pkmStatus->BurnerStates;
    updateWebText("p01_burner", (kmStatusCpy.BurnerStates == 0) ? webText.OFF[config.lang] : webText.ON[config.lang], false);
    updateWebText("p06_burner_control", cfgArrayTexts.BURNER_STATE[config.lang][kmStatusCpy.BurnerStates], false);
  } else if (kmStatusCpy.ExhaustTemp != pkmStatus->ExhaustTemp) {
    kmStatusCpy.ExhaustTemp = pkmStatus->ExhaustTemp;
    updateWebTextInt("p07_exhaust_gas_temp", kmStatusCpy.ExhaustTemp, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_2 != pkmStatus->BurnerOperatingDuration_2) {
    kmStatusCpy.BurnerOperatingDuration_2 = pkmStatus->BurnerOperatingDuration_2;
    updateWebTextInt("p06_burner_runtime_minutes65536", kmStatusCpy.BurnerOperatingDuration_2, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_1 != pkmStatus->BurnerOperatingDuration_1) {
    kmStatusCpy.BurnerOperatingDuration_1 = pkmStatus->BurnerOperatingDuration_1;
    updateWebTextInt("p06_burner_runtime_minutes256", kmStatusCpy.BurnerOperatingDuration_1, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_0 != pkmStatus->BurnerOperatingDuration_0) {
    kmStatusCpy.BurnerOperatingDuration_0 = pkmStatus->BurnerOperatingDuration_0;
    updateWebTextInt("p06_burner_runtime_minutes", kmStatusCpy.BurnerOperatingDuration_0, false);
  } else if (kmStatusCpy.BurnerOperatingDuration_Sum != pkmStatus->BurnerOperatingDuration_Sum) {
    kmStatusCpy.BurnerOperatingDuration_Sum = pkmStatus->BurnerOperatingDuration_Sum;
    updateWebTextInt("p06_burner_runtime_overall", kmStatusCpy.BurnerOperatingDuration_Sum, false);
  } else if (kmStatusCpy.BurnerCalcOilConsumption != pkmStatus->BurnerCalcOilConsumption) {
    kmStatusCpy.BurnerCalcOilConsumption = pkmStatus->BurnerCalcOilConsumption;
    // is handled somewhere else
  } else if (kmStatusCpy.OutsideTemp != pkmStatus->OutsideTemp) {
    kmStatusCpy.OutsideTemp = pkmStatus->OutsideTemp;
    updateWebTextInt("p07_outside_temp", kmStatusCpy.OutsideTemp, false);
    updateWebTextInt("p01_temp_out_act", kmStatusCpy.OutsideTemp, false);
  } else if (kmStatusCpy.OutsideDampedTemp != pkmStatus->OutsideDampedTemp) {
    kmStatusCpy.OutsideDampedTemp = pkmStatus->OutsideDampedTemp;
    updateWebTextInt("p07_outside_temp_damped", kmStatusCpy.OutsideDampedTemp, false);
    updateWebTextInt("p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp, false);
  } else if (kmStatusCpy.ControllerVersionMain != pkmStatus->ControllerVersionMain) {
    kmStatusCpy.ControllerVersionMain = pkmStatus->ControllerVersionMain;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  } else if (kmStatusCpy.ControllerVersionSub != pkmStatus->ControllerVersionSub) {
    kmStatusCpy.ControllerVersionSub = pkmStatus->ControllerVersionSub;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  } else if (kmStatusCpy.Modul != pkmStatus->Modul) {
    kmStatusCpy.Modul = pkmStatus->Modul;
    updateWebTextInt("p09_logamatic_modul", kmStatusCpy.Modul, false);
  } else if (kmStatusCpy.ERR_Alarmstatus != pkmStatus->ERR_Alarmstatus) {
    kmStatusCpy.ERR_Alarmstatus = pkmStatus->ERR_Alarmstatus;
    // TODO: check
  }
}

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

  // refresh elemets not faster than 50ms
  if (refreshTimer1.cycleTrigger(50)) {

    if (!updateSettingsElementsDone) {
      updateSettingsElements();
    } else {
      switch (UpdateCnt50ms) {
      case 0:
        updateKm271AlarmElements();
        break;
      case 1:
        updateKm271StatusElements();
        break;
      case 2:
        updateKm271ConfigElements();
        break;
      case 3:
        webReadLogBufferCyclic();
        break;
      default:
        UpdateCnt50ms = -1;
        break;
      }
      UpdateCnt50ms = (UpdateCnt50ms + 1) % 4;
    }
  }

  // refresh elemets every 1 seconds
  if (refreshTimer2.cycleTrigger(1000)) {
    switch (UpdateCnt1s) {
    case 0:
      updateSystemInfoElements();
      break;
    case 1:
      updateOilmeterElements();
      break;
    case 2:
      updateSensorElements();
      break;
    default:
      UpdateCnt1s = -1;
      break;
    }
    UpdateCnt1s = (UpdateCnt1s + 1) % 3;
  }

  // in simulation mode, load simdata and display simModeBar
  if (simulationTimer.delayOn(SIM_MODE && clientConnected && !simulationInit && !setupMode, 2000)) {
    simulationInit = true;
    showElementClass("simModeBar", true);
    startSimData();
  }

  webInitDone = true; // init done
}
