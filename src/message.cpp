#include <message.h>
#include <basics.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <EMailSender.h>
#include <km271.h>
#include <HTTPClient.h>

/* D E C L A R A T I O N S ****************************************************/  
bool cmdSendMail = false;                   // local command to send mail
bool cmdSendTelegram = false;               // local command to send mail
char emailStatus[128];                      // status of sendmail
char telegramMsg[256];                      // Telegram message String
const int bufferSize = 1024;
char telegramBuffer[bufferSize];            // Buffer for Telegram messages
char emailBuffer[bufferSize];               // Buffer for Email messages
char pushoverBuffer[bufferSize];            // Buffer for Pushover messages

EMailSender::EMailMessage emailMessage;     // email message object
WiFiClientSecure telegram_client;
UniversalTelegramBot bot("", telegram_client);
muTimer telegramSendTimer = muTimer();
muTimer pushoverSendTimer = muTimer();
muTimer emailSendTimer = muTimer();
HTTPClient http;

/**
 * *******************************************************************
 * @brief   Setup for Telegram bot
 * @param   none
 * @return  none
 * *******************************************************************/
void messageSetup() {
  memset(telegramBuffer, 0, sizeof(telegramBuffer));
  memset(emailBuffer, 0, sizeof(emailBuffer));
  memset(pushoverBuffer, 0, sizeof(pushoverBuffer));
  telegram_client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
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
      if (config.telegram.enable && config.telegram.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Config: %s = %s", desc, value);
        addTelegramMsg(tmpMsg);
      }
      if (config.pushover.enable && config.pushover.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Config: %s = %s", desc, value);
        addPushoverMsg(tmpMsg);
      }
      break;
    
    case KM_TYP_STATUS: // status values from Logamatic

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/status/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
      if (config.telegram.enable && config.telegram.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Status: %s = %s", desc, value);
        addTelegramMsg(tmpMsg);
      }
      if (config.pushover.enable && config.pushover.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Status: %s = %s", desc, value);
        addPushoverMsg(tmpMsg);
      }
    break;

    case KM_TYP_ALARM:  // unacknowledged alarm message from Logamatic

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
      if (config.telegram.enable){
        snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
        addTelegramMsg(tmpMsg);
      }
      if (config.pushover.enable){
        snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
        addPushoverMsg(tmpMsg);
      }
      if (config.email.enable){
        snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
        addEmailMsg(tmpMsg);
      }
    break;

    case KM_TYP_ALARM_H:  // acknowledged alarm message from Logamatic (History)

      // desc = subtopic / value = value
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
        mqttPublish(newCfgTopic, value, false);
      }
    break;

    case KM_TYP_DEBUG:  // all messages from Logamatic

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/debug_message/", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
      if (config.telegram.enable && config.telegram.filter==MSG_FILTER_DEBUG ){
        snprintf(tmpMsg, sizeof(tmpMsg), "Debug: %s", desc);
        addTelegramMsg(tmpMsg);
      }
      if (config.pushover.enable && config.pushover.filter==MSG_FILTER_DEBUG ){
        snprintf(tmpMsg, sizeof(tmpMsg), "Debug: %s", desc);
        addPushoverMsg(tmpMsg);
      }
    break;

    case KM_TYP_MESSAGE:  // feedback messages from Logamatic

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/message/", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
      if (config.telegram.enable && config.telegram.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Message: %s", desc);
        addTelegramMsg(tmpMsg);
      }
      if (config.pushover.enable && config.pushover.filter>=MSG_FILTER_INFO){
        snprintf(tmpMsg, sizeof(tmpMsg), "Message: %s", desc);
        addPushoverMsg(tmpMsg);
      }
    break;

    case KM_TYP_INFO: // additional info  message from Logamatic

      // desc = description / value = NONE
      if (config.mqtt.enable){
        snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/info/", config.mqtt.topic);
        mqttPublish(newCfgTopic, desc, false);
      }
    break;

    case KM_TYP_UNDEF_MSG:  // unknown messages from Logamatic

    // desc = description / value = NONE
    if (config.mqtt.enable){
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/undefined_message/", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    if (config.telegram.enable && config.telegram.filter==MSG_FILTER_DEBUG ){
      snprintf(tmpMsg, sizeof(tmpMsg), "undefined message: %s = %s", desc);
      addTelegramMsg(tmpMsg);
    }
    if (config.pushover.enable && config.pushover.filter==MSG_FILTER_DEBUG ){
      snprintf(tmpMsg, sizeof(tmpMsg), "undefined message: %s = %s", desc);
      addPushoverMsg(tmpMsg);
    } 
    break;

    default:
      break;
  }
}


