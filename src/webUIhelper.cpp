#include <language.h>
#include <webUI.h>
#include <webUIhelper.h>

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
e_logTyp logType;

void webClearLog() {
  if (logType == SYSLOG) {
    clearLogBuffer(SYSLOG);
  } else {
    clearLogBuffer(KMLOG);
  }
}

void webSetLogType(e_logTyp typ) { logType = typ; };

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

  s_logdata *pLogData;
  if (logType == SYSLOG) {
    pLogData = getLogBuffer(SYSLOG);
  } else {
    pLogData = getLogBuffer(KMLOG);
  }

  while (logReadActive) {

    if (logLine == 0 && pLogData->lastLine == 0) {
      // log empty
      logReadActive = false;
      return;
    }
    if (config.log.order == 1) {
      logIdx = (pLogData->lastLine - logLine - 1) % MAX_LOG_LINES;
    } else {
      if (pLogData->buffer[pLogData->lastLine][0] == '\0') {
        // buffer is not full - start reading at element index 0
        logIdx = logLine % MAX_LOG_LINES;
      } else {
        // buffer is full - start reading at element index "pLogData->lastLine"
        logIdx = (pLogData->lastLine + logLine) % MAX_LOG_LINES;
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
      if (pLogData->buffer[logIdx][0] != '\0') {
        entryArray.add(pLogData->buffer[logIdx]);
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