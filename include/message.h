#pragma once
#include <Arduino.h>

/* D E C L A R A T I O N S ****************************************************/
enum e_kmMsgTyp {
  KM_TYP_STATUS,    // Status value
  KM_TYP_CONFIG,    // Config Value
  KM_TYP_SENSOR,    // Sensor Value
  KM_TYP_ALARM,     // Alarm Message
  KM_TYP_ALARM_H,   // Alarm History Message
  KM_TYP_DEBUG,     // Debug Message
  KM_TYP_MESSAGE,   // feedback Message
  KM_TYP_INFO,      // Info Message
  KM_TYP_KM_DEBUG,  // KM271-Info Message
  KM_TYP_UNDEF_MSG, // undefined Message
};

#define MAX_LOG_LINES 200 // max log lines
#define MAX_LOG_ENTRY 128 // max length of one entry

#define MY_LOGE(tag, format, ...) esp_log_write(ESP_LOG_ERROR, tag, "E (APP-%s): " format "\n", tag, ##__VA_ARGS__)
#define MY_LOGI(tag, format, ...) esp_log_write(ESP_LOG_INFO, tag, "I (APP-%s): " format "\n", tag, ##__VA_ARGS__)
#define MY_LOGW(tag, format, ...) esp_log_write(ESP_LOG_WARN, tag, "W (APP-%s): " format "\n", tag, ##__VA_ARGS__)
#define MY_LOGD(tag, format, ...) esp_log_write(ESP_LOG_DEBUG, tag, "D (APP-%s): " format "\n", tag, ##__VA_ARGS__)

struct s_logdata {
  int lastLine;
  char buffer[MAX_LOG_LINES][MAX_LOG_ENTRY];
};

extern s_logdata logData;

/* P R O T O T Y P E S ********************************************************/
void addPushoverMsg(const char *str);
void messageSetup();
void messageCyclic();
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value);
void addLogBuffer(const char *message);
void clearLogBuffer();
bool setDebugFilter(char *input, size_t input_len, char *errMsg, size_t errMsg_len);
