#pragma once

typedef enum {
  MSG_TYP_INFO,                                                       
  MSG_TYP_WARNING,                                                        
  MSG_TYP_ERROR            
} e_msgInfoType; 

typedef enum {
  MSG_SRC_SYSTEM,                                                       
  MSG_SRC_KM271                                                                  
} e_msgSrcType; 

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

extern char emailStatus[128];

void msgHandler(e_msgInfoType type, e_msgSrcType src, const char *message);
void addPushoverMsg(const char *str);
void messageSetup();
void messageCyclic();
void km271Msg(e_kmMsgTyp typ, const char *desc, const char *value);
char* getLogBuffer();
void clearLogBuffer();