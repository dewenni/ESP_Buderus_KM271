#include <telnet.h>
#include <basics.h>
#include <message.h>
#include <config.h>
#include <simulation.h>
#include <language.h>
#include <webUI.h>
#include <oilmeter.h>
#include "EscapeCodes.h"

ESPTelnet telnet;
s_telnetIF telnetIF;

const char HELP[] PROGMEM = R"rawliteral(
here is a list of supported commands:
x                                 aboard active streaming (Serial/KM271)
cls                               clear screen
info                              print system information
restart                           restart the ESP
disconnect                        disconnect from telnet server
simdata                           generate simulation data 
log read                          read all entries of the log buffer
log clear                         clear the log buffer
log mode                          get actual log mode
log mode 1..5                     set log mode 1:Alarm, 2:Alarm+Info, 3:Logamatic values, 4:unknown telegrams, 5:debug telegrams
debug <0/1>                       get status or enable/disable debug messages
debug filter <XX_XX..>            get or set debug filter
serial stream start/stop          start stop serial stream
km271 stream start/stop           start stop serial stream
oilmeter <value>                  get or set Oil Meter value
)rawliteral";

void readLogger();
s_opt_arrays webOptArrays;
const int MAX_PAR = 3;
const int MAX_CHAR = 64;
char param[MAX_PAR][MAX_CHAR];
EscapeCodes ansi;

