#include <webUI.h>
#include <webTools.h>
#include <basics.h>
#include <km271.h>
#include <ESPAsyncWebServer.h>
#include <simulation.h>

AsyncWebServer server(80);
AsyncEventSource events("/events");

int counter = 0;
bool state = false;
int option = 0;
bool clientConnected = false;
bool bootInit = false;
bool updateKmConfig = false;

s_km271_status *pkmStatus;
s_km271_status kmStatusCpy;

s_km271_config_str *pkmConfigStr;
s_km271_config_num kmConfigNumCpy;
s_km271_config_num *pkmConfigNum;

s_webui_texts webText;
s_cfg_arrays cfgArrayTexts;

char tmpMessage[300]={'\0'};   

/* P R O T O T Y P E S ********************************************************/ 
void webCallback(const char *elementId, const char *value);
void updateAll(); 

/* D E C L A R A T I O N S ****************************************************/
muTimer refreshTimer = muTimer();         // timer to refresh other values
muTimer connectionTimer = muTimer();         // timer to refresh other values
muTimer simulationTimer = muTimer();         // timer to refresh other values

/**
 * *******************************************************************
 * @brief   create ON/OFF String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* onOffString(uint8_t value){
  static char ret_str[64];
  if (value!=0)
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ON[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OFF[config.lang]);

  return ret_str;
}
/**
 * *******************************************************************
 * @brief   create ERROR/OK String from integer
 * @param   value as integer
 * @return  pointer to char array - pay attention, it is local static
 * *******************************************************************/
const char* errOkString(uint8_t value){
  static char ret_str[64];
  if (value!=0)
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ERROR[config.lang]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OK[config.lang]);

  return ret_str;
}

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

void updateWebTextInt(const char* elementID, long value, bool isInput) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"text\":\"" + String(value) + "\",\"isInput\":" + (isInput ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "updateText");
}

void updateWebTextFloat(const char* elementID, double value, bool isInput) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"text\":\"" + String(value) + "\",\"isInput\":" + (isInput ? "true" : "false") + "}";
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

void updateWebValueInt(const char* elementID, long value) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"value\":\"" + String(value) + "\"}";
  sendWebUpdate(message.c_str(), "updateValue");
}

void updateWebValueFloat(const char* elementID, double value) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"value\":\"" + String(value) + "\"}";
  sendWebUpdate(message.c_str(), "updateValue");
}

void enableElement(const char* elementID, bool enable) {
  String message = "{\"elementID\":\"" + String(elementID) + "\",\"enable\":" + (enable ? "true" : "false") + "}";
  sendWebUpdate(message.c_str(), "enableElement");
}

// Callback function for web elements
void webCallback(const char *elementId, const char *value){
    msg("Received - Element ID: ");
    msg(elementId);
    msg(", Value: ");
    msgLn(value);

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
  events.onConnect([](AsyncEventSourceClient *client)
                   {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it gots is: %u\n", client->lastId());
    }
    // Send a message to newly connected client
    client->send("ping", NULL, millis(), 5000);
    updateAll(); 
    clientConnected=true; });

  server.addHandler(&events);
  server.begin();

  

} // END SETUP


/**
 * *******************************************************************
 * @brief   update all values (only call once)
 * @param   none 
 * @return  none
 * *******************************************************************/
void updateAll(){

  memset((void *)&kmStatusCpy, 111, sizeof(s_km271_status));
  memset((void *)&kmConfigNumCpy, 111, sizeof(s_km271_config_num));
  //memset((void *)&kmConfigStrCpy, 0, sizeof(s_km271_config_str));


}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none 
 * @return  none
 * *******************************************************************/