/**
 * *******************************************************************
 * @brief   add a message to Telegram Send-Buffer
 * @param   str
 * @return  none
 * *******************************************************************/
void addTelegramMsg(const char* str) {
  const char* bufferFullMsg = "send buffer full!";
  int remainingSpace = bufferSize - strlen(telegramBuffer) - 1;
  if (remainingSpace >= (strlen(str) + 1)) {
    snprintf(telegramBuffer + strlen(telegramBuffer), remainingSpace + 1, "%s\n", str);
  } else if (remainingSpace >= (strlen(bufferFullMsg) + 1)) {
    snprintf(telegramBuffer + strlen(telegramBuffer), remainingSpace + 1, "%s\n", bufferFullMsg);
  } else {
    Serial.println("send buffer full!");
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
  int remainingSpace = bufferSize - strlen(pushoverBuffer) - 1;
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
 * @brief   add a message to Email Send-Buffer
 * @param   str
 * @return  none
 * *******************************************************************/
void addEmailMsg(const char* str) {
  const char* bufferFullMsg = "send buffer full!";
  int remainingSpace = bufferSize - strlen(emailBuffer) - 1;
  if (remainingSpace >= (strlen(str) + 1)) {
    snprintf(emailBuffer + strlen(emailBuffer), remainingSpace + 1, "%s\n", str);
  } else if (remainingSpace >= (strlen(bufferFullMsg) + 1)) {
    snprintf(emailBuffer + strlen(emailBuffer), remainingSpace + 1, "%s\n", bufferFullMsg);
  } else {
    Serial.println("send buffer full!");
  }
}


/**
 * *******************************************************************
 * @brief   function to send Email Message
 * @param   msg
 * @return  none
 * *******************************************************************/
void sendMail()
{
  // send message constructor
  EMailSender emailSend(config.email.user , config.email.password, config.email.sender, "ESP_Buderus_KM271", config.email.server, config.email.port);

  // message options
  emailMessage.subject = "ESP_Buderus_KM271";
  emailMessage.mime = MIME_TEXT_PLAIN;

  // message response
  EMailSender::Response resp = emailSend.send(config.email.receiver, emailMessage);

  // debug & status of send message function
  snprintf(emailStatus, sizeof(emailStatus), resp.desc.c_str());
  Serial.println("Sending status: ");
  Serial.println(resp.status);
  Serial.println(resp.code);
  Serial.println(emailStatus);

  memset(emailBuffer, 0, sizeof(emailBuffer));

}

/**
 * *******************************************************************
 * @brief   send Pushover Message
 * @param   str
 * @return  none
 * *******************************************************************/
void sendPushoverMsg() {
  DynamicJsonDocument notification(256 + bufferSize);
  notification["token"] = config.pushover.token;    //required
  notification["user"] = config.pushover.user_key;  //required
  notification["message"] = pushoverBuffer;         //required
  notification["title"] = "ESP_Buderus_KM271";      //optional

  // Create a buffer to serialize the JSON object
  char jsonBuffer[256 + bufferSize];
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

  // send Email message if something inside the buffer
  if (emailSendTimer.cycleTrigger(10000)){
    if(strlen(emailBuffer)){
      Serial.println("Email Message sent");
      emailMessage.message = emailBuffer;
      sendMail();
    }
  }

  // send Telegram message if something inside the buffer
  if (telegramSendTimer.cycleTrigger(2500)){
    if(strlen(telegramBuffer)){
      bot.updateToken(config.telegram.token);
      Serial.println("Telegram Message:");
      Serial.println(bot.sendMessage(config.telegram.chat_id, telegramBuffer, ""));
      memset(telegramBuffer, 0, sizeof(telegramBuffer));
    }
  }

  // send Pushover message if something inside the buffer
  if (pushoverSendTimer.cycleTrigger(3500)){
    if(strlen(pushoverBuffer)){
      Serial.println("Pushover Message sent");
      sendPushoverMsg();
    }
  }

}