void telnetShell(){
  telnet.print(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.print("$ >");
  telnet.print(ansi.reset());
}


void onTelnetConnect(String ip)
{
  msg("Telnet: ");
  msg(ip.c_str());
  msgLn(" connected");
  telnet.println(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nESP Buderus KM271");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: \"help\" for further information");
  telnet.println("\n----------------------------------------------------------------------\n");
  telnet.println(ansi.reset());
  telnetShell();
}

void onTelnetDisconnect(String ip) {
  msg("- Telnet: ");
  msg(ip.c_str());
  msgLn(" disconnected");
}

void onTelnetReconnect(String ip) {
  msg("- Telnet: ");
  msg(ip.c_str());
  msgLn(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  msg("- Telnet: ");
  msg(ip.c_str());
  msgLn(" tried to connected");
}

/**
 * *******************************************************************
 * @brief   log commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void telCmdLog(char param[MAX_PAR][MAX_CHAR]){
 
  if (strlen(param[1])==0) {
      if (config.log.enable)
        telnet.println("enabled");
      else
        telnet.println("disabled");
      telnetShell();
  // log: disable
  } else if (!strcmp(param[1], "0") && strlen(param[2])==0) {
      config.log.enable = false;
      clearLogBuffer();
      configSaveToFile();
      //updateSettingsValues();
      telnet.println("disabled");
      telnetShell();
  // log enable
  } else if (!strcmp(param[1], "1") && strlen(param[2])==0) {
      config.log.enable = true;
      clearLogBuffer();
      configSaveToFile();
      //updateSettingsValues();
      telnet.println("enabled");
      telnetShell();
  // log: read buffer
  } else if (!strcmp(param[1], "read") && strlen(param[2])==0) {
      readLogger();
      telnetShell();
  // log clear buffer
  } else if (!strcmp(param[1], "clear") && strlen(param[2])==0) {
      clearLogBuffer();
      telnetShell();
  // log mode read
  } else if (!strcmp(param[1], "mode") && strlen(param[2])==0) {
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
      telnetShell();
  // log mode set
  } else if (!strcmp(param[1], "mode") && strlen(param[2])>0) {
    int mode = atoi(param[2]);
    if (mode > 0 && mode < 6)
    {
      config.log.filter = mode - 1;
      clearLogBuffer();
      configSaveToFile();
      //updateSettingsValues();
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    } else {
      telnet.println("invalid mode - mode must be between 1 and 5");
    }
    telnetShell();
  } else {
      telnet.println("unknown parameter");
      telnetShell();
  }
}

/**
 * *******************************************************************
 * @brief   Debug commands
 * @param   params received parameters
 * @return  none
 * *******************************************************************/
void telCmdDebug(char param[MAX_PAR][MAX_CHAR]){
  if (strlen(param[1])==0 && strlen(param[2])==0) {
    if (config.debug.enable) {
      telnet.println("enabled");
    } else {
      telnet.println("disabled");
    }
    telnetShell();
  // debug: disable
  } else if (!strcmp(param[1], "0") && strlen(param[2])==0) {
      config.debug.enable = false;
      clearLogBuffer();
      configSaveToFile();
      //updateSettingsValues();
      telnet.println("disabled");
      telnetShell();
  // debug: enable
  } else if (!strcmp(param[1], "1") && strlen(param[2])==0) {
      config.debug.enable = true;
      clearLogBuffer();
      configSaveToFile();
      //updateSettingsValues();
      telnet.println("enabled");
      telnetShell();
  // debug: get filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2])==0) {
      telnet.println(config.debug.filter);
      telnetShell();
  // debug: set filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2])!=0) {
      char errMsg[256];
      if (setDebugFilter(param[2], strlen(param[2]), errMsg, sizeof(errMsg))){
        telnet.println(config.debug.filter);
      } else {
        telnet.println(errMsg);
      }
      telnetShell();     
  } else {
      telnet.println("unknown parameter");
      telnetShell();
  }
}

/**
 * *******************************************************************
 * @brief   function to print system parameters
 * @param   none
 * @return  none
 * *******************************************************************/
void getSysInfo(){

  telnet.print(ansi.setFG(ANSI_BRIGHT_WHITE));
  telnet.println("ESP-INFO");
  telnet.print(ansi.reset());
  telnet.printf("ESP Flash Usage: %s %%\n", floatToString((float)ESP.getSketchSize() * 100 / ESP.getFreeSketchSpace()));
  telnet.printf("ESP Heap Usage: %s %%\n", floatToString((float)ESP.getFreeHeap()*100/ESP.getHeapSize()));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", floatToString((float)ESP.getMaxAllocHeap()/1000.0));
  telnet.printf("ESP MAX Alloc Heap: %s KB\n", floatToString((float)ESP.getMinFreeHeap()/1000.0));
  
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
 * @brief   function to read log buffer
 * @param   none
 * @return  none
 * *******************************************************************/
void readLogger(){
  int line = 0;
  while (line < MAX_LOG_LINES) {
    int lineIndex;
    if (line == 0 && logData.lastLine == 0){
      telnet.println("log empty");
      break;
    } else if (logData.buffer[logData.lastLine][0] == '\0') {
      lineIndex = line % MAX_LOG_LINES; // buffer is not full - start reading at element index 0
    } else {
      lineIndex = (logData.lastLine + line) % MAX_LOG_LINES;  // buffer is full - start reading at element index "logData.lastLine"
    } if (lineIndex < 0) {
      lineIndex += MAX_LOG_LINES;
    } if (lineIndex >= MAX_LOG_LINES){
      lineIndex = 0;
    } if (line == 0) {
      telnet.printf("<LOG-Begin>\n%s\n", logData.buffer[lineIndex]);  // add header
      line++;
    } else if (line == MAX_LOG_LINES - 1) {
      telnet.printf("%s\n<LOG-END>\n", logData.buffer[lineIndex]);  // add footer
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
bool extractMessage(String str, char param[MAX_PAR][MAX_CHAR]){
  const char *p = str.c_str();
  int i=0, par=0;
  // initialize parameter strings
  for (int j=0; j<MAX_PAR; j++){
      memset(&param[j], 0, sizeof(param[0]));
  }
  // extract answer into parameter
  while (*p!='\0'){
    if (i>=MAX_CHAR-1 ){
      param[par][i] = '\0';
      return false;
    }
    if (*p==' ' || i==MAX_CHAR-1 ){
      param[par][i] = '\0';
      par++;
      p++;
      i=0;
      if (par>=MAX_PAR)
        return false;
    }
    param[par][i] = *p++;
    i++;
  }
  return true;
}

/**
 * *******************************************************************
 * @brief   receives telnet message and call sub-functions
 * @return  none
 * *******************************************************************/
void onTelnetInput(String str) {

// check for valid commands
if (!extractMessage(str, param))
{
  telnet.println("syntax error");
  telnetShell();
  }

  // empty command
  if (strlen(param[0])==0){
      telnetShell();
  // CLS - clear screen
  } else if (!strcmp(param[0], "cls")){
    telnet.print(ansi.cls());
    telnet.print(ansi.home());
    telnetShell();  
  // "x" to abort streaming
  } else if (!strcmp(param[0], "x")){
    telnetIF.km271Stream = false;
    telnetIF.serialStream = false;
    telnet.println("streaming aborted");
    telnetShell();
  // help: a list of commands
  } else if (!strcmp(param[0], "help") && strlen(param[1])==0) {
      telnet.println(HELP);
      telnetShell();
  // info
  } else if (!strcmp(param[0], "info") && strlen(param[1])==0) {
      getSysInfo();
      telnetShell();
    // restart: restart the ESP
  } else if (!strcmp(param[0], "restart") && strlen(param[1])==0) {
      telnet.println("ESP will restart - you have to reconnect");
      ESP.restart();
  // disconnect: disconnect from telnet server
  } else if (!strcmp(param[0], "disconnect") && strlen(param[1])==0){
      telnet.println("disconnect");
      telnet.disconnectClient();
  // simdata: generate simulation data 
  } else if (!strcmp(param[0], "simdata") && strlen(param[1])==0) {
      startSimData();
      telnet.println("simdata started");
      telnetShell();
  // log commands
  } else if (!strcmp(param[0], "log")) {
      telCmdLog(param);
  // debug commands
  } else if (!strcmp(param[0], "debug")) {
      telCmdDebug(param);   
  // oilmeter: get
  } else if (!strcmp(param[0], "oilmeter") && strlen(param[1])==0) {
      telnet.println(getOilmeter());
      telnetShell();
  // oilmeter: set
  } else if (!strcmp(param[0], "oilmeter") && strlen(param[1])!=0) {
      cmdSetOilmeter(atol(param[1]));
      telnet.println(getOilmeter());
      telnetShell();
  // serial start
  } else if (!strcmp(param[0], "serial") && !strcmp(param[1], "stream") && !strcmp(param[2], "start")) {
      telnetIF.serialStream = true;
      telnet.println("serial stream active - to abort streaming, send \"x\"");
      telnetShell();
  // serial stop
  } else if (!strcmp(param[0], "serial") && !strcmp(param[1], "stream") && !strcmp(param[2], "stop")) {
      telnetIF.serialStream = false;
      telnet.println("serial stream disabled");
      telnetShell(); 
   // km271 start
  } else if (!strcmp(param[0], "km271") && !strcmp(param[1], "stream") && !strcmp(param[2], "start")) {
      telnetIF.km271Stream = true;
      telnet.printf("km271 stream active - %s\n", webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
      telnet.println("=> to abort streaming, send \"x\"");
      telnetShell();
  // km271 stop
  } else if (!strcmp(param[0], "km271") && !strcmp(param[1], "stream") && !strcmp(param[2], "stop")) {
      telnetIF.km271Stream = false;
      telnet.println("km271 stream disabled");
      telnetShell(); 
    // unknown command
  } else {
      telnet.println("unknown command");
      telnetShell();
  }
  
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

  msg("Telnet Server: ");
  if (telnet.begin()) {
    msgLn("running!");
  } else {
    msgLn("error!");
  }
}

/**
 * *******************************************************************
 * @brief   cyclic function for Telnet
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicTelnet() {
  telnet.loop();
}

