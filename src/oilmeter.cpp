//*****************************************************************************
// 
// Title      : optional handling of Oil Meter to measure oil consumtion
// hardware   : Braun HZ5DR / HZ5R / HZ5 (modified with reed contact)
// Remark     : if you dont want to use this, you can disable this in config.h
//*****************************************************************************
#include <oilmeter.h>
#include <mqtt.h>
#include <basics.h>
#include <EEPROM.h>

/* V A R I A B L E S ********************************************************/
int addr = 0;                             // start address for EEPROM
int writeCounter =0;                      // counter for write to EEPROM
bool reboot = true;                       // flag for reboot

#define DI_OIL_CNT 26                     // input for oilmeter sensor
#define DO_OIL_CNT 25                     // LED signal oilmeter trigger

#define OILTRIGGER_TIME 1000              // 1.000 = 1sec
#define OILCYCLICINFO_TIME 3600000        // 360.000 = 1 hour

muTimer oilTrigger = muTimer();           // timer for debouncing trigger input
muTimer oilCyclicInfo = muTimer();        // timer for cyclic information


/**
 * *******************************************************************
 * @brief   Publish actual Oilmeter values via MQTT
 * @param   none
 * @return  none
 * *******************************************************************/
void sendOilmeter() {
  // publish actual value
  mqttPublish(addTopic("/oilcounter"), String(data.oilcounter).c_str(), true);
}

/**
 * *******************************************************************
 * @brief   Set new value for Oilcounter and safe to EEPROM
 * @param   setvalue  new setvalue
 * @return  none
 * *******************************************************************/
void cmdSetOilmeter(long setvalue) {

  data.oilcounter = setvalue;
  cmdStoreOilmeter();
  
  String message = "oilcounter was set to: " + String(data.oilcounter);
  mqttPublish(addTopic("/message"), String(message).c_str(), false);

  sendOilmeter();
}

/**
 * *******************************************************************
 * @brief   store actual Oilcountervalue to EEPROM
 * @param   none
 * @return  none
 * *******************************************************************/
void cmdStoreOilmeter() {
  EEPROM.put(addr,data);
  EEPROM.commit();
  mqttPublish(addTopic("/message"), String("oilcounter stored!").c_str(), false);
}

/**
 * *******************************************************************
 * @brief   Basic Setup for Oilcounter components
 * @param   none
 * @return  none
 * *******************************************************************/
void setupOilmeter(){
  
  // a byte-array cache in RAM
  EEPROM.begin(sizeof(data));
  EEPROM.get(addr,data);
  
  Serial.print("restored value from Flash: ");
  Serial.println(data.oilcounter);
  String message = "oilcounter was set to: " + String(data.oilcounter);
  mqttPublish(addTopic("/message"), String(message).c_str(), false);

  // IO Setup
  pinMode(DI_OIL_CNT, INPUT_PULLUP);    // Trigger Input
  pinMode(DO_OIL_CNT, OUTPUT);          // Status LED

}

/**
 * *******************************************************************
 * @brief   Cyclic Oilcounter function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicOilmeter()
{
  digitalWrite(DO_OIL_CNT, digitalRead(DI_OIL_CNT));    // signal for every incomming impulse with Onboard LED

  // debounce input trigger with timer function
  bool statusTrigger = oilTrigger.delayOnOffTrigger(!digitalRead(DI_OIL_CNT), 0, OILTRIGGER_TIME) == 1;
  if (statusTrigger && !reboot) {
      data.oilcounter = data.oilcounter + 2;            // one impulse = 0,02 litre
      writeCounter++;                                   // increase counter for writing to EEPROM
      Serial.println(data.oilcounter);
      sendOilmeter();                                   // send new Countervalue via MQTT
  }

  if (writeCounter >= 100)                              // save water meter every 100 counts = 2 liter)
  {
    writeCounter = 0;
    cmdStoreOilmeter();                                 // store new value in flash
  }

  // send cyclic infos
  if (oilCyclicInfo.cycleTrigger(OILCYCLICINFO_TIME))   // send actual information every x seconds
  {
    sendOilmeter();                                     // send new Countervalue via MQTT
  }

  reboot = false;                                       // reset reboot flag
}

