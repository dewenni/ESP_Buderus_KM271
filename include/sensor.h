#pragma once

/* I N C L U D E S ****************************************************/
#include <MycilaDS18.h>
#include <basics.h>
#include <config.h>
#include <mqtt.h>

/* D E C L A R A T I O N S ****************************************************/
struct s_sensor {
  float ch1_temp = -127;
  float ch2_temp = -127;
};
extern s_sensor sensor;

/* P R O T O T Y P E S ********************************************************/
void setupSensor();
void cyclicSensor();