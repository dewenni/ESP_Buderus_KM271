#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <EspWebUI.h>
#include <config.h>
#include <gzip_css.h>
#include <gzip_js.h>
#include <gzip_login_html.h>
#include <gzip_m_html.h>
#include <gzip_ntp_html.h>
#include <language.h>

extern EspWebUI webUI;

/* P R O T O T Y P E S ********************************************************/
void webUISetup();
void webUICyclic();
void webReadLogBuffer();