void updateKm271Config(){

  pkmConfigStr = km271GetConfigStringsAdr();
  pkmConfigNum = km271GetConfigValueAdr();

  updateWebValueInt("p02_hc1_frost_protection_threshold",pkmConfigNum->hc1_frost_protection_threshold);
  updateWebTextInt("p02_hc1_frost_protection_threshold_txt",pkmConfigNum->hc1_frost_protection_threshold, false);
  updateWebText("p03_hc1_frost_protection_threshold",pkmConfigStr->hc1_frost_protection_threshold, false);

  updateWebValueInt("p02_hc1_summer_mode_threshold",pkmConfigNum->hc1_summer_mode_threshold);
  updateWebTextInt("p02_hc1_summer_mode_threshold_txt",pkmConfigNum->hc1_summer_mode_threshold, false);
  updateWebText("p03_hc1_summer_mode_threshold",pkmConfigStr->hc1_summer_mode_threshold, false);

  updateWebValueInt("p02_hc2_frost_protection_threshold",pkmConfigNum->hc2_frost_protection_threshold);
  updateWebTextInt("p04_hc2_frost_protection_threshold_txt",pkmConfigNum->hc2_frost_protection_threshold, false); 
  updateWebText("p04_hc2_frost_protection_threshold",pkmConfigStr->hc2_frost_protection_threshold, false);  

  updateWebValueInt("p02_hc2_summer_mode_threshold",pkmConfigNum->hc2_summer_mode_threshold);
  updateWebTextInt("p04_hc2_summer_mode_threshold_txt",pkmConfigNum->hc2_summer_mode_threshold, false);
  updateWebText("p04_hc2_summer_mode_threshold",pkmConfigStr->hc2_summer_mode_threshold, false);

  updateWebText("p03_hc1_night_temp",pkmConfigStr->hc1_night_temp, false);
  updateWebText("p03_hc1_day_temp",pkmConfigStr->hc1_day_temp, false);
  
  updateWebState("p02_hc1_opmode_night",pkmConfigNum->hc1_operation_mode==0 ? true:false);
  updateWebState("p02_hc1_opmode_day",pkmConfigNum->hc1_operation_mode==1 ? true:false);
  updateWebState("p02_hc1_opmode_auto",pkmConfigNum->hc1_operation_mode==2 ? true:false);
  updateWebText("p03_hc1_operation_mode",pkmConfigStr->hc1_operation_mode, false);

  updateWebText("p03_hc1_holiday_temp",pkmConfigStr->hc1_holiday_temp, false);
  updateWebText("p03_hc1_max_temp",pkmConfigStr->hc1_max_temp, false);
  
  updateWebValueInt("p02_hc1_interpretation",pkmConfigNum->hc1_interpretation);
  updateWebTextInt("p02_hc1_interpretation_txt",pkmConfigNum->hc1_interpretation, false);
  updateWebText("p03_hc1_interpretation",pkmConfigStr->hc1_interpretation, false);

  updateWebText("p03_hc1_switch_on_temperature",pkmConfigStr->hc1_switch_on_temperature, false);

  updateWebValueInt("p02_hc1_switch_off_threshold",pkmConfigNum->hc1_switch_off_threshold);
  updateWebTextInt("p02_hc1_switch_off_threshold_txt",pkmConfigNum->hc1_switch_off_threshold, false);
  updateWebText("p03_hc1_switch_off_threshold",pkmConfigStr->hc1_switch_off_threshold, false); 
  
  updateWebText("p03_hc1_reduction_mode",pkmConfigStr->hc1_reduction_mode, false);
  updateWebValueInt("p02_hc1_reduct_mode",pkmConfigNum->hc1_reduction_mode);

  updateWebText("p03_hc1_heating_system",pkmConfigStr->hc1_heating_system, false);
  updateWebText("p03_hc1_temp_offset",pkmConfigStr->hc1_temp_offset, false);
  updateWebText("p03_hc1_remotecontrol",pkmConfigStr->hc1_remotecontrol, false);
  updateWebText("p04_hc2_night_temp",pkmConfigStr->hc2_night_temp, false);
  updateWebText("p04_hc2_day_temp",pkmConfigStr->hc2_day_temp, false);

  updateWebState("p02_hc2_opmode_night",pkmConfigNum->hc2_operation_mode==0 ? true:false);
  updateWebState("p02_hc2_opmode_day",pkmConfigNum->hc2_operation_mode==1 ? true:false);
  updateWebState("p02_hc2_opmode_auto",pkmConfigNum->hc2_operation_mode==2 ? true:false);
  updateWebText("p04_hc2_operation_mode",pkmConfigStr->hc2_operation_mode, false);

  updateWebText("p04_hc2_holiday_temp",pkmConfigStr->hc2_holiday_temp, false);
  updateWebText("p04_hc2_max_temp",pkmConfigStr->hc2_max_temp, false);

  updateWebValueInt("p02_hc2_interpretation",pkmConfigNum->hc2_interpretation);
  updateWebTextInt("p02_hc2_interpretation_txt",pkmConfigNum->hc2_interpretation, false);
  updateWebText("p04_hc2_interpretation",pkmConfigStr->hc2_interpretation, false);
  
  updateWebText("p05_hw_priority",pkmConfigStr->ww_priority, false);
  updateWebText("p04_hc2_switch_on_temperature",pkmConfigStr->hc2_switch_on_temperature, false);

  updateWebValueInt("p02_hc2_switch_off_threshold",pkmConfigNum->hc2_switch_off_threshold);
  updateWebTextInt("p02_hc2_switch_off_threshold_txt",pkmConfigNum->hc2_switch_off_threshold, false);
  updateWebText("p04_hc2_switch_off_threshold",pkmConfigStr->hc2_switch_off_threshold, false);
  
  updateWebText("p04_hc2_reduction_mode",pkmConfigStr->hc2_reduction_mode, false);
  updateWebValueInt("p02_hc2_reduct_mode",pkmConfigNum->hc2_reduction_mode);

  updateWebText("p04_hc2_heating_system",pkmConfigStr->hc2_heating_system, false);
  updateWebText("p04_hc2_temp_offset",pkmConfigStr->hc2_temp_offset, false);
  updateWebText("p04_hc2_remotecontrol",pkmConfigStr->hc2_remotecontrol, false);
  updateWebText("p07_building_type",pkmConfigStr->building_type, false);
  
  updateWebValueInt("p02_ww_temp",pkmConfigNum->ww_temp);
  updateWebTextInt("p02_ww_temp_txt",pkmConfigNum->ww_temp, false);
  updateWebText("p05_hw_temp",pkmConfigStr->ww_temp, false);

  updateWebState("p02_ww_opmode_night",pkmConfigNum->ww_operation_mode==0 ? true:false);
  updateWebState("p02_ww_opmode_day",pkmConfigNum->ww_operation_mode==1 ? true:false);
  updateWebState("p02_ww_opmode_auto",pkmConfigNum->ww_operation_mode==2 ? true:false);
  updateWebText("p05_hw_operation_mode",pkmConfigStr->ww_operation_mode, false);

  updateWebText("p05_hw_processing",pkmConfigStr->ww_processing, false);
  
  updateWebValueInt("p02_ww_circulation",pkmConfigNum->ww_circulation);
  updateWebTextInt("p02_ww_circulation_txt",pkmConfigNum->ww_circulation, false);
  updateWebText("p05_hw_circulation",pkmConfigStr->ww_circulation, false);
  
  updateWebText("p07_language",pkmConfigStr->language, false);
  updateWebText("p07_display",pkmConfigStr->display, false);
  updateWebText("p07_burner_type",pkmConfigStr->burner_type, false);
  updateWebText("p06_max_boiler_temperature",pkmConfigStr->max_boiler_temperature, false);
  updateWebText("p07_pump_logic_temp",pkmConfigStr->pump_logic_temp, false);
  updateWebText("p07_exhaust_gas_temperature_threshold",pkmConfigStr->exhaust_gas_temperature_threshold, false);
  updateWebText("p07_burner_min_modulation",pkmConfigStr->burner_min_modulation, false);
  updateWebText("p07_burner_modulation_runtime",pkmConfigStr->burner_modulation_runtime, false);
  
  updateWebValueInt("p02_hc1_prg",pkmConfigNum->hc1_program);
  updateWebValueInt("p02_hc1_holiday_days",pkmConfigNum->hc1_holiday_days);
  
  updateWebText("p03_hc1_timer01",pkmConfigStr->hc1_timer01, false);
  updateWebText("p03_hc1_timer02",pkmConfigStr->hc1_timer02, false);
  updateWebText("p03_hc1_timer03",pkmConfigStr->hc1_timer03, false);
  updateWebText("p03_hc1_timer04",pkmConfigStr->hc1_timer04, false);
  updateWebText("p03_hc1_timer05",pkmConfigStr->hc1_timer05, false);
  updateWebText("p03_hc1_timer06",pkmConfigStr->hc1_timer06, false);
  updateWebText("p03_hc1_timer07",pkmConfigStr->hc1_timer07, false);
  updateWebText("p03_hc1_timer08",pkmConfigStr->hc1_timer08, false);
  updateWebText("p03_hc1_timer09",pkmConfigStr->hc1_timer09, false);
  updateWebText("p03_hc1_timer10",pkmConfigStr->hc1_timer10, false);
  updateWebText("p03_hc1_timer11",pkmConfigStr->hc1_timer11, false);
  updateWebText("p03_hc1_timer12",pkmConfigStr->hc1_timer12, false);
  updateWebText("p03_hc1_timer13",pkmConfigStr->hc1_timer13, false);
  updateWebText("p03_hc1_timer14",pkmConfigStr->hc1_timer14, false);
  
  updateWebValueInt("p02_hc2_prg",pkmConfigNum->hc2_program);
  updateWebValueInt("p04_hc2_holiday_days",pkmConfigNum->hc2_holiday_days);
  
  updateWebText("p04_hc2_timer01",pkmConfigStr->hc2_timer01, false);
  updateWebText("p04_hc2_timer02",pkmConfigStr->hc2_timer02, false);
  updateWebText("p04_hc2_timer03",pkmConfigStr->hc2_timer03, false);
  updateWebText("p04_hc2_timer04",pkmConfigStr->hc2_timer04, false);
  updateWebText("p04_hc2_timer05",pkmConfigStr->hc2_timer05, false);
  updateWebText("p04_hc2_timer06",pkmConfigStr->hc2_timer06, false);
  updateWebText("p04_hc2_timer07",pkmConfigStr->hc2_timer07, false);
  updateWebText("p04_hc2_timer08",pkmConfigStr->hc2_timer08, false);
  updateWebText("p04_hc2_timer09",pkmConfigStr->hc2_timer09, false);
  updateWebText("p04_hc2_timer10",pkmConfigStr->hc2_timer10, false);
  updateWebText("p04_hc2_timer11",pkmConfigStr->hc2_timer11, false);
  updateWebText("p04_hc2_timer12",pkmConfigStr->hc2_timer12, false);
  updateWebText("p04_hc2_timer13",pkmConfigStr->hc2_timer13, false);
  updateWebText("p04_hc2_timer14",pkmConfigStr->hc2_timer14, false);
  
  updateWebText("p07_time_offset",pkmConfigStr->time_offset, false);

  updateKmConfig = false;
}

