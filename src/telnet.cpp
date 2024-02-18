#include <telnet.h>
#include <message.h>
#include <config.h>
#include <simulation.h>

ESPTelnet telnet;

const char HELP[] PROGMEM = R"rawliteral(
here is a list of supported commands

help           a list of commands
restart        restart the ESP
disconnect     disconnect from telnet server
simdata        generate simulation data 
log read       read all entries of the log buffer
log clear      clear the log buffer
)rawliteral";

const char *shell = "$ >";
void readLogger();


void onTelnetConnect(String ip)
{
  Serial.print("- Telnet: ");
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
  if (strcmp(param[0], "help")) {
      telnet.println(HELP);
      telnet.print(shell);
  } else if (strcmp(param[0], "restart")) {
      telnet.println("ESP will restart - you have to reconnect");
      ESP.restart();
  } else if (strcmp(param[0], "disconnect")){
      telnet.println("disconnect");
      telnet.disconnectClient();
  } else if (strcmp(param[0], "simdata")) {
    startSimData();
    telnet.print(shell);
  } else if (strcmp(param[0], "log") && strcmp(param[1], "read")) {
    readLogger();
    telnet.print(shell);
  } else if (strcmp(param[0], "log") && strcmp(param[1], "clear")) {
    clearLogBuffer();
    telnet.print(shell);
  } else {
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
    // reverse reading
    if (config.log.order == 1)
    {
      lineIndex = (logData.lastLine - line - 1) % MAX_LOG_LINES;
    }
    else
    {
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
        break;
      }
    }
  }
}