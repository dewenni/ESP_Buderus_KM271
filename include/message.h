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

enum e_logTyp {
  SYSLOG,
  KMLOG,
};

#define MAX_LOG_LINES 200 // max log lines
#define MAX_LOG_ENTRY 128 // max length of one entry

struct s_logdata {
  int lastLine;
  char buffer[MAX_LOG_LINES][MAX_LOG_ENTRY];
};


/* P R O T O T Y P E S ********************************************************/
void addPushoverMsg(const char *str);
void messageSetup();
void messageCyclic();
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value);
void addLogBuffer(e_logTyp typ, const char *message);
void clearLogBuffer(e_logTyp typ);
bool setDebugFilter(char *input, size_t input_len, char *errMsg, size_t errMsg_len);
s_logdata *getLogBuffer(e_logTyp typ);