/**
 * *******************************************************************
 * @brief   update status values of KM271
 * @param   none 
 * @return  none
 * *******************************************************************/
void updateKm271Status(){

  pkmStatus = km271GetStatusValueAdr();

  if (kmStatusCpy.HC1_OperatingStates_1 != pkmStatus->HC1_OperatingStates_1) {
    kmStatusCpy.HC1_OperatingStates_1 = pkmStatus->HC1_OperatingStates_1;
    
    // AUTOMATIC / MANUAL (Day/Night)
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) {              // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s ", webText.AUTOMATIC[config.lang]);
    }
    else {                                                            // MANUAL
      if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)) {            // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      }
      else {                                                          // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    updateWebText("p01_hc1_opmode", tmpMessage, false);
    
    // Summer / Winter
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) {              // AUTOMATIC
      updateWebText("p01_hc1_summer_winter", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);  
    }
    else { // generate status from actual temperature and summer threshold
      updateWebText("p01_hc1_summer_winter", (kmStatusCpy.OutsideDampedTemp > kmConfigNumCpy.hc1_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
    }
    updateWebText("p01_hc1_summer_winter", tmpMessage, false);

    updateWebText("p03_hc1_ov1_off_time_optimization", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0)), false); 
    updateWebText("p03_hc1_ov1_on_time_optimization", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1)), false); 
    updateWebText("p03_hc1_ov1_automatic", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)), false);  
    updateWebText("p03_hc1_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)), false);  
    updateWebText("p03_hc1_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)), false);  
    updateWebText("p03_hc1_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)), false);     
    updateWebText("p03_hc1_ov1_frost_protection", onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)), false); 

  } else if (kmStatusCpy.HC1_OperatingStates_2 != pkmStatus->HC1_OperatingStates_2) {
    kmStatusCpy.HC1_OperatingStates_2 = pkmStatus->HC1_OperatingStates_2;

    // Day / Night
    updateWebText("p01_hc1_day_night", (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]), false);

    updateWebText("p03_hc1_ov2_summer",  onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0)), false); 
    updateWebText("p03_hc1_ov2_day",  onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)), false); 
    updateWebText("p03_hc1_ov2_no_conn_to_remotectrl",  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2)), false); 
    updateWebText("p03_hc1_ov2_remotectrl_error",  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)), false);
    updateWebText("p03_hc1_ov2_failure_flow_sensor",  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)), false);
    updateWebText("p03_hc1_ov2_flow_at_maximum",  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)), false);
    updateWebText("p03_hc1_ov2_external_signal_input",  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6)), false); 


  } else if (kmStatusCpy.HC1_HeatingForwardTargetTemp != pkmStatus->HC1_HeatingForwardTargetTemp) {
    kmStatusCpy.HC1_HeatingForwardTargetTemp = pkmStatus->HC1_HeatingForwardTargetTemp;
    updateWebTextInt("p01_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    updateWebTextInt("p03_hc1_flow_set", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);

  }
  else if (kmStatusCpy.HC1_HeatingForwardActualTemp != pkmStatus->HC1_HeatingForwardActualTemp) {
    kmStatusCpy.HC1_HeatingForwardActualTemp = pkmStatus->HC1_HeatingForwardActualTemp;
    updateWebTextInt("p01_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    updateWebTextInt("p03_hc1_flow_act", kmStatusCpy.HC1_HeatingForwardTargetTemp, false);
    
  }
  else if (kmStatusCpy.HC1_RoomTargetTemp != pkmStatus->HC1_RoomTargetTemp) {
    kmStatusCpy.HC1_RoomTargetTemp = pkmStatus->HC1_RoomTargetTemp;
    updateWebTextInt("p03_hc1_room_setpoint", kmStatusCpy.HC1_RoomTargetTemp, false);
  }
  else if (kmStatusCpy.HC1_RoomActualTemp != pkmStatus->HC1_RoomActualTemp) {
    kmStatusCpy.HC1_RoomActualTemp = pkmStatus->HC1_RoomActualTemp;
    updateWebTextInt("p03_hc1_room_temp", kmStatusCpy.HC1_RoomActualTemp, false);
  }
  else if (kmStatusCpy.HC1_SwitchOnOptimizationTime != pkmStatus->HC1_SwitchOnOptimizationTime) {
    kmStatusCpy.HC1_SwitchOnOptimizationTime = pkmStatus->HC1_SwitchOnOptimizationTime;
    updateWebTextInt("p03_hc1_on_time_opt_duration", kmStatusCpy.HC1_SwitchOnOptimizationTime, false);
  }
  else if (kmStatusCpy.HC1_SwitchOffOptimizationTime != pkmStatus->HC1_SwitchOffOptimizationTime) {
    kmStatusCpy.HC1_SwitchOffOptimizationTime = pkmStatus->HC1_SwitchOffOptimizationTime;
    updateWebTextInt("p03_hc1_off_time_opt_duration", kmStatusCpy.HC1_SwitchOffOptimizationTime, false);
  }
  else if (kmStatusCpy.HC1_PumpPower != pkmStatus->HC1_PumpPower) {
    kmStatusCpy.HC1_PumpPower = pkmStatus->HC1_PumpPower;
    updateWebText("p01_hc1_pump", (kmStatusCpy.HC1_PumpPower==0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false); 
    updateWebTextInt("p03_hc1_pump", kmStatusCpy.HC1_PumpPower, false);   
  }
  else if (kmStatusCpy.HC1_MixingValue != pkmStatus->HC1_MixingValue) {
    kmStatusCpy.HC1_MixingValue = pkmStatus->HC1_MixingValue;
    updateWebTextInt("p03_hc1_mixer", kmStatusCpy.HC1_MixingValue, false);   
  }
  else if (kmStatusCpy.HC1_HeatingCurvePlus10 != pkmStatus->HC1_HeatingCurvePlus10) {
    kmStatusCpy.HC1_HeatingCurvePlus10 = pkmStatus->HC1_HeatingCurvePlus10;
    updateWebTextInt("p03_hc1_heat_curve_10C", kmStatusCpy.HC1_HeatingCurvePlus10, false);   
  }
  else if (kmStatusCpy.HC1_HeatingCurve0 != pkmStatus->HC1_HeatingCurve0) {
    kmStatusCpy.HC1_HeatingCurve0 = pkmStatus->HC1_HeatingCurve0;
    updateWebTextInt("p03_hc1_heat_curve_0C", kmStatusCpy.HC1_HeatingCurve0, false);
  }
  else if (kmStatusCpy.HC1_HeatingCurveMinus10 != pkmStatus->HC1_HeatingCurveMinus10) {
    kmStatusCpy.HC1_HeatingCurveMinus10 = pkmStatus->HC1_HeatingCurveMinus10;
    updateWebTextInt("p03_hc1_heat_curve_-10C", kmStatusCpy.HC1_HeatingCurveMinus10, false);   
  }
  else if (kmStatusCpy.HC2_OperatingStates_1 != pkmStatus->HC2_OperatingStates_1) {
    kmStatusCpy.HC2_OperatingStates_1 = pkmStatus->HC2_OperatingStates_1;
    // HC2-Operating State
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) {              // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s", webText.AUTOMATIC[config.lang]);
    }
    else {                                                            // MANUAL
      if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)) {            // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      }
      else {                                                          // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    updateWebText("p01_hc2_opmode", tmpMessage, false);

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) {              // AUTOMATIC
      updateWebText("p01_hc2_summer_winter", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0) ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
    }
    else { // generate status from actual temperature and summer threshold
      updateWebText("p01_hc2_summer_winter", (kmStatusCpy.OutsideDampedTemp > kmConfigNumCpy.hc2_summer_mode_threshold ? webText.SUMMER[config.lang] : webText.WINTER[config.lang]), false);
    
    }
    updateWebText("p01_hc2_summer_winter", tmpMessage, false);

    updateWebText("p04_hc2_ov1_off_time_optimization", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0)), false); 
    updateWebText("p04_hc2_ov1_on_time_optimization", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1)), false); 
    updateWebText("p04_hc2_ov1_automatic", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)), false);  
    updateWebText("p04_hc2_ov1_ww_priority", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)), false);  
    updateWebText("p04_hc2_ov1_screed_drying", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)), false);  
    updateWebText("p04_hc2_ov1_holiday", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)), false);     
    updateWebText("p04_hc2_ov1_frost_protection", onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)), false); 

  }
  else if (kmStatusCpy.HC2_OperatingStates_2 != pkmStatus->HC2_OperatingStates_2) {
    kmStatusCpy.HC2_OperatingStates_2 = pkmStatus->HC2_OperatingStates_2;
    // Day / Night
    updateWebText("p01_hc2_day_night", (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1) ? webText.DAY[config.lang] : webText.NIGHT[config.lang]), false);

    updateWebText("p04_hc2_ov2_summer",  onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0)), false); 
    updateWebText("p04_hc2_ov2_day",  onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)), false); 
    updateWebText("p04_hc2_ov2_no_conn_to_remotectrl",  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2)), false); 
    updateWebText("p04_hc2_ov2_remotectrl_error",  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)), false);
    updateWebText("p04_hc2_ov2_failure_flow_sensor",  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)), false);
    updateWebText("p04_hc2_ov2_flow_at_maximum",  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)), false);
    updateWebText("p04_hc2_ov2_external_signal_input",  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6)), false); 

  }
  else if (kmStatusCpy.HC2_HeatingForwardTargetTemp != pkmStatus->HC2_HeatingForwardTargetTemp) {
    kmStatusCpy.HC2_HeatingForwardTargetTemp = pkmStatus->HC2_HeatingForwardTargetTemp;
    updateWebTextInt("p01_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
    updateWebTextInt("p04_hc2_flow_set", kmStatusCpy.HC2_HeatingForwardTargetTemp, false);
  }
  else if (kmStatusCpy.HC2_HeatingForwardActualTemp != pkmStatus->HC2_HeatingForwardActualTemp) {
    kmStatusCpy.HC2_HeatingForwardActualTemp = pkmStatus->HC2_HeatingForwardActualTemp;
    updateWebTextInt("p01_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
    updateWebTextInt("p04_hc2_flow_act", kmStatusCpy.HC2_HeatingForwardActualTemp, false);
  }
  else if (kmStatusCpy.HC2_RoomTargetTemp != pkmStatus->HC2_RoomTargetTemp) {
    kmStatusCpy.HC2_RoomTargetTemp = pkmStatus->HC2_RoomTargetTemp;
    updateWebTextInt("p03_hc2_room_setpoint", kmStatusCpy.HC2_RoomTargetTemp, false);
  }
  else if (kmStatusCpy.HC2_RoomActualTemp != pkmStatus->HC2_RoomActualTemp) {
    kmStatusCpy.HC2_RoomActualTemp = pkmStatus->HC2_RoomActualTemp;
    updateWebTextInt("p03_hc2_room_temp", kmStatusCpy.HC2_RoomActualTemp, false);
  }
  else if (kmStatusCpy.HC2_SwitchOnOptimizationTime != pkmStatus->HC2_SwitchOnOptimizationTime) {
    kmStatusCpy.HC2_SwitchOnOptimizationTime = pkmStatus->HC2_SwitchOnOptimizationTime;
    updateWebTextInt("p04_hc2_on_time_opt_duration", kmStatusCpy.HC2_SwitchOnOptimizationTime, false);
  }
  else if (kmStatusCpy.HC2_SwitchOffOptimizationTime != pkmStatus->HC2_SwitchOffOptimizationTime) {
    kmStatusCpy.HC2_SwitchOffOptimizationTime = pkmStatus->HC2_SwitchOffOptimizationTime;
    updateWebTextInt("p04_hc2_off_time_opt_duration", kmStatusCpy.HC2_SwitchOffOptimizationTime, false);
  }
  else if (kmStatusCpy.HC2_PumpPower != pkmStatus->HC2_PumpPower) {
    kmStatusCpy.HC2_PumpPower = pkmStatus->HC2_PumpPower;
    updateWebText("p01_hc2_pump", (kmStatusCpy.HC2_PumpPower==0 ? webText.OFF[config.lang] : webText.ON[config.lang]), false); 
    updateWebTextInt("p04_hc2_pump", kmStatusCpy.HC2_PumpPower, false);
  }
  else if (kmStatusCpy.HC2_MixingValue != pkmStatus->HC2_MixingValue) {
    kmStatusCpy.HC2_MixingValue = pkmStatus->HC2_MixingValue;
    updateWebTextInt("p04_hc2_mixer", kmStatusCpy.HC2_MixingValue, false);
  }
  else if (kmStatusCpy.HC2_HeatingCurvePlus10 != pkmStatus->HC2_HeatingCurvePlus10) {
    kmStatusCpy.HC2_HeatingCurvePlus10 = pkmStatus->HC2_HeatingCurvePlus10;
    updateWebTextInt("p04_hc2_heat_curve_10C", kmStatusCpy.HC2_HeatingCurvePlus10, false);
  }
  else if (kmStatusCpy.HC2_HeatingCurve0 != pkmStatus->HC2_HeatingCurve0) {
    kmStatusCpy.HC2_HeatingCurve0 = pkmStatus->HC2_HeatingCurve0;
    updateWebTextInt("p04_hc2_heat_curve_0C", kmStatusCpy.HC2_HeatingCurve0, false);
  }
  else if (kmStatusCpy.HC2_HeatingCurveMinus10 != pkmStatus->HC2_HeatingCurveMinus10) {
    kmStatusCpy.HC2_HeatingCurveMinus10 = pkmStatus->HC2_HeatingCurveMinus10;
    updateWebTextInt("p04_hc2_heat_curve_-10C", kmStatusCpy.HC2_HeatingCurveMinus10, false);  
  }
  else if (kmStatusCpy.HotWaterOperatingStates_1 != pkmStatus->HotWaterOperatingStates_1) {
    kmStatusCpy.HotWaterOperatingStates_1 = pkmStatus->HotWaterOperatingStates_1;
    // WW-Operating State
    if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) {       // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s", webText.AUTOMATIC[config.lang]);
      }
    else {                                                         // MANUAL
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5))       // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      else {                                                       // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "%s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    updateWebText("p01_ww_opmode", tmpMessage, false); 

    updateWebText("p05_hw_ov1_auto",  onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)), false);   
    updateWebText("p05_hw_ov1_disinfection",  onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)), false);  
    updateWebText("p05_hw_ov1_reload",  onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)), false);  
    updateWebText("p05_hw_ov1_holiday",  onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)), false);  
    updateWebText("p05_hw_ov1_err_disinfection",  errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)), false);  
    updateWebText("p05_hw_ov1_err_sensor",  errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)), false);  
    updateWebText("p05_hw_ov1_stays_cold",  errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)), false);  
    updateWebText("p05_hw_ov1_err_anode",  errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)), false); 

  }
  else if (kmStatusCpy.HotWaterOperatingStates_2 != pkmStatus->HotWaterOperatingStates_2) {
    kmStatusCpy.HotWaterOperatingStates_2 = pkmStatus->HotWaterOperatingStates_2;

    updateWebText("p05_hw_ov2_load",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)), false);   
    updateWebText("p05_hw_ov2_manual",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)), false);  
    updateWebText("p05_hw_ov2_reload",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)), false);  
    updateWebText("p05_hw_ov2_off_time_opt_duration",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)), false);  
    updateWebText("p05_hw_ov2_on_time_opt_duration",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)), false);  
    updateWebText("p05_hw_ov2_day",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)), false);  
    updateWebText("p05_hw_ov2_hot",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)), false);  
    updateWebText("p05_hw_ov2_priority",   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)), false); 

  }
  else if (kmStatusCpy.HotWaterTargetTemp != pkmStatus->HotWaterTargetTemp) {
    kmStatusCpy.HotWaterTargetTemp = pkmStatus->HotWaterTargetTemp;
    updateWebTextInt("p05_hw_set_temp", kmStatusCpy.HotWaterTargetTemp, false); 
    updateWebTextInt("p01_ww_temp_set", kmStatusCpy.HotWaterTargetTemp, false); 
  }
  else if (kmStatusCpy.HotWaterActualTemp != pkmStatus->HotWaterActualTemp) {
    kmStatusCpy.HotWaterActualTemp = pkmStatus->HotWaterActualTemp;
    updateWebTextInt("p05_hw_act_temp", kmStatusCpy.HotWaterActualTemp, false);
    updateWebTextInt("p01_ww_temp_act", kmStatusCpy.HotWaterActualTemp, false);
  }
  else if (kmStatusCpy.HotWaterOptimizationTime != pkmStatus->HotWaterOptimizationTime) {
    kmStatusCpy.HotWaterOptimizationTime = pkmStatus->HotWaterOptimizationTime;
    updateWebText("p05_hw_on_time_opt_duration", onOffString(kmStatusCpy.HotWaterOptimizationTime), false);
  }
  else if (kmStatusCpy.HotWaterPumpStates != pkmStatus->HotWaterPumpStates) {
    kmStatusCpy.HotWaterPumpStates = pkmStatus->HotWaterPumpStates;
    updateWebText("p05_hw_pump_type_charge", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)), false);
    updateWebText("p05_hw_pump_type_circulation", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)), false);
    updateWebText("p05_hw_pump_type_groundwater_solar", onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)), false);
  }
  else if (kmStatusCpy.BoilerForwardTargetTemp != pkmStatus->BoilerForwardTargetTemp) {
    kmStatusCpy.BoilerForwardTargetTemp = pkmStatus->BoilerForwardTargetTemp;
    updateWebTextInt("p06_boiler_setpoint", kmStatusCpy.BoilerForwardTargetTemp, false);
    updateWebTextInt("p01_burner_temp_set", kmStatusCpy.BoilerForwardTargetTemp, false);
  }
  else if (kmStatusCpy.BoilerForwardActualTemp != pkmStatus->BoilerForwardActualTemp) {
    kmStatusCpy.BoilerForwardActualTemp = pkmStatus->BoilerForwardActualTemp;
    updateWebTextInt("p06_boiler_temp", kmStatusCpy.BoilerForwardActualTemp, false);
    updateWebTextInt("p01_burner_temp_act", kmStatusCpy.BoilerForwardActualTemp, false);
  }
  else if (kmStatusCpy.BurnerSwitchOnTemp != pkmStatus->BurnerSwitchOnTemp) {
    kmStatusCpy.BurnerSwitchOnTemp = pkmStatus->BurnerSwitchOnTemp;
    updateWebTextInt("p06_boiler_switch_on_temp", kmStatusCpy.BurnerSwitchOnTemp, false);
  }
  else if (kmStatusCpy.BurnerSwitchOffTemp != pkmStatus->BurnerSwitchOffTemp) {
    kmStatusCpy.BurnerSwitchOffTemp = pkmStatus->BurnerSwitchOffTemp;
    updateWebTextInt("p06_boiler_switch_off_temp", kmStatusCpy.BurnerSwitchOffTemp, false);
  }
  else if (kmStatusCpy.BoilerIntegral_1 != pkmStatus->BoilerIntegral_1) {
    kmStatusCpy.BoilerIntegral_1 = pkmStatus->BoilerIntegral_1;
  }
  else if (kmStatusCpy.BoilerIntegral_2 != pkmStatus->BoilerIntegral_2) {
    kmStatusCpy.BoilerIntegral_2 = pkmStatus->BoilerIntegral_2;
  }
  else if (kmStatusCpy.BoilerErrorStates != pkmStatus->BoilerErrorStates) {
    kmStatusCpy.BoilerErrorStates = pkmStatus->BoilerErrorStates;

    updateWebText("p06_boiler_failure_burner", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)), false);
    updateWebText("p06_boiler_failure_boiler_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)), false);
    updateWebText("p06_boiler_failure_aux_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)), false);
    updateWebText("p06_boiler_failure_boiler_stays_cold", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)), false);
    updateWebText("p06_boiler_failure_exhaust_gas_sensor", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)), false);
    updateWebText("p06_boiler_failure_exhaust_gas_over_limit", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)), false);
    updateWebText("p06_boiler_failure_safety_chain", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)), false);
    updateWebText("p06_boiler_failure_external", errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)), false);

  }
  else if (kmStatusCpy.BoilerOperatingStates != pkmStatus->BoilerOperatingStates) {
    kmStatusCpy.BoilerOperatingStates = pkmStatus->BoilerOperatingStates;

    updateWebText("p06_boiler_state_exhaust_gas_test", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)), false);
    updateWebText("p06_boiler_state_stage1", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)), false);
    updateWebText("p06_boiler_state_boiler_protection", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)), false);
    updateWebText("p06_boiler_state_active", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)), false);
    updateWebText("p06_boiler_state_performance_free", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4)), false);
    updateWebText("p06_boiler_state_performance_high", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5)), false);
    updateWebText("p06_boiler_state_stage2", onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)), false);

  }
  else if (kmStatusCpy.BurnerStates != pkmStatus->BurnerStates) {
    kmStatusCpy.BurnerStates = pkmStatus->BurnerStates;
    updateWebText("p01_burner", (kmStatusCpy.BurnerStates==0)?webText.OFF[config.lang]:webText.ON[config.lang], false);
    updateWebText("p06_burner_control", cfgArrayTexts.BURNER_STATE[config.lang][kmStatusCpy.BurnerStates], false);

  }
  else if (kmStatusCpy.ExhaustTemp != pkmStatus->ExhaustTemp) {
    kmStatusCpy.ExhaustTemp = pkmStatus->ExhaustTemp;
    updateWebTextInt("p07_exhaust_gas_temp", kmStatusCpy.ExhaustTemp, false);
  }
  else if (kmStatusCpy.BurnerOperatingDuration_2 != pkmStatus->BurnerOperatingDuration_2) {
    kmStatusCpy.BurnerOperatingDuration_2 = pkmStatus->BurnerOperatingDuration_2;
    updateWebTextInt("p06_burner_runtime_minutes65536", kmStatusCpy.BurnerOperatingDuration_2, false);
  }
  else if (kmStatusCpy.BurnerOperatingDuration_1 != pkmStatus->BurnerOperatingDuration_1) {
    kmStatusCpy.BurnerOperatingDuration_1 = pkmStatus->BurnerOperatingDuration_1;
    updateWebTextInt("p06_burner_runtime_minutes256", kmStatusCpy.BurnerOperatingDuration_1, false);
  }
  else if (kmStatusCpy.BurnerOperatingDuration_0 != pkmStatus->BurnerOperatingDuration_0) {
    kmStatusCpy.BurnerOperatingDuration_0 = pkmStatus->BurnerOperatingDuration_0;
    updateWebTextInt("p06_burner_runtime_minutes", kmStatusCpy.BurnerOperatingDuration_0, false);
  }
  else if (kmStatusCpy.BurnerOperatingDuration_Sum != pkmStatus->BurnerOperatingDuration_Sum) {
    kmStatusCpy.BurnerOperatingDuration_Sum = pkmStatus->BurnerOperatingDuration_Sum;
    updateWebTextInt("p06_burner_runtime_overall", kmStatusCpy.BurnerOperatingDuration_Sum, false);
  }
  else if (kmStatusCpy.BurnerCalcOilConsumption != pkmStatus->BurnerCalcOilConsumption) {
    kmStatusCpy.BurnerCalcOilConsumption = pkmStatus->BurnerCalcOilConsumption;
    //ToDo
  }
  else if (kmStatusCpy.OutsideTemp != pkmStatus->OutsideTemp) {
    kmStatusCpy.OutsideTemp = pkmStatus->OutsideTemp;
    updateWebTextInt("p07_outside_temp", kmStatusCpy.OutsideTemp, false);
    updateWebTextInt("p01_temp_out_act", kmStatusCpy.OutsideTemp, false);
  }
  else if (kmStatusCpy.OutsideDampedTemp!= pkmStatus->OutsideDampedTemp) {
    kmStatusCpy.OutsideDampedTemp = pkmStatus->OutsideDampedTemp;
    updateWebTextInt("p07_outside_temp_damped", kmStatusCpy.OutsideDampedTemp, false);
    updateWebTextInt("p01_temp_out_dmp", kmStatusCpy.OutsideDampedTemp, false);
  }
  else if (kmStatusCpy.ControllerVersionMain != pkmStatus->ControllerVersionMain) {
    kmStatusCpy.ControllerVersionMain = pkmStatus->ControllerVersionMain;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  }
  else if (kmStatusCpy.ControllerVersionSub != pkmStatus->ControllerVersionSub) {
    kmStatusCpy.ControllerVersionSub = pkmStatus->ControllerVersionSub;
    snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
    updateWebText("p09_logamatic_version", tmpMessage, false);
  }
  else if (kmStatusCpy.Modul != pkmStatus->Modul) {
    kmStatusCpy.Modul = pkmStatus->Modul;
    updateWebTextInt("p09_logamatic_modul", kmStatusCpy.ControllerVersionSub, false);

    // TODO
    updateKmConfig = true;
  }
  else if (kmStatusCpy.ERR_Alarmstatus != pkmStatus->ERR_Alarmstatus) {
    kmStatusCpy.ERR_Alarmstatus = pkmStatus->ERR_Alarmstatus;
    // ToDo
  }

}



/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUICylic(){

  updateKm271Status();

  if(updateKmConfig){
    updateKm271Config();
  }

  if (simulationTimer.delayOn(clientConnected && !bootInit, 2000))
  {
    bootInit = true;
    startSimData();
    updateWebText("p00_version", "4.x.x", false);
    updateKm271Config();
  }

  if (connectionTimer.cycleTrigger(2000))
  {
      events.send("ping", "ping", millis());
  }

  

}
