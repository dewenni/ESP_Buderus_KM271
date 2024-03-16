#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>
#include <gzip_css.h>
#include <gzip_html.h>
#include <gzip_js.h>
#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

// ======================================================
// Prototypes
// ======================================================
void webUISetup();
void webUICylic();