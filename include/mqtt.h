#pragma once

// Compatibility include for the application. Transport and device hooks are
// separated so mqttBasic.cpp/.h can be updated identically in both projects.
#include <Arduino.h>
#include <config.h>
#include <language.h>
#include <mqttBasic.h>
#include <mqttCustom.h>
