#include <telnet.h>
#include <message.h>
#include <config.h>
#include <simulation.h>
#include <language.h>
#include <webUI.h>

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

const char *shell = "$ >";
void readLogger();
s_opt_arrays webOptArrays;

void onTelnetConnect(String ip)
{
  Serial.print("Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  
  telnet.println("\n----------------------------------------------------------------------");
  telnet.println("\nESP Buderus KM271");
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("use command: help for futher information");
  telnet.println("\n----------------------------------------------------------------------\n\n");
  telnet.print(shell);
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

void onTelnetInput(String str) {
  
    const int MAX_PAR = 3;
    const int MAX_CHAR = 32;
    char param[MAX_PAR][MAX_CHAR];
    const char *p = str.c_str();
    int i=0, par=0;
    
    // initialize parameter strings
    for (int j=0; j<MAX_PAR; j++){
        param[j][0] = '\0';
    }
    // extract answer into parameter
    while (*p!='\n'){
      if (i>=MAX_CHAR-1 ){
        param[par][i] = '\0';
        break;
      }
      if (*p==' ' || i==MAX_CHAR-1 ){
        param[par][i] = '\0';
        par++;
        p++;
        i=0;
        if (par>=MAX_PAR)
          break; 
      }
      param[par][i] = *p++;
      i++;
    }

  // check for valid commands
  
  // help: a list of commands
  if (!strcmp(param[0], "help") && strlen(param[1])==0) {
      telnet.println(HELP);
      telnet.print(shell);
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
      telnet.print(shell);
  // log: get enabled/disabled status
  } else if (!strcmp(param[0], "log") && strlen(param[1])==0) {
      if (config.log.enable)
        telnet.println("enabled");
      else
        telnet.println("disabled");
      telnet.print(shell);
  // log: disable
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "0") && strlen(param[2])==0) {
      config.log.enable = false;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("disabled");
      telnet.print(shell);
  // log enable
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "1") && strlen(param[2])==0) {
      config.log.enable = true;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println("enabled");
      telnet.print(shell);
  // log: read buffer
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "read") && strlen(param[2])==0) {
      readLogger();
      telnet.print(shell);
  // log clear buffer
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "clear") && strlen(param[2])==0) {
      clearLogBuffer();
      telnet.print(shell);
  // log mode read
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "mode") && strlen(param[2])==0) {
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
      telnet.print(shell);
  // log mode set
  } else if (!strcmp(param[0], "log") && !strcmp(param[1], "mode") && strlen(param[2])>0) {
    int mode = atoi(param[2]);
    if (mode > 0 && mode < 6)
    {
      config.log.filter = mode - 1;
      clearLogBuffer();
      configSaveToFile();
      updateSettingsValues();
      telnet.println(webOptArrays.LOG_FILTER[config.lang][config.log.filter]);
    }
    else {
      telnet.println("invalid mode - mode must be between 1 and 5");
    }
    telnet.print(shell);
  // debug: get enabled/disabled status
  } else if (!strcmp(param[0], "debug") && strlen(param[1])==0 && strlen(param[2])==0) {
      if (config.debug.enable)
        telnet.println("enabled");
      else
        telnet.println("disabled");
      telnet.print(shell);
  // debug: disable
  } else if (!strcmp(param[0], "debug") && !strcmp(param[1], "0") && strlen(param[2])==0) {
        config.debug.enable = false;
        clearLogBuffer();
        configSaveToFile();
        updateSettingsValues();
        telnet.println("disabled");
        telnet.print(shell);
  // debug: enable
  } else if (!strcmp(param[0], "debug") && !strcmp(param[1], "1") && strlen(param[2])==0) {
        config.debug.enable = true;
        clearLogBuffer();
        configSaveToFile();
        updateSettingsValues();
        telnet.println("enabled");
        telnet.print(shell);
  // debug: get filter
    } else if (!strcmp(param[0], "debug") && !strcmp(param[1], "filter") && strlen(param[2])==0) {
      telnet.println(config.debug.filter);
      telnet.print(shell);
  // empty command
  } else if (strlen(param[0])==0){
      telnet.printf("\n%s", shell);
  // unknown command
  } else {
      telnet.println("unknown command");
      telnet.print(shell);
  }
  
}


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

void cyclicTelnet() {
  telnet.loop();
}


void readLogger(){
  int line = 0;
  while (line < MAX_LOG_LINES)
  {
    int lineIndex;
    if (logData.buffer[logData.lastLine][0] == '\0')
    {
      // buffer is not full - start reading at element index 0
      lineIndex = line % MAX_LOG_LINES;
    }
    else
    {
      // buffer is full - start reading at element index "logData.lastLine"
      lineIndex = (logData.lastLine + line) % MAX_LOG_LINES;
    }
    // check buffer overflow
    if (lineIndex < 0)
    {
      lineIndex += MAX_LOG_LINES;
    }
    if (lineIndex >= MAX_LOG_LINES)
    {
      lineIndex = 0;
    }
    // add header
    if (line == 0)
    {
      telnet.printf("<LOG-Begin>\n%s\n", logData.buffer[lineIndex]);
      line++;
    }
    // add footer
    else if (line == MAX_LOG_LINES - 1)
    {
      telnet.printf("%s\n<LOG-END>\n", logData.buffer[lineIndex]);
      line++;
    }
    // add messages
    else
    {
      if (logData.buffer[lineIndex][0] != '\0')
      {
        telnet.printf("%s\n", logData.buffer[lineIndex]);
        line++;
      }
      else
      {
        telnet.printf("<LOG-END>\n");
        break;
      }
    }
  }
}