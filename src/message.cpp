#include "esp_log.h"
#include <HTTPClient.h>
#include <basics.h>
#include <km271.h>
#include <message.h>
#include <telnet.h>

/* D E C L A R A T I O N S ****************************************************/
#define MSG_BUF_SIZE 1024 // buffer size for messaging

#define HEAP_CHECK_INTERVAL 10000 // check every x seconds
#define HEAP_LOW_PERCENTAGE 10    // min value for free heap in Percentage
#define HEAP_SAMPLE_COUNT 3       // sample count for heap leak

// Heap check variables
static uint32_t totalHeap = 0;
static uint32_t heapSamples[HEAP_SAMPLE_COUNT];
static int sampleIndex = 0;
static char pushoverBuffer[MSG_BUF_SIZE]; // Buffer for Pushover messages
static const char *TAG = "MSG";    // LOG TAG
s_logdata logData;
static HTTPClient http;
 
static muTimer pushoverSendTimer = muTimer();
static muTimer checkHeapTimer = muTimer();
static muTimer mainTimer = muTimer();           // timer for cyclic info
static muTimer cyclicSendStatTimer = muTimer(); // timer to send periodic km271 messages
static muTimer cyclicSendCfgTimer = muTimer();  // timer to send periodic km271 messages

/**
 * *******************************************************************
 * @brief   Function to initialize heap monitoring
 * @param   none
 * @return  none
 * *******************************************************************/
void initHeapMonitoring() {
  totalHeap = esp_get_free_heap_size(); // Get total available heap memory

  // Initialize the ring buffer with the current heap values
  for (int i = 0; i < HEAP_SAMPLE_COUNT; i++) {
    heapSamples[i] = totalHeap;
  }
}

/**
 * *******************************************************************
 * @brief   Function to calculate the moving average of the heap
 * @param   none
 * @return  none
 * *******************************************************************/
size_t getAverageHeap() {
  size_t sum = 0;
  for (int i = 0; i < HEAP_SAMPLE_COUNT; i++) {
    sum += heapSamples[i];
  }
  return sum / HEAP_SAMPLE_COUNT;
}

/**
 * *******************************************************************
 * @brief   Function to monitor the heap status
 * @param   none
 * @return  none
 * *******************************************************************/
void checkHeapStatus() {
  static int filledSamples = 0; // Tracks how many samples have been collected

  // Check if totalHeap has been initialized properly to avoid division by zero
  if (totalHeap == 0) {
    MY_LOGW(TAG, "Error: totalHeap is 0, make sure initHeapMonitoring() was called.");
    return;
  }

  // Get the current free heap
  uint32_t currentHeap = esp_get_free_heap_size();

  // Add the current value to the ring buffer
  heapSamples[sampleIndex] = currentHeap;
  sampleIndex = (sampleIndex + 1) % HEAP_SAMPLE_COUNT;

  // Track the number of samples until the buffer is fully filled at least once
  if (filledSamples < HEAP_SAMPLE_COUNT) {
    filledSamples++;
    return; // Do not perform checks until we have enough samples
  }

  // Calculate the average of the last values
  size_t averageHeap = getAverageHeap();

  // Check if the free heap is below 10% of the total heap
  if ((averageHeap * 100 / totalHeap) < HEAP_LOW_PERCENTAGE) {
    MY_LOGW(TAG, "Warning: Heap memory below %i %%!", HEAP_LOW_PERCENTAGE);
    if (config.pushover.enable) {
      addPushoverMsg("Warning: free Heap memory is critical!");
    }
  }
}

/**
 * *******************************************************************
 * @brief   custom callback function for ESP_LOG messages
 * @param   format, args
 * @return  vprintf(format, args)
 * *******************************************************************/
int custom_vprintf(const char *format, va_list args) {

  // create a copy
  va_list args_copy;
  va_copy(args_copy, args);

  // add to buffer
  if (config.log.filter == LOG_FILTER_SYSTEM) {
    char testMessage[MAX_LOG_ENTRY];
    vsnprintf(testMessage, sizeof(testMessage), format, args_copy);
    addLogBuffer(testMessage);
  }

  // send to telnet stream
  if (telnetIF.serialStream) { // Provide to Telnet stream (if active)
    telnet.printf(format, args_copy);
    telnetShell();
  }

  // free copy of va_list
  va_end(args_copy);

  return vprintf(format, args);
}

/**
 * *******************************************************************
 * @brief   clear Logbuffer
 * @param   none
 * @return  none
 * *******************************************************************/
