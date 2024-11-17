#include <message.h>
#include <sensor.h>

/* D E C L A R A T I O N S ****************************************************/
s_sensor sensor; // global Sensor Informations

// Setup a OneWire instance to communicate with any OneWire devices
OneWire oneWire1;
OneWire oneWire2;

// Pass OneWire reference to Dallas Temperature
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);

#define REFRESH_TIME 10000
muTimer readTimer = muTimer(); // timer to refresh values
float sens1_old = 0.0;
float sens2_old = 0.0;

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt SENSOR topic
 * @param   suffix that shoould be add to the static part of the topic
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
    oneWire1.begin(config.sensor.ch1_gpio);
  }
  if (config.sensor.ch2_enable) {
    oneWire2.begin(config.sensor.ch2_gpio);
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
      sensor1.requestTemperatures();
      sensor.ch1_temp = sensor1.getTempCByIndex(0);
      if (sensor.ch1_temp != sens1_old) {
        sens1_old = sensor.ch1_temp;
        char topic1[32];
        replace_whitespace(config.sensor.ch1_name, topic1, sizeof(topic1));
        km271Msg(KM_TYP_SENSOR, topic1, floatToString(sensor.ch1_temp));
      }
    }

    if (config.sensor.ch2_enable) {
      sensor2.requestTemperatures();
      sensor.ch2_temp = sensor2.getTempCByIndex(0);
      if (sensor.ch2_temp != sens2_old) {
        sens2_old = sensor.ch2_temp;
        char topic2[32];
        replace_whitespace(config.sensor.ch2_name, topic2, sizeof(topic2));
        km271Msg(KM_TYP_SENSOR, topic2, floatToString(sensor.ch2_temp));
      }
    }
  }
}