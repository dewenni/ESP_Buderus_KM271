#include "EscapeCodes.h"
#include <basics.h>
#include <config.h>
#include <language.h>
#include <message.h>
#include <mqttDiscovery.h>
#include <oilmeter.h>
#include <simulation.h>
#include <telnet.h>
#include <webUI.h>

/* D E C L A R A T I O N S ****************************************************/
ESPTelnet telnet;
s_telnetIF telnetIF;
s_opt_arrays webOptArrays;
EscapeCodes ansi;
char param[MAX_PAR][MAX_CHAR];
static const char *TAG = "TELNET"; // LOG TAG

/* P R O T O T Y P E S ********************************************************/
void readLogger();
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]);
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]);
void cmdHelp(char param[MAX_PAR][MAX_CHAR]);
void cmdCls(char param[MAX_PAR][MAX_CHAR]);
void cmdConfig(char param[MAX_PAR][MAX_CHAR]);
void cmdInfo(char param[MAX_PAR][MAX_CHAR]);
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]);
void cmdRestart(char param[MAX_PAR][MAX_CHAR]);
void cmdSimdata(char param[MAX_PAR][MAX_CHAR]);
void cmdOilmeter(char param[MAX_PAR][MAX_CHAR]);
void cmdLog(char param[MAX_PAR][MAX_CHAR]);
void cmdDebug(char param[MAX_PAR][MAX_CHAR]);
void cmdSerial(char param[MAX_PAR][MAX_CHAR]);
void cmdKm271(char param[MAX_PAR][MAX_CHAR]);
void cmdHA(char param[MAX_PAR][MAX_CHAR]);
void cmdTest(char param[MAX_PAR][MAX_CHAR]);

Command commands[] = {
    {"cls", cmdCls, "Clear screen", ""},
    {"config", cmdConfig, "config commands", "<reset>"},
    {"debug", cmdDebug, "configure debug options", "[enable], [disable], [filter], [filter <XX_XX_...>]"},
    {"disconnect", cmdDisconnect, "disconnect telnet", ""},
    {"help", cmdHelp, "Displays this help message", "[command]"},
    {"info", cmdInfo, "Print system information", ""},
    {"km271", cmdKm271, "km271 stream output", "<stream> <[start], [stop]>"},
    {"log", cmdLog, "logger commands", "[enable], [disable], [read], [clear], [mode], [mode <1..5>]"},
    {"oilmeter", cmdOilmeter, "Get or set oil-meter value", "[value]"},
    {"restart", cmdRestart, "Restart the ESP", ""},
    {"serial", cmdSerial, "serial stream output", "<stream> <[start], [stop]>"},
    {"simdata", cmdSimdata, "generate simulated KM271 values", ""},
    {"ha", cmdHA, "Home Assistant commands", "[sendconfig], [resetconfig]"},
    {"test", cmdTest, "test commands", "[crash]"},
};
const int commandsCount = sizeof(commands) / sizeof(commands[0]);

