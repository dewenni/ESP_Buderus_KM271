#include <language.h>
#include <webUI.h>
#include <webUIhelper.h>

/**
 * *******************************************************************
 * @brief   format build date/time information
 * @param   date input string
 * @return  hash value
 * *******************************************************************/
void getBuildDateTime(char *formatted_date) {
  // Monatsnamen für die Umwandlung in Zahlen
  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char month_text[4] = {0};
  int day, year;
  // Extrahiere den Monat, Tag und Jahr aus dem __DATE__ String
  sscanf(__DATE__, "%s %d %d", month_text, &day, &year);

  // Finde den Monat im Array und konvertiere ihn in eine Zahl
  int month = 0;
  for (int i = 0; i < 12; i++) {
    if (strcmp(month_text, months[i]) == 0) {
      month = i + 1;
      break;
    }
  }

  // Format das Datum als "DD-MM-YYYY"
  sprintf(formatted_date, "%02d.%02d.%d - %s", day, month, year, __TIME__);
}

/**
 * *******************************************************************
 * @brief   create ON/OFF String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char *onOffString(uint8_t value) {
  static char ret_str[64];
  if (value != 0)
    snprintf(ret_str, sizeof(ret_str), "%s", WEB_TXT::ON[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", WEB_TXT::OFF[config.lang]);

  return ret_str;
}
/**
 * *******************************************************************
 * @brief   create ERROR/OK String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char *errOkString(uint8_t value) {
  static char ret_str[64];
  if (value != 0)
    snprintf(ret_str, sizeof(ret_str), "%s", WEB_TXT::ERROR[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", WEB_TXT::OK[config.lang]);

  return ret_str;
}

/**
 * *******************************************************************
 * @brief   update Logger output
 * @param   none
 * @return  none
 * *******************************************************************/
static int logLine, logIdx = 0;
static bool logReadActive = false;
JsonDocument jsonLog;

bool webLogRefreshActive() { return logReadActive; }

void webReadLogBuffer() {
  logReadActive = true;
  logLine = 0;
  logIdx = 0;
}

void webReadLogBufferCyclic() {

  jsonLog.clear();
  jsonLog["type"] = "logger";
  jsonLog["cmd"] = "add_log";
  JsonArray entryArray = jsonLog["entry"].to<JsonArray>();

  while (logReadActive) {

    if (logLine == 0 && logData.lastLine == 0) {
      // log empty
      logReadActive = false;
      return;
    }
    if (config.log.order == 1) {
      logIdx = (logData.lastLine - logLine - 1) % MAX_LOG_LINES;
    } else {
      if (logData.buffer[logData.lastLine][0] == '\0') {
        // buffer is not full - start reading at element index 0
        logIdx = logLine % MAX_LOG_LINES;
      } else {
        // buffer is full - start reading at element index "logData.lastLine"
        logIdx = (logData.lastLine + logLine) % MAX_LOG_LINES;
      }
    }
    if (logIdx < 0) {
      logIdx += MAX_LOG_LINES;
    }
    if (logIdx >= MAX_LOG_LINES) {
      logIdx = 0;
    }
    if (logLine == MAX_LOG_LINES - 1) {
      // end
      updateWebJSON(jsonLog);
      logReadActive = false;
      return;
    } else {
      if (logData.buffer[logIdx][0] != '\0') {
        entryArray.add(logData.buffer[logIdx]);
        logLine++;
      } else {
        // no more entries
        logReadActive = false;
        updateWebJSON(jsonLog);
        return;
      }
    }
  }
}