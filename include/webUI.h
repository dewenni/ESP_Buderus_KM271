#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <config.h>
#include <gzip_css.h>
#include <gzip_js.h>
#include <gzip_login_html.h>
#include <gzip_m_html.h>
#include <gzip_max_ws_html.h>
#include <gzip_ntp_html.h>
#include <language.h>


/* P R O T O T Y P E S ********************************************************/
void webUISetup();
void webUICyclic();
void webReadLogBuffer();

void updateWebLog(const char *entry, const char *cmd);
void updateWebLanguage(const char *language);
void updateWebText(const char *elementID, const char *text, bool isInput);
void updateWebTextInt(const char *elementID, long value, bool isInput);
void updateWebTextFloat(const char *elementID, double value, bool isInput, int decimals);
void updateWebState(const char *elementID, bool state);
void updateWebValueStr(const char *elementID, const char *value);
void updateWebValueInt(const char *elementID, long value);
void updateWebValueFloat(const char *elementID, double value, int decimals);
void showElementClass(const char *className, bool show);
void updateWebDialog(const char *elementID, const char *state);
void updateWebSetIcon(const char *elementID, const char *icon);
void updateWebJSON(JsonDocument &jsonDoc);
void updateWebHref(const char *elementID, const char *href);
void updateWebBusy(const char *elementID, bool busy);
void updateWebHideElement(const char *id, bool show);
void updateWebTooltip(const char *id, const char *tooltip);