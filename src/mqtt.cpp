#include <mqtt.h>
#include <basics.h>
#include <km271.h>
#include <WiFi.h>
#include <oilmeter.h>

// ======================================================
// declaration
// ======================================================
WiFiClient espClient;
PubSubClient mqtt_client(espClient);


/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt topic
 * @param   none
 * @return  none
 * *******************************************************************/
char newTopic[256];
const char * addTopic(const char *suffix){
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
  String payloadString = String((char*)payload);
  long intVal = payloadString.toInt();

  Serial.print("topic: ");
  Serial.println(topic);

  // ESP restarten auf Kommando
  if (strcmp (topic, addTopic("/cmd/restart")) == 0){
    mqtt_client.publish(addTopic("/message"), "restart requested!");
    delay(1000);
    ESP.restart();
  }
  // set date and time
  else if (strcmp (topic, addTopic("/setvalue/setdatetime")) == 0){
    Serial.println("cmd set date time");
    km271SetDateTime();
  }
  // set oilmeter
  else if (strcmp (topic, addTopic("/setvalue/oilcounter")) == 0){
    Serial.println("cmd setvalue oilcounter");
    cmdSetOilmeter(intVal);
  }
  // HK1 Betriebsart
  else if (strcmp (topic, addTopic("/setvalue/hk1_betriebsart")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK1_BA, intVal);
  }
  // HK2 Betriebsart
  else if (strcmp (topic, addTopic("/setvalue/hk2_betriebsart")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK2_BA, intVal);
  }
  // HK1 Programm
  else if (strcmp (topic, addTopic("/setvalue/hk1_programm")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK1_PROGRAMM, intVal);
  }
  // HK2 Programm
  else if (strcmp (topic, addTopic("/setvalue/hk2_programm")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK2_PROGRAMM, intVal);
  }
  // HK1 Auslegung
  else if (strcmp (topic, addTopic("/setvalue/hk1_auslegung")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK1_AUSLEGUNG, intVal);
  }
  // HK2 Auslegung
  else if (strcmp (topic, addTopic("/setvalue/hk2_auslegung")) == 0){  
    km271sendCmd(KM271_SENDCMD_HK2_AUSLEGUNG, intVal);
  }
  // HK1 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic("/setvalue/hk1_aussenhalt_ab")) == 0){
    km271sendCmd(KM271_SENDCMD_HK1_AUSSENHALT, intVal);
  }
  // HK2 Aussenhalt-Ab Temperatur
  else if (strcmp (topic, addTopic("/setvalue/hk2_aussenhalt_ab")) == 0){
    km271sendCmd(KM271_SENDCMD_HK2_AUSSENHALT, intVal);
  }  
  // WW Betriebsart
  else if (strcmp (topic, addTopic("/setvalue/ww_betriebsart")) == 0){
    km271sendCmd(KM271_SENDCMD_WW_BA, intVal);
  }
  // Sommer-Ab Temperatur
  else if (strcmp (topic, addTopic("/setvalue/sommer_ab")) == 0){
    km271sendCmd(KM271_SENDCMD_SOMMER_AB, intVal);
  }  
  // Frost-Ab Temperatur
  else if (strcmp (topic, addTopic("/setvalue/frost_ab")) == 0){
    km271sendCmd(KM271_SENDCMD_FROST_AB, intVal);
  } 
  // WW-Temperatur
  else if (strcmp (topic, addTopic("/setvalue/ww_soll")) == 0){
    km271sendCmd(KM271_SENDCMD_WW_SOLL, intVal);
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

