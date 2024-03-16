#include <webUI.h>
#include <webTools.h>
#include <basics.h>
#include <km271.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

int counter = 0;
bool state = false;
int option = 0;

s_km271_status kmStatusCpy;
s_km271_config_num kmConfigCpy;
s_km271_config_str kmConfigStrCpy;

/* P R O T O T Y P E S ********************************************************/ 
void webCallback(const char *elementId, const char *value);

/* D E C L A R A T I O N S ****************************************************/
muTimer refreshTimer = muTimer();         // timer to refresh other values
muTimer connectionTimer = muTimer();         // timer to refresh other values


void handleData(AsyncWebServerRequest *request) {
  if (request->hasParam("elementId") && request->hasParam("value")) {
    String elementId = request->getParam("elementId")->value();
    String value = request->getParam("value")->value();
    webCallback(elementId.c_str(), value.c_str());
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Invalid Request");
  }
}

void sendWebUpdate(const char *message, const char * event) {
  events.send(message, event, millis());
  delay(10);
}

void hideElementClass(const String& className, bool hide) {
  String message = "{\"className\":\"" + className + "\",\"hide\":" + (hide ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "hideElementClass");
}

void setLanguage(const char* language) {
  String message = "{\"language\":\"" + String(language) + "\"" + "}";
  sendWebUpdate(message.c_str(), "setLanguage");
}

void updateWebText(const char* elementID, const char* text, bool isInput) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"text\":\"" + String(text) + "\",\"isInput\":" + (isInput ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "updateText");
}

void updateWebState(const char* elementID, bool state) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"state\":" + (state ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "updateState");
}

void updateWebValueStr(const char* elementID, const char* value) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"value\":\"" + value + "\"}";
  sendWebUpdate(message.c_str(), "updateValue");
}

void updateWebValueInt32(const char* elementID, long value) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"value\":\"" + String(value) + "\"}";
  sendWebUpdate(message.c_str(), "updateValue");
}

void updateWebValueFloat32(const char* elementID, double value) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"value\":\"" + String(value) + "\"}";
  sendWebUpdate(message.c_str(), "updateValue");
}

void enableElement(const char* elementID, bool enable) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"enable\":" + (enable ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "enableElement");
}

// Callback function for web elements
void webCallback(const char *elementId, const char *value){
    Serial.print("Received - Element ID: ");
    Serial.print(elementId);
    Serial.print(", Value: ");
    Serial.println(value);

}


/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUISetup(){

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", gzip_html, gzip_html_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", gzip_css, gzip_css_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/gzip.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/js", gzip_js, gzip_js_size);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/sendData", HTTP_GET, handleData);

  // SSE Endpoint
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it gots is: %u\n", client->lastId());
    }
    // Send a message to newly connected client
    client->send("ping", NULL, millis(), 5000);
  });

  webToolsSetup();

  server.addHandler(&events);
  server.begin();

} // END SETUP




/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none 
 * @return  none
 * *******************************************************************/
void updateKm271Status(){

  if (kmStatusCpy.BoilerErrorStates != kmStatus.)

}




/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUICylic(){



  if (connectionTimer.cycleTrigger(2000))
  {
      events.send("ping", "ping", millis());
  }


}
