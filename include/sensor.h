#pragma once

/* I N C L U D E S ****************************************************/
#include <DallasTemperature.h>
#include <OneWire.h>
#include <basics.h>
#include <config.h>
#include <mqtt.h>

/* D E C L A R A T I O N S ****************************************************/
typedef struct {
  float ch1_temp;
  float ch2_temp;
} s_sensor;
extern s_sensor sensor;

/* P R O T O T Y P E S ********************************************************/
void setupSensor();
void cyclicSensor();