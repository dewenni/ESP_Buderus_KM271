#include <message.h>
#include <sensor.h>

/* D E C L A R A T I O N S ****************************************************/
s_sensor sensor; // global Sensor Informations

Mycila::DS18 sensor1;
Mycila::DS18 sensor2;

#define REFRESH_TIME 10000
static  muTimer readTimer = muTimer(); // timer to refresh values

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt SENSOR topic
 * @param   suffix that should be add to the static part of the topic
 * @return  pointer to result topic string
 * *******************************************************************/
const char *addSensorTopic(const char *suffix) {
  static char sensTopic[256];
  snprintf(sensTopic, sizeof(sensTopic), "%s/sensor/%s", config.mqtt.topic, suffix);
  return sensTopic;
}

/**
 * *******************************************************************
 * @brief   Basic Setup for Sensor components
 * @param   none
 * @return  none
 * *******************************************************************/
void setupSensor(void) {
  if (config.sensor.ch1_enable) {
    sensor1.begin(config.sensor.ch1_gpio);

    sensor1.listen([](float temperature, bool changed) {
      sensor.ch1_temp = temperature;
      char topic1[32];
      replace_whitespace(config.sensor.ch1_name, topic1, sizeof(topic1));
      km271Msg(KM_TYP_SENSOR, topic1, floatToString(sensor.ch1_temp));
    });
  }
  if (config.sensor.ch2_enable) {
    sensor2.begin(config.sensor.ch2_gpio);
    sensor2.listen([](float temperature, bool changed) {
      sensor.ch2_temp = temperature;
      char topic2[32];
      replace_whitespace(config.sensor.ch2_name, topic2, sizeof(topic2));
      km271Msg(KM_TYP_SENSOR, topic2, floatToString(sensor.ch2_temp));
    });
  }
}

/**
 * *******************************************************************
 * @brief   Cyclic function for Sensor components
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicSensor(void) {
  // check temperatures at intervals
  if (readTimer.cycleTrigger(REFRESH_TIME)) {

    if (config.sensor.ch1_enable) {
      if (!sensor1.read()){
        sensor.ch1_temp = -127;
      }
    }

    if (config.sensor.ch2_enable) {
      if (!sensor2.read()) {
        sensor.ch2_temp = -127;
      }
    }
  }
}