#pragma once
#include "ESPTelnet.h"

// data struct for Telnet interface
typedef struct {
      bool serialStream;
      bool km271Stream;
  } s_telnetIF;

const int MAX_PAR = 3;
const int MAX_CHAR = 64;

struct Command
{
  const char *name;
  void (*function)(char param[MAX_PAR][MAX_CHAR]);
  const char *description;
  const char *parameters;
};

extern ESPTelnet telnet;
extern s_telnetIF telnetIF;

void setupTelnet();
void cyclicTelnet();
void telnetShell();