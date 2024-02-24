#pragma once
#include "ESPTelnet.h"

// data struct for Telnet interface
typedef struct {
      bool serialStream;
      bool km271Stream;
  } s_telnetIF;

extern ESPTelnet telnet;
extern s_telnetIF telnetIF;


void setupTelnet();
void cyclicTelnet();
void telnetShell();