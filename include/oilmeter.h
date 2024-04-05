#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <config.h>

/* D E C L A R A T I O N S ****************************************************/

// data struct for Flashstorage
struct {
  long oilcounter = 0;
} data;

/* P R O T O T Y P E S ********************************************************/
void sendOilmeter();
void cmdSetOilmeter(long setvalue);
void setupOilmeter();
void cyclicOilmeter();
void cmdStoreOilmeter();
long getOilmeter();