#include <mqtt.h>
#include <basics.h>
#include <km271.h>
#include <WiFi.h>
#include <oilmeter.h>


/* P R O T O T Y P E S ********************************************************/  
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_reconnect();

/* D E C L A R A T I O N S ****************************************************/  
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
s_mqtt_cmds mqttCmd;  // exts that are used as topics for KM271 commands


/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
const char * addTopic(const char *suffix){
  static char newTopic[256];
  strcpy(newTopic, MQTT_TOPIC);
  strcat(newTopic, suffix);
  return newTopic;
}


/**
 * *******************************************************************
 * @brief   MQTT callback function
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';

  long intVal = atoi((char*)payload);
  float floatVal = atoff((char*)payload);

  Serial.print("topic: ");
  Serial.println(topic);

  // ESP restarten auf Kommando
  if (strcmp (topic, addTopic(mqttCmd.RESTART[LANG])) == 0){
    mqtt_client.publish(addTopic("/message"), "restart requested!");
    delay(1000);
    ESP.restart();
  }
  // set date and time
  else if (strcmp (topic, addTopic(mqttCmd.DATETIME[LANG])) == 0){
    Serial.println("cmd set date time");
    km271SetDateTimeNTP();
  }
  // set oilmeter
  else if (strcmp (topic, addTopic(mqttCmd.OILCNT[LANG])) == 0){
    Serial.println("cmd setvalue oilcounter");
    cmdSetOilmeter(intVal);
  }
  // HK1 Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.HC1_OPMODE[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, intVal);
  }
  // HK2 Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.HC2_OPMODE[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, intVal);
  }
  // HK1 Programm
  else if (strcmp (topic, addTopic(mqttCmd.HC1_PRG[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, intVal);
  }
  // HK2 Programm
  else if (strcmp (topic, addTopic(mqttCmd.HC2_PRG[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, intVal);
  }
  // HK1 Auslegung
  else if (strcmp (topic, addTopic(mqttCmd.HC1_INTERPRET[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, intVal);
  }
  // HK2 Auslegung
  else if (strcmp (topic, addTopic(mqttCmd.HC2_INTERPRET[LANG])) == 0){  
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_SWITCH_OFF_THRESHOLD[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_SWITCH_OFF_THRESHOLD[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, intVal);
  }  
  // HK1 Tag-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_DAY_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_DAY_SETPOINT, floatVal);
  }  
  // HK2 Tag-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_DAY_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_DAY_SETPOINT, floatVal);
  } 
  // HK1 Nacht-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_NIGHT_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_NIGHT_SETPOINT, floatVal);
  }  
  // HK2 Nacht-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_NIGHT_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_NIGHT_SETPOINT, floatVal);
  }
  // HK1 Ferien-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC1_HOLIDAY_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, floatVal);
  }  
  // HK2 Ferien-Soll Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.HC2_HOLIDAY_SETPOINT[LANG])) == 0){
    km271sendCmdFlt(KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, floatVal);
  } 
  // WW Betriebsart
  else if (strcmp (topic, addTopic(mqttCmd.WW_OPMODE[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, intVal);
  }
  // Sommer-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.SUMMER[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_SUMMER, intVal);
  }  
  // Frost-Ab Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.FROST[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_FROST, intVal);
  } 
  // WW-Temperatur
  else if (strcmp (topic, addTopic(mqttCmd.WW_SETPOINT[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, intVal);
  } 
  // HK1 Ferien Tage
  else if (strcmp (topic, addTopic(mqttCmd.HC1_HOLIDAYS[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, intVal);
  }  
  // HK2 Ferien Tage
  else if (strcmp (topic, addTopic(mqttCmd.HC2_HOLIDAYS[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, intVal);
  } 
  // WW Pump Cycles
  else if (strcmp (topic, addTopic(mqttCmd.WW_PUMP_CYCLES[LANG])) == 0){
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, intVal);
  } 
}


/**
 * *******************************************************************
 * @brief   Check MQTT connection and automatic reconnect
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttCyclic(){
    mqtt_client.loop();
    
    const char* willTopic = addTopic("/status");
    const char* willMsg = "offline";
    int mqtt_retry = 0;
    bool res;
    
    if (!mqtt_client.connected() && (WiFi.status() == WL_CONNECTED)) {
        while (!mqtt_client.connected() && mqtt_retry < 5 && WiFi.status() == WL_CONNECTED) {
            mqtt_retry++;
            Serial.println("MQTT not connected, reconnect...");
            res = mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PW, willTopic, 0, 1, willMsg);
            if (!res) {
                Serial.print("failed, rc=");
                Serial.print(mqtt_client.state());
                Serial.println(", retrying");
                delay(MQTT_RECONNECT);
            } else {
                Serial.println("MQTT connected");
                // Once connected, publish an announcement...
                sendWiFiInfo();
                // ... and resubscribe
                mqtt_client.subscribe(addTopic("/cmd/#"));
                mqtt_client.subscribe(addTopic("/setvalue/#"));
            }          
        }
        if(mqtt_retry >= 5){
          Serial.print("MQTT connection not possible, rebooting...");
          storeData(); // store Data before reboot
          delay(500);
          ESP.restart();
        }
    } 
}

/**
 * *******************************************************************
 * @brief   Basic MQTT setup
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttSetup(){
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(mqttCallback);
}


/**
 * *******************************************************************
 * @brief   MQTT Publish function for external use
 * @param   none
 * @return  none
 * *******************************************************************/
void mqttPublish(const char* sendtopic, const char* payload, boolean retained){
  mqtt_client.publish(sendtopic, payload, retained);
}