void clearLogBuffer() {
  logData.lastLine = 0;
  for (int i = 0; i < MAX_LOG_LINES; i++) {
    memset(logData.buffer[i], 0, sizeof(logData.buffer[i]));
  }
}

/**
 * *******************************************************************
 * @brief   add new entry to LogBuffer
 * @param   none
 * @return  none
 * *******************************************************************/
void addLogBuffer(const char *message) {
  if (strlen(message) != 0) {
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

  esp_log_level_set("*", MYLOG_LEVEL);  // set log level
  esp_log_set_vprintf(&custom_vprintf); // set custom vprintf callback function

  // Enable serial port
  Serial.begin(115200);

  initHeapMonitoring();
}

/**
 * *******************************************************************
 * @brief   Message from KM271
 * @param   typ, desc, value
 * @return  none
 * *******************************************************************/
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value) {
  char newCfgTopic[256];
  char tmpMsg[256];

  switch (typ) {
  case KM_TYP_CONFIG: // config values from Logamatic

    // desc = subtopic / value = value
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/config/%s", config.mqtt.topic, desc);
      mqttPublish(newCfgTopic, value, config.mqtt.config_retain);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Config: %s = %s", desc, value);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Config: %s = %s", desc, value);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_STATUS: // status values from Logamatic

    // desc = subtopic / value = value
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/status/%s", config.mqtt.topic, desc);
      mqttPublish(newCfgTopic, value, false);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Status: %s = %s", desc, value);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Status: %s = %s", desc, value);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_SENSOR: // sensor values from optional OneWire sensor

    // desc = subtopic / value = value
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/sensor/%s", config.mqtt.topic, desc);
      mqttPublish(newCfgTopic, value, false);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Sensor: %s = %s", desc, value);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_VALUES) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Sensor: %s = %s", desc, value);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_ALARM: // unacknowledged alarm message from Logamatic

    // desc = subtopic / value = value
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
      mqttPublish(newCfgTopic, value, false);
    }
    if (config.pushover.enable && (config.pushover.filter == MSG_FILTER_ALARM || config.pushover.filter == MSG_FILTER_INFO)) {
      snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
      addPushoverMsg(tmpMsg);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_ALARM) {
      snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_ALARM) {
      snprintf(tmpMsg, sizeof(tmpMsg), "%s = %s", desc, value);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_ALARM_H: // acknowledged alarm message from Logamatic (History)

    // desc = subtopic / value = value
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/alarm/%s", config.mqtt.topic, desc);
      mqttPublish(newCfgTopic, value, false);
    }
    break;

  case KM_TYP_DEBUG: // all messages from Logamatic - filtered by config.debug.filter

    // desc = description / value = NONE
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/debug_message", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_DEBUG) {
      snprintf(tmpMsg, sizeof(tmpMsg), "debug : %s", desc);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_DEBUG) {
      snprintf(tmpMsg, sizeof(tmpMsg), "debug : %s", desc);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_MESSAGE: // feedback messages from Logamatic

    // desc = description / value = NONE
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/message", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    if (config.pushover.enable && config.pushover.filter == MSG_FILTER_INFO) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Message: %s", desc);
      addPushoverMsg(tmpMsg);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_INFO) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Message : %s", desc);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_INFO) {
      snprintf(tmpMsg, sizeof(tmpMsg), "Message : %s", desc);
      telnet.println(tmpMsg);
    }
    break;

  case KM_TYP_INFO: // additional info message from Logamatic (only mqtt)

    // desc = description / value = NONE
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/info", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    break;

  case KM_TYP_KM_DEBUG: // additional debug message from Logamatic (only mqtt)

    // desc = description / value = NONE
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/debug", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    break;

  case KM_TYP_UNDEF_MSG: // unknown messages from Logamatic

    // desc = description / value = NONE
    if (config.mqtt.enable) {
      snprintf(newCfgTopic, sizeof(newCfgTopic), "%s/undefined_message", config.mqtt.topic);
      mqttPublish(newCfgTopic, desc, false);
    }
    if (config.log.enable && config.log.filter == LOG_FILTER_UNKNOWN) {
      snprintf(tmpMsg, sizeof(tmpMsg), "undef msg : %s", desc);
      addLogBuffer(tmpMsg);
    }
    if (telnetIF.km271Stream && config.log.filter == LOG_FILTER_UNKNOWN) {
      snprintf(tmpMsg, sizeof(tmpMsg), "undef msg : %s", desc);
      telnet.println(tmpMsg);
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
void addPushoverMsg(const char *str) {
  const char *bufferFullMsg = "send buffer full!";
  int remainingSpace = MSG_BUF_SIZE - strlen(pushoverBuffer) - 1;
  if (remainingSpace >= (strlen(str) + 1)) {
    snprintf(pushoverBuffer + strlen(pushoverBuffer), remainingSpace + 1, "%s\n", str);
  } else if (remainingSpace >= (strlen(bufferFullMsg) + 1)) {
    snprintf(pushoverBuffer + strlen(pushoverBuffer), remainingSpace + 1, "%s\n", bufferFullMsg);
  } else {
    MY_LOGE(TAG, "Pushover send buffer full!");
  }
}

/**
 * *******************************************************************
 * @brief   send Pushover Message
 * @param   str
 * @return  none
 * *******************************************************************/
void sendPushoverMsg() {
  JsonDocument notification;
  notification["token"] = config.pushover.token;   // required
  notification["user"] = config.pushover.user_key; // required
  notification["message"] = pushoverBuffer;        // required
  notification["title"] = config.wifi.hostname;    // optional

  // Create a buffer to serialize the JSON object
  char jsonBuffer[256 + MSG_BUF_SIZE];
  serializeJson(notification, jsonBuffer, sizeof(jsonBuffer));

  http.begin("http://api.pushover.net/1/messages.json"); // using http instead of https, because https needs >40kb of Heap memory!!!
  http.addHeader("Content-Type", "application/json");
  http.setReuse(false);
  http.POST(jsonBuffer);                             // Send the POST request with the JSON data
  http.end();                                        // Close the connection
  memset(pushoverBuffer, 0, sizeof(pushoverBuffer)); // clean message buffer
}

/**
 * *******************************************************************
 * @brief   check Debug Filter format
 * @param   input       given input string
 * @param   input_len   input string length
 * @param   msg         return string
 * @param   msg_len     return string length
 * @return  true:
 * *******************************************************************/
bool setDebugFilter(char *input, size_t input_len, char *errMsg, size_t errMsg_len) {
  if (input_len == 32) { // 32 characters: 11 Hex-values + 10 separators "_"
    // Iteration thru payload
    char payloadCpy[33];
    snprintf(payloadCpy, sizeof(payloadCpy), "%s", input);
    char *token = strtok(input, "_");
    size_t i = 0;
    while (token != NULL && i < 11) {
      // check, if the substring contains valid hex values
      size_t tokenLen = strlen(token);
      if (tokenLen != 2 || (!isxdigit(token[0]) && token[0] != 'X') || (!isxdigit(token[1]) && token[1] != 'X')) {
        // invalid hex values found
        strncpy(errMsg, "invalid hex parameter", errMsg_len);
        return false;
      }
      token = strtok(NULL, "_"); // next substring
      i++;
    }
    // check if all 11 hex values are found
    if (i == 11) {
      // everything seems to be valid - save
      snprintf(config.debug.filter, sizeof(config.debug.filter), "%s", payloadCpy);
      configSaveToFile();
      return true;
    } else {
      // not enough hex values found
      strncpy(errMsg, "not enough hex parameter", errMsg_len);
      return false;
    }
  } else {
    // invalid parameter length
    strncpy(errMsg, "invalid parameter size", errMsg_len);
    return false;
  }
}

/**
 * *******************************************************************
 * @brief   Message Cyclic Loop
 * @param   none
 * @return  none
 * *******************************************************************/
void messageCyclic() {
  // send Pushover message if something inside the buffer
  if (pushoverSendTimer.cycleTrigger(2000) && config.pushover.enable && (wifi.connected || eth.connected)) {
    if (strlen(pushoverBuffer)) {
      MY_LOGI(TAG, "Pushover Message sent");
      sendPushoverMsg();
    }
  }

  // check HEAP
  if (checkHeapTimer.cycleTrigger(10000)) {
    checkHeapStatus();
  }

  // send all km271 values from time to time
  if (!setupMode && config.mqtt.cyclicSendMin > 0 && mqttIsConnected()) {
    if (cyclicSendCfgTimer.delayOnTrigger(true, config.mqtt.cyclicSendMin * 60000)) {
      sendAllKmCfgValues();
    }
    if (cyclicSendStatTimer.delayOnTrigger(true, (config.mqtt.cyclicSendMin * 60000) + 2000)) {
      sendAllKmStatValues();
      cyclicSendCfgTimer.delayReset();
      cyclicSendStatTimer.delayReset();
    }
  }

  // send cyclic infos
  if (mainTimer.cycleTrigger(10000) && !setupMode && mqttIsConnected()) {
    sendWiFiInfo();
    sendETHInfo();
    sendKM271Info();
    sendKM271Debug();
    sendSysInfo();
  }
}
