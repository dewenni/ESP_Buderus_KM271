#pragma once

/* I N C L U D E S ****************************************************/
#include <km271.h>
#include <oilmeter.h>
#include <sensor.h>
#include <webUIhelper.h>

// time structure for burner runtime
typedef struct TimeComponents {
  int years;
  int days;
  int hours;
  int minutes;
} timeComponents;

/* P R O T O T Y P E S ********************************************************/
void updateAllElements();
void webUIupdates();
