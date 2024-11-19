#pragma once

/* I N C L U D E S ****************************************************/
#include <km271.h>
#include <oilmeter.h>
#include <sensor.h>
#include <webUIhelper.h>

// time structure for burner runtime
struct timeComponents {
  int years;
  int days;
  int hours;
  int minutes;
};

/* P R O T O T Y P E S ********************************************************/
void updateAllElements();
void webUIupdates();
void updateGpioSettings();
void setOtaActive(bool active);
bool getOtaActive();