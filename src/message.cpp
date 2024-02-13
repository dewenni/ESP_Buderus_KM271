#include <message.h>
#include <basics.h>
#include <km271.h>
#include <HTTPClient.h>

/* D E C L A R A T I O N S ****************************************************/  
#define MSG_BUF_SIZE 1024            // buffer size for messaging
char pushoverBuffer[MSG_BUF_SIZE];   // Buffer for Pushover messages

muTimer pushoverSendTimer = muTimer();
HTTPClient http;

/**
 * *******************************************************************
 * @brief   clear Logbuffer
 * @param   none 
 * @return  none
 * *******************************************************************/
void clearLogBuffer() {
  logData.lastLine = 0;
  for (int i = 0; i < MAX_LOG_LINES; i++){
    memset(logData.buffer[i], 0, sizeof(logData.buffer[i]));
  }
}

/**
 * *******************************************************************
 * @brief   add new entry to LogBuffer
 * @param   none 
 * @return  none
 * *******************************************************************/
void addLogBuffer(const char *message){  
  if (strlen(message)!=0){
    snprintf(logData.buffer[logData.lastLine], sizeof(logData.buffer[logData.lastLine]), "[%s]  %s", getDateTimeString(), message);
    logData.lastLine = (logData.lastLine + 1) % MAX_LOG_LINES; // update the lastLine index in a circular manner
  }
}

/**
 * *******************************************************************
 * @brief   Setup for Telegram bot
 * @param   none
 * @return  none
 * *******************************************************************/
void messageSetup() {
  memset(pushoverBuffer, 0, sizeof(pushoverBuffer));
}

/**
 * *******************************************************************
 * @brief   Message from KM271
 * @param   typ, desc, value
 * @return  none
 * *******************************************************************/
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value){
  char newCfgTopic[256];
  char tmpMsg[256];

  switch (typ)
  {
    case KM_TYP_CONFIG: // config values from Logamatic
      
      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/config/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, config.mqtt.config_retain);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_VALUES){
        snprintf(tmpMsg, sizeof(tmpMsg), "Config: %s = %s", desc, value);
        addLogBuffer(tmpMsg);
      }
      break;
    
    case KM_TYP_STATUS: // status values from Logamatic

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/status/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_VALUES){
        snprintf(tmpMsg, sizeof(tmpMsg), "Status: %s = %s", desc, value);
        addLogBuffer(tmpMsg);
      }
    break;

    case KM_TYP_SENSOR: // sensor values from optional OneWire sensor

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/sensor/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_VALUES){
        snprintf(tmpMsg, sizeof(tmpMsg), "Sensor: %s = %s", desc, value);
        addLogBuffer(tmpMsg);
      }

    break;

    case KM_TYP_ALARM:  // unacknowledged alarm message from Logamatic

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
      if (config.pushover.enable && (config.pushover.filter==MSG_FILTER_ALARM || config.pushover.filter==MSG_FILTER_INFO)){
        snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
        addPushoverMsg(tmpMsg);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_ALARM){
        snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
        addLogBuffer(tmpMsg);
      }
    break;

    case KM_TYP_ALARM_H:  // acknowledged alarm message from Logamatic (History)

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
    break;

    case KM_TYP_DEBUG:  // all messages from Logamatic - filtered by config.debug.filter

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/debug_message", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_DEBUG){
        snprintf(tmpMsg, sizeof(tmpMsg), "debug : %s", desc);
        addLogBuffer(tmpMsg);
      }
    break;

    case KM_TYP_MESSAGE:  // feedback messages from Logamatic

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/message", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
      if (config.pushover.enable && config.pushover.filter==MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Message: %s", desc);
        addPushoverMsg(tmpMsg);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Message : %s", desc);
        addLogBuffer(tmpMsg);
      }     
    break;

    case KM_TYP_INFO: // additional info message from Logamatic (only mqtt)

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/info", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
    break;

    case KM_TYP_UNDEF_MSG:  // unknown messages from Logamatic

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/undefined_message", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
      if (config.log.enable && config.log.filter==LOG_FILTER_UNKNOWN){
        snprintf(tmpMsg, sizeof(tmpMsg), "undef msg : %s", desc);
        addLogBuffer(tmpMsg);
      }  
      break;

    default:
      break;
  }
}

/**
 * *******************************************************************
 * @brief   add a message to Pushover Send-Buffer
 * @param   str
 * @return  none
 * *******************************************************************/
void addPushoverMsg(const char* str) {
  const char* bufferFullMsg = "send buffer full!";
  int remainingSpace = MSG_BUF_SIZE - strlen(pushoverBuffer) - 1;
  if (remainingSpace >= (strlen(str) + 1)) {
    snprintf(pushoverBuffer + strlen(pushoverBuffer), remainingSpace + 1, "%s\n", str);
  } else if (remainingSpace >= (strlen(bufferFullMsg) + 1)) {
    snprintf(pushoverBuffer + strlen(pushoverBuffer), remainingSpace + 1, "%s\n", bufferFullMsg);
  } else {
    Serial.println("send buffer full!");
  }
}

/**
 * *******************************************************************
 * @brief   send Pushover Message
 * @param   str
 * @return  none
 * *******************************************************************/
void sendPushoverMsg() {
  DynamicJsonDocument notification(256 + MSG_BUF_SIZE);
  notification["token"] = config.pushover.token;    //required
  notification["user"] = config.pushover.user_key;  //required
  notification["message"] = pushoverBuffer;         //required
  notification["title"] = config.wifi.hostname;     //optional

  // Create a buffer to serialize the JSON object
  char jsonBuffer[256 + MSG_BUF_SIZE];
  serializeJson(notification, jsonBuffer, sizeof(jsonBuffer));

  http.begin("http://api.pushover.net/1/messages.json");  // using http instead of https, because https needs >40kb of Heap memory!!!
  http.addHeader("Content-Type", "application/json");
  http.setReuse(false);
  http.POST(jsonBuffer);                                  // Send the POST request with the JSON data
  http.end();                                             // Close the connection
  memset(pushoverBuffer, 0, sizeof(pushoverBuffer));      // clean message buffer
}

/**
 * *******************************************************************
 * @brief   Message Cyclic Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void messageCyclic(){ 
  // send Pushover message if something inside the buffer
  if (pushoverSendTimer.cycleTrigger(2000) && config.pushover.enable){
    if(strlen(pushoverBuffer)){
      Serial.println("Pushover Message sent");
      sendPushoverMsg();
    }
  }
}