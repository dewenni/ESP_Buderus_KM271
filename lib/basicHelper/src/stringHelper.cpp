#include "stringHelper.h"

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as int8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* int8ToString(int8_t value){
  static char ret_str[5];
  snprintf(ret_str, sizeof(ret_str), "%i", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint8_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint8ToString(uint8_t value){
  static char ret_str[5];
  snprintf(ret_str, sizeof(ret_str), "%u", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as uint16_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint16ToString(uint16_t value){
  static char ret_str[10];
  snprintf(ret_str, sizeof(ret_str), "%u", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @details because the format specifier %llu is not supported by any 
 *          platforms, we need to convert it this way
 * @param   value as uint64_t
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* uint64ToString(uint64_t value){
  static char ret_str[21];
  char* ptr = ret_str;
  int num_digits = 0;
  do {
    *ptr++ = (value % 10) + '0';
    value /= 10;
    num_digits++;
  } while (value != 0);
  *ptr-- = '\0';
  char* start_ptr = ret_str;
  char tmp_char;
  while (start_ptr < ptr) {
    tmp_char = *start_ptr;
    *start_ptr++ = *ptr;
    *ptr-- = tmp_char;
  }
  return ret_str;
}
/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as float
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* floatToString(float value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.1f", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer - with 3 digits
 * @param   value as float
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* floatToString4(float value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.4f", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   create String from integer
 * @param   value as double
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* doubleToString(double value){
  static char ret_str[64];
  snprintf(ret_str, sizeof(ret_str), "%.1lf", value);
  return ret_str;
}

/**
 * *******************************************************************
 * @brief   safe strcat function
 * @param   dest string destination
 * @param   src string source
 * @param   dest_size destination size
 * @return  none
 * *******************************************************************/
char *strcat_safe(char *dest, const char *src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        // not enough space
        return NULL;
    }   
    strcat(dest, src);
    return dest;
}

/**
 * *******************************************************************
 * @brief   convert String to BOOL if String contains true/false
 * @param   str e
 * @return  true/false
 * *******************************************************************/
bool stringToBool(const char* str) {
    if (strcmp(str, "true") == 0) {
        return true;
    }
    return false;
}


/**
 * *******************************************************************
 * @brief   create date and time as String
 * @param   none
 * @return  pointer to date and time String (local static memory)
 * *******************************************************************/
const char * getDateTimeString() {
  /* ---------------- INFO ---------------------------------
  dti.tm_year + 1900  // years since 1900
  dti.tm_mon + 1      // January = 0 (!)
  dti.tm_mday         // day of month
  dti.tm_hour         // hours since midnight  0-23
  dti.tm_min          // minutes after the hour  0-59
  dti.tm_sec          // seconds after the minute  0-61*
  dti.tm_wday         // days since Sunday 0-6
  dti.tm_isdst        // Daylight Saving Time flag
  --------------------------------------------------------- */
  static char dateTimeInfo[74]={'\0'};      // Date and time info String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%02d.%02d.%02d - %02i:%02i:%02i", dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec);
  return dateTimeInfo;
}

/**
 * *******************************************************************
 * @brief   create date  String
 * @param   none
 * @return  pointer to date String (local static memory)
 * *******************************************************************/
const char * getDateString() {
  static char dateInfo[74]={'\0'};          // Date String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(dateInfo, sizeof(dateInfo), "%02d.%02d.%02d", dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900));
  return dateInfo;
}
/**
 * *******************************************************************
 * @brief   create date  String
 * @param   none
 * @return  pointer to date String (local static memory)
 * *******************************************************************/
const char * getDateStringWeb() {
  static char dateInfo[74]={'\0'};          // Date String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(dateInfo, sizeof(dateInfo), "%02d-%02d-%02d", (dti.tm_year + 1900), (dti.tm_mon + 1), dti.tm_mday);
  return dateInfo;
}


/**
 * *******************************************************************
 * @brief   create time as String
 * @param   none
 * @return  pointer to time String (local static memory)
 * *******************************************************************/
const char * getTimeString() {
  static char timeInfo[74]={'\0'};          // Date and time info String
  time_t now;                               // this is the epoch
  tm dti;                                   // the structure tm holds time information in a more convient way
  time(&now);                               // read the current time
  localtime_r(&now, &dti);                  // update the structure tm with the current time
  snprintf(timeInfo, sizeof(timeInfo), "%02i:%02i:%02i", dti.tm_hour, dti.tm_min, dti.tm_sec);
  return timeInfo;
}