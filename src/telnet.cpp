#include <telnet.h>
#include <message.h>
#include <config.h>
#include <simulation.h>
#include <language.h>
#include <webUI.h>
#include <oilmeter.h>
#include "EscapeCodes.h"

ESPTelnet telnet;

const char HELP[] PROGMEM = R"rawliteral(
here is a list of supported commands

help           a list of commands
restart        restart the ESP
disconnect     disconnect from telnet server
simdata        generate simulation data 
log read       read all entries of the log buffer
log clear      clear the log buffer
log mode       get actual log mode
log mode 1..5  set log mode 1:Alarm, 2:Alarm+Info, 3:Logamatic values, 4:unknown telegrams, 5:debug telegrams
)rawliteral";

void readLogger();
s_opt_arrays webOptArrays;
const int MAX_PAR = 3;
const int MAX_CHAR = 64;
char param[MAX_PAR][MAX_CHAR];
EscapeCodes ansi;

void shell(){
  telnet.print(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.print("$ >");
  telnet.print(ansi.reset());
}


void onTelnetConnect(String ip)
{
  Serial.print("Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  telnet.println(ansi.setFG(ANSI_BRIGHT_GREEN));
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nESP Buderus KM271");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: \"help\" for futher information");
  telnet.println("\n----------------------------------------------------------------------\n");
  telnet.println(ansi.reset());
  shell();
}

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
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
      shell();
  // log: disable
  } else if (!strcmp(param[1], "0") && strlen(param[2])==0) {
      config.log.enable = false;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("disabled");
      shell();
  // log enable
  } else if (!strcmp(param[1], "1") && strlen(param[2])==0) {
      config.log.enable = true;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("enabled");
      shell();
  // log: read buffer
  } else if (!strcmp(param[1], "read") && strlen(param[2])==0) {
      readLogger();
      shell();
  // log clear buffer
  } else if (!strcmp(param[1], "clear") && strlen(param[2])==0) {
      clearLogBuffer();
      shell();
  // log mode read
  } else if (!strcmp(param[1], "mode") && strlen(param[2])==0) {
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
      shell();
  // log mode set
  } else if (!strcmp(param[1], "mode") && strlen(param[2])>0) {
    int mode = atoi(param[2]);
    if (mode > 0 && mode < 6)
    {
      config.log.filter = mode - 1;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    } else {
      telnet.println("invalid mode - mode must be between 1 and 5");
    }
    shell();
  } else {
      telnet.println("unknown parameter");
      shell();
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
    shell();
  // debug: disable
  } else if (!strcmp(param[1], "0") && strlen(param[2])==0) {
      config.debug.enable = false;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("disabled");
      shell();
  // debug: enable
  } else if (!strcmp(param[1], "1") && strlen(param[2])==0) {
      config.debug.enable = true;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("enabled");
      shell();
  // debug: get filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2])==0) {
      telnet.println(config.debug.filter);
      shell();
  // debug: set filter
  } else if (!strcmp(param[1], "filter") && strlen(param[2])!=0) {
      char errMsg[256];
      if (setDebugFilter(param[2], strlen(param[2]), errMsg, sizeof(errMsg))){
        telnet.println(config.debug.filter);
      } else {
        telnet.println(errMsg);
      }
      shell();     
  } else {
      telnet.println("unknown parameter");
      shell();
  }
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
  if (!extractMessage(str, param)){
    telnet.println("syntax error");
    shell();
  }

  // empty command
  if (strlen(param[0])==0){
      shell();
  // help: a list of commands
  } else if (!strcmp(param[0], "help") && strlen(param[1])==0) {
      telnet.println(HELP);
      shell();
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
      shell();
  // log commands
  } else if (!strcmp(param[0], "log")) {
      telCmdLog(param);
  // debug commands
  } else if (!strcmp(param[0], "debug")) {
      telCmdDebug(param);   
  // oilmeter: get
  } else if (!strcmp(param[0], "oilmeter") && strlen(param[1])==0) {
      telnet.println(getOilmeter());
      shell();
  // oilmeter: set
  } else if (!strcmp(param[0], "oilmeter") && strlen(param[1])!=0) {
      cmdSetOilmeter(atol(param[1]));
      telnet.println(getOilmeter());
      shell();
  // unknown command
  } else {
      telnet.println("unknown command");
      shell();
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

  Serial.print("Telnet Server: ");
  if (telnet.begin()) {
    Serial.println("running!");
  } else {
    Serial.println("error!");
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