// print telnet shell
void telnetShell() {
  telnet.print(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.print("$ >");
  telnet.print(ansi.reset());
}

void onTelnetConnect(String ip) {
  MY_LOGI(TAG, "Telnet: %s connected", ip.c_str());
  telnet.println(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nESP Buderus KM271");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: \"help\" for further information");
  telnet.println("\n----------------------------------------------------------------------\n");
  telnet.println(ansi.reset());
  telnetShell();
}

void onTelnetDisconnect(String ip) { MY_LOGI(TAG, "Telnet: %s disconnected", ip.c_str()); }

void onTelnetReconnect(String ip) { MY_LOGI(TAG, "Telnet: %s reconnected", ip.c_str()); }

void onTelnetConnectionAttempt(String ip) { MY_LOGI(TAG, "Telnet: %s tried to connect", ip.c_str()); }

void onTelnetInput(String str) {
  if (!extractMessage(str, param)) {
    telnet.println("Syntax error");
  } else {
    dispatchCommand(param);
  }
  telnetShell();
}

/**
 * *******************************************************************
 * @brief   setup function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void setupTelnet() {
  // passing on functions for various telnet events
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  if (telnet.begin(23, false)) {
    MY_LOGI(TAG, "Telnet Server running!");
  } else {
    MY_LOGI(TAG, "Telnet Server error!");
  }
}

/**
 * *******************************************************************
 * @brief   cyclic function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicTelnet() { telnet.loop(); }

/**
 * *******************************************************************
 * @brief   telnet command: log commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdLog(char param[MAX_PAR][MAX_CHAR]) {

  if (strlen(param[1]) == 0) {
    if (config.log.enable)
      telnet.println("enabled");
    else
      telnet.println("disabled");
    telnetShell();
    // log: disable
  } else if (!strcmp(param[1], "disable") && strlen(param[2]) == 0) {
    config.log.enable = false;
    clearLogBuffer();
    // updateSettingsValues();
    telnet.println("disabled");
    // log enable
  } else if (!strcmp(param[1], "enable") && strlen(param[2]) == 0) {
    config.log.enable = true;
    clearLogBuffer();
    telnet.println("enabled");
    // log: read buffer
  } else if (!strcmp(param[1], "read") && strlen(param[2]) == 0) {
    readLogger();
    // log clear buffer
  } else if (!strcmp(param[1], "clear") && strlen(param[2]) == 0) {
    clearLogBuffer();
    // log mode read
  } else if (!strcmp(param[1], "mode") && strlen(param[2]) == 0) {
    telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    // log mode set
  } else if (!strcmp(param[1], "mode") && strlen(param[2]) > 0) {
    int mode = atoi(param[2]);
    if (mode > 0 && mode < 7) {
      config.log.filter = mode - 1;
      clearLogBuffer();
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    } else {
      telnet.println("invalid mode - mode must be between 1 and 6");
    }
  } else {
    telnet.println("unknown parameter");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: debug options
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdDebug(char param[MAX_PAR][MAX_CHAR]) {
  if (strlen(param[1]) == 0 && strlen(param[2]) == 0) {
    if (config.debug.enable) {
      telnet.println("enabled");
    } else {
      telnet.println("disabled");
    }
    // debug: disable
  } else if (!strcmp(param[1], "disable") && strlen(param[2]) == 0) {
    config.debug.enable = false;
    clearLogBuffer();
    telnet.println("disabled");
    // debug: enable
  } else if (!strcmp(param[1], "enable") && strlen(param[2]) == 0) {
    config.debug.enable = true;
    clearLogBuffer();
    telnet.println("enabled");
    // debug: get filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2]) == 0) {
    telnet.println(config.debug.filter);
    // debug: set filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2]) != 0) {
    char errMsg[256];
    if (setDebugFilter(param[2], strlen(param[2]), errMsg, sizeof(errMsg))) {
      telnet.println(config.debug.filter);
    } else {
      telnet.println(errMsg);
    }
  } else {
    telnet.println("unknown parameter");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: config structure
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdTest(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "crash") && !strcmp(param[2], "")) {
    int *ptr = nullptr; // Nullpointer
    *ptr = 42;
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: config structure
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdConfig(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "reset") && !strcmp(param[2], "")) {
    configInitValue();
    configSaveToFile();
    telnet.println("config was set to defaults");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: Home Assistant
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdHA(char param[MAX_PAR][MAX_CHAR]) {

  if (!strcmp(param[1], "sendconfig") && !strcmp(param[2], "")) {
    mqttDiscoverySendConfig();
    telnet.println("home assistant discovery messages sent");
  } else if (!strcmp(param[1], "resetconfig") && !strcmp(param[2], "")) {
    mqttDiscoveryResetConfig();
    telnet.println("home assistant discovery config reset messages sent");
  }
}
/**
 * *******************************************************************
 * @brief   telnet command: print system information
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdInfo(char param[MAX_PAR][MAX_CHAR]) {

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("ESP-INFO");
  telnet.print(ansi.reset());
  telnet.printf("ESP Flash Usage: %s %%\n", floatToString((float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace()));
  telnet.printf("ESP Heap Usage: %s %%\n", floatToString((float)ESP.getFreeHeap() * 100 / ESP.getHeapSize()));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", floatToString((float)ESP.getMaxAllocHeap() / 1000.0));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", floatToString((float)ESP.getMinFreeHeap() / 1000.0));

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nRESTART - UPTIME");
  telnet.print(ansi.reset());
  char tmpMsg[64];
  getUptime(tmpMsg, sizeof(tmpMsg));
  telnet.printf("Uptime: %s\n", tmpMsg);
  getRestartReason(tmpMsg, sizeof(tmpMsg));
  telnet.printf("Restart Reason: %s\n", tmpMsg);

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nWiFi-INFO");
  telnet.print(ansi.reset());
  telnet.printf("IP-Address: %s\n", wifi.ipAddress);
  telnet.printf("WiFi-Signal: %s %%\n", uint8ToString(wifi.signal));
  telnet.printf("WiFi-Rssi: %s dbm\n", int8ToString(wifi.rssi));

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("\nLOGGING-INFO");
  telnet.print(ansi.reset());
  telnet.printf("Logging: %s\n", config.log.enable ? "enabled" : "disabled");
  telnet.printf("Log %s\n", webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
  telnet.printf("Debug: %s\n", config.debug.enable ? "enabled" : "disabled");
  telnet.printf("Debug Filter: %s\n", config.debug.filter);

  telnet.println();
}

/**
 * *******************************************************************
 * @brief   telnet command: clear output
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdCls(char param[MAX_PAR][MAX_CHAR]) {
  telnet.print(ansi.cls());
  telnet.print(ansi.home());
}

/**
 * *******************************************************************
 * @brief   telnet command: disconnect
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdDisconnect(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("disconnect");
  telnet.disconnectClient();
}

/**
 * *******************************************************************
 * @brief   telnet command: restart ESP
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdRestart(char param[MAX_PAR][MAX_CHAR]) {
  telnet.println("ESP will restart - you have to reconnect");
  saveRestartReason("telnet command");
  yield();
  delay(1000);
  yield();
  ESP.restart();
}

/**
 * *******************************************************************
 * @brief   telnet command: generate simulation values
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdSimdata(char param[MAX_PAR][MAX_CHAR]) {
  startSimData();
  telnet.println("simdata started");
}

/**
 * *******************************************************************
 * @brief   telnet command: serial stream
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdSerial(char param[MAX_PAR][MAX_CHAR]) {
  if (!strcmp(param[1], "stream") && !strcmp(param[2], "start")) {
    telnetIF.serialStream = true;
    telnet.println("serial stream active - to abort streaming, send \"x\"");
  } else if (!strcmp(param[1], "stream") && !strcmp(param[2], "stop")) {
    telnetIF.serialStream = false;
    telnet.println("serial stream disabled");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: km271 stream
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdKm271(char param[MAX_PAR][MAX_CHAR]) {
  if (!strcmp(param[1], "stream") && !strcmp(param[2], "start")) {
    telnetIF.km271Stream = true;
    telnet.printf("km271 stream active - %s\n", webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    telnet.println("=> to abort streaming, send \"x\"");
  } else if (!strcmp(param[1], "stream") && !strcmp(param[2], "stop")) {
    telnetIF.km271Stream = false;
    telnet.println("km271 stream disabled");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: get/set oil meter
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdOilmeter(char param[MAX_PAR][MAX_CHAR]) {
  // oilmeter: get
  if (strlen(param[1]) == 0) {
    telnet.println(getOilmeter());
    // oilmeter: set
  } else if (strlen(param[1]) != 0) {
    cmdSetOilmeter(atol(param[1]));
    telnet.println(getOilmeter());
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: sub function to print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void printHelp(const char *command = nullptr) {
  if (command == nullptr) {
    // print help of all commands
    for (int i = 0; i < commandsCount; ++i) {
      telnet.printf("%-15s %-60s\n", commands[i].name, commands[i].parameters);
    }
  } else {
    // print help of specific command
    for (int i = 0; i < commandsCount; ++i) {
      if (strcmp(command, commands[i].name) == 0) {
        telnet.printf("%-15s %-60s\n%s\n", commands[i].name, commands[i].parameters, commands[i].description);
        return;
      }
    }
    telnet.println("Unknown command. Use 'help' to see all commands.");
  }
}

/**
 * *******************************************************************
 * @brief   telnet command: print help
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void cmdHelp(char param[MAX_PAR][MAX_CHAR]) {
  if (strlen(param[1]) > 0) {
    printHelp(param[1]);
  } else {
    printHelp();
  }
}

/**
 * *******************************************************************
 * @brief   function to read log buffer
 * @param   none
 * @return  none
 * *******************************************************************/
void readLogger() {
  int line = 0;
  while (line < MAX_LOG_LINES) {
    int lineIndex;
    if (line == 0 && logData.lastLine == 0) {
      telnet.println("log empty");
      break;
    } else if (logData.buffer[logData.lastLine][0] == '\0') {
      lineIndex = line % MAX_LOG_LINES; // buffer is not full - start reading at element index 0
    } else {
      lineIndex = (logData.lastLine + line) % MAX_LOG_LINES; // buffer is full - start reading at element index "logData.lastLine"
    }
    if (lineIndex < 0) {
      lineIndex += MAX_LOG_LINES;
    }
    if (lineIndex >= MAX_LOG_LINES) {
      lineIndex = 0;
    }
    if (line == 0) {
      telnet.printf("<LOG-Begin>\n%s\n", logData.buffer[lineIndex]); // add header
      line++;
    } else if (line == MAX_LOG_LINES - 1) {
      telnet.printf("%s\n<LOG-END>\n", logData.buffer[lineIndex]); // add footer
      line++;
    } else {
      if (logData.buffer[lineIndex][0] != '\0') {
        telnet.printf("%s\n", logData.buffer[lineIndex]); // add messages
        line++;
      } else {
        telnet.printf("<LOG-END>\n");
        break;
      }
    }
  }
}

/**
 * *******************************************************************
 * @brief   check receives telnet message and extract to param array
 * @param   str received message
 * @param   param char array of parameters
 * @return  none
 * *******************************************************************/
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]) {
  const char *p = str.c_str();
  int i = 0, par = 0;
  // initialize parameter strings
  for (int j = 0; j < MAX_PAR; j++) {
    memset(&param[j], 0, sizeof(param[0]));
  }
  // extract answer into parameter
  while (*p != '\0') {
    if (i >= MAX_CHAR - 1) {
      param[par][i] = '\0';
      return false;
    }
    if (*p == ' ' || i == MAX_CHAR - 1) {
      param[par][i] = '\0';
      par++;
      p++;
      i = 0;
      if (par >= MAX_PAR)
        return false;
    }
    param[par][i] = *p++;
    i++;
  }
  return true;
}

/**
 * *******************************************************************
 * @brief   telnet command dispatcher
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void dispatchCommand(char param[MAX_PAR][MAX_CHAR]) {
  for (int i = 0; i < commandsCount; ++i) {
    if (!strcmp(param[0], commands[i].name)) {
      commands[i].function(param);
      return;
    }
  }
  telnet.println("Unknown command");
}
