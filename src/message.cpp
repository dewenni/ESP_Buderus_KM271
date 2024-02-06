#include <message.h>
#include <basics.h>
#include <km271.h>
#include <HTTPClient.h>

/* D E C L A R A T I O N S ****************************************************/  
#define MSG_BUF_SIZE 1024            // buffer size for messaging
char pushoverBuffer[MSG_BUF_SIZE];   // Buffer for Pushover messages
#define BUFFER_SIZE 10000            // buffer size for logging Buffer
char messageBuffer[BUFFER_SIZE];     // logBuffer

muTimer pushoverSendTimer = muTimer();
HTTPClient http;

/**
 * *******************************************************************
 * @brief   write message to logBuffer
 * @param   message
 * @return  none
 * *******************************************************************/
void logMessage(const char* message) {
  // Calculate length of message
  int messageLength = strlen(message);
  char dateTime[32];
  snprintf(dateTime, sizeof(dateTime), "%s > ", getDateTimeString());
  int dateTimeLength = strlen(dateTime);
  // Check if message fits in buffer, if not, remove old messages
  while (strlen(messageBuffer) + messageLength + dateTimeLength >= BUFFER_SIZE)
  {
    // Find position of first newline character
    char* newlinePos = strchr(messageBuffer, '\n');
    
    // If newlinePos is NULL, there are no newlines in the buffer, so clear the buffer
    if (newlinePos == NULL) {
      messageBuffer[0] = '\0';
      break;
    }

    // Move pointer to character after newline
    newlinePos++;

    // Calculate length of string after newline
    int remainingLength = strlen(newlinePos);

    // Shift remaining string to the beginning of the buffer
    memmove(messageBuffer, newlinePos, remainingLength + 1);
  }
  // Add "Msg : " prefix to message
  strcat(messageBuffer, dateTime);
  // Add message to buffer
  strcat(messageBuffer, message);
  // Add newline at the end of message
  strcat(messageBuffer, "\n");
}

/**
 * *******************************************************************
 * @brief   get pointer to logBuffer
 * @param   none
 * @return  char* to logBuffer
 * *******************************************************************/
char* getLogBuffer(){
  return messageBuffer;
}

/**
 * *******************************************************************
 * @brief   Setup for Telegram bot
 * @param   none
 * @return  none
 * *******************************************************************/
void messageSetup() {
  memset(pushoverBuffer, 0, sizeof(pushoverBuffer));
  memset(messageBuffer, 0, sizeof(messageBuffer));
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
        logMessage(tmpMsg);
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
  size_t jsonSize = serializeJson(notification, jsonBuffer, sizeof(jsonBuffer));

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