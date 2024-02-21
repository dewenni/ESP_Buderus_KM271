#pragma once
#include <Arduino.h>

/* D E C L A R A T I O N S ****************************************************/  
typedef enum {
  KM_TYP_STATUS,                                                           // Status value
  KM_TYP_CONFIG,                                                           // Config Value
  KM_TYP_SENSOR,                                                           // Sensor Value
  KM_TYP_ALARM,                                                            // Alarm Message
  KM_TYP_ALARM_H,                                                          // Alarm History Message
  KM_TYP_DEBUG,                                                            // Debug Message
  KM_TYP_MESSAGE,                                                          // feedback Message
  KM_TYP_INFO,                                                             // Info Message
  KM_TYP_UNDEF_MSG,                                                        // undefined Message
} e_kmMsgTyp;

#define MAX_LOG_LINES 50   // max log lines
#define MAX_LOG_ENTRY 128  // max length of one entry
typedef struct {
    int lastLine;
    char buffer[MAX_LOG_LINES][MAX_LOG_ENTRY];
} s_logdata;

extern s_logdata logData;

/* P R O T O T Y P E S ********************************************************/ 
void addPushoverMsg(const char *str);
void messageSetup();
void messageCyclic();
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value);
void addLogBuffer();
void clearLogBuffer();
bool setDebugFilter(char *input, size_t input_len, char *errMsg, size_t errMsg_len);