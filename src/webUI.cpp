#include <webUI.h>
#include <km271.h>
#include <language.h>
#include <basics.h>
#include <oilmeter.h>

/* P R O T O T Y P E S ********************************************************/ 
void updateDashboard();
void updateStatusValues();
void updateConfigValues();
void updateSystemInfo();
void updateOilmeter();
void updateAlarmTab();
void generalCallback(Control *sender, int type);
void updateSettingsValues();

/* D E C L A R A T I O N S ****************************************************/  
s_webui_id id;
s_webui_texts webText;
s_stat_topics km271StatTopics;
s_cfg_topics km271CfgTopics;
s_cfg_arrays cfgArrayTexts;

s_km271_status      kmStatusCpy; 
s_km271_config_str  kmConfigStrCpy;
s_km271_config_num  kmConfigNumCpy;
s_km271_alarm_str   kmAlarmMsgCpy;

muTimer valeCompareTimer = muTimer();     // timer to refresh id.tab.config
s_config configSave;

char tmpMessage[300]={'\0'};              // temp string
long oilcounter, oilcounterOld;           // actual and old oilcounter value
const char* LANGUAGES[MAX_LANG] = {"🇩🇪 Deutsch", "🇬🇧 English"};
const char* BOARDS[4] = {"select Board...", "generic ESP32", "KM271-WiFi v0.0.5", "KM271-WiFi v0.0.6"};
char hc_opmode_optval[3][100]={'\0'};
char elementBuffer[5000]={'\0'};          // Buffer to create table content
char elementString[500]={'\0'};           // temp string
IPAddress tmpIpAddress;                   // temporary IP address

// ======================================================
// Helper functions
// ======================================================
const char* addUnit(const char* input, const char* unit){
  static char ret_str[128];
  snprintf(ret_str, sizeof(ret_str), "%s %s", input, unit);
  return ret_str;
}

void initElements(){
  snprintf(elementBuffer, sizeof(elementBuffer), "");
}
void addElement(const char* label, const char* value){
  snprintf(elementString, sizeof(elementString), "<span class=\"d60\">%s</span><span class=\"v40\">%s</span>",  label, value);
  strcat(elementBuffer, elementString);
}
void addElementUnit(const char* label, const char* value, const char* unit){
  snprintf(elementString, sizeof(elementString), "<span class=\"d60\">%s</span><span class=\"v30\">%s</span><span class=\"u10\">%s</span>",  label, value, unit);
  strcat(elementBuffer, elementString);
}
void addElementWide(const char* label, const char* value){
  snprintf(elementString, sizeof(elementString), "<span class=\"d30\">%s</span><span class=\"v70\">%s</span>",  label, value);
  strcat(elementBuffer, elementString);
}
uint16_t addEmtyElement(uint16_t parent){
  uint16_t id = ESPUI.addControl(ControlType::Label, "", "--", ControlColor::None, parent);
  ESPUI.setElementStyle(id, "background-color: unset; width: 100%");
  return id;
}
void updateElements(uint16_t parent){
  ESPUI.updateLabel(parent, elementBuffer);
}

/**
 * *******************************************************************
 * @brief   helper function to add new Group
 * @param   title group title
 * @param   color group color
 * @param   tab parent tab
 * @return  none
 * *******************************************************************/
uint16_t addGroupHelper(const char * title, ControlColor color, uint16_t tab) {
	auto groupID = ESPUI.addControl(Label, title, "", color, tab);
	ESPUI.setElementStyle(groupID, LABLE_STYLE_GROUP);
  return groupID;
}

/**
 * *******************************************************************
 * @brief   helper function to add status value to a group
 * @param   title   value title
 * @param   value   value
 * @param   unit    unit of value
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addGroupValueHelper(const char * title, String value, String unit, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_CLEAR);
	uint16_t value_id = ESPUI.addControl(Label, "", value, None, group);
  ESPUI.setElementStyle(value_id, LABLE_STYLE_VALUE);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", unit, None, group), LABLE_STYLE_UNIT);
  return value_id;
}

/**
 * *******************************************************************
 * @brief   helper function to add text input field to a group
 * @param   title   value title
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addTextInputHelper(const char * title, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_INPUT_LABEL);
	uint16_t value_id = ESPUI.addControl(Text, "", "", Dark, group, generalCallback);
  ESPUI.setElementStyle(value_id, LABLE_STYLE_INPUT_TEXT);
  return value_id;
}

/**
 * *******************************************************************
 * @brief   helper function to add number input field to a group
 * @param   title   value title
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addNumberInputHelper(const char * title, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_NUMER_LABEL);
  uint16_t value_id = ESPUI.addControl(Number, "", "0", Dark, group, generalCallback);
  return value_id;
}

/**
 * *******************************************************************
 * @brief   helper function to add switcher field to a group
 * @param   title   value title
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addSwitcherInputHelper(const char * title, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_SWITCH_LABEL);
  uint16_t value_id = ESPUI.addControl(Switcher, "", "", Dark, group, generalCallback);
  return value_id;
}

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

/**
 * *******************************************************************
 * @brief   Elements for Dashboard Tab
 * *******************************************************************/
void addDashboardTab(){
    
  id.tab.dashboard = ESPUI.addControl(Tab, "", webText.DASHBORAD[config.lang], ControlColor::None, 0, generalCallback);

  // HC1 OperationMode
  if (config.km271.use_hc1) {
    auto hc1_opmode_Group = addGroupHelper(webText.HC1_OPMODE[config.lang], Dark, id.tab.dashboard);
    id.dash.hc1_opmode = ESPUI.addControl(Label, "", "--", None, hc1_opmode_Group);
    ESPUI.setElementStyle(id.dash.hc1_opmode, LABLE_STYLE_DASH);
  }
  // HC2 OperationMode
  if (config.km271.use_hc2) {
    auto hc2_opmode_Group = addGroupHelper(webText.HC2_OPMODE[config.lang], Dark, id.tab.dashboard);
    id.dash.hc2_opmode = ESPUI.addControl(Label, "", "--", None, hc2_opmode_Group);
    ESPUI.setElementStyle(id.dash.hc2_opmode, LABLE_STYLE_DASH);
  }

  // WW OperationMode
  if (config.km271.use_ww) {
    auto ww_opmode_Group = addGroupHelper(webText.WW_OPMODE[config.lang], Dark, id.tab.dashboard);
    id.dash.ww_opmode = ESPUI.addControl(Label, "", "--", None, ww_opmode_Group);
    ESPUI.setElementStyle(id.dash.ww_opmode, LABLE_STYLE_DASH);
  }
  // HC Day-Night
  if (config.km271.use_hc1) {
    auto hc1dayNight_Group = addGroupHelper(webText.HC1_DAY_NIGHT[config.lang], Dark, id.tab.dashboard);
    id.dash.hc1dayNight = ESPUI.addControl(Label, "", "--", None, hc1dayNight_Group);
    ESPUI.setElementStyle(id.dash.hc1dayNight, LABLE_STYLE_DASH);
  }
  if (config.km271.use_hc2) {
    auto hc2dayNight_Group = addGroupHelper(webText.HC2_DAY_NIGHT[config.lang], Dark, id.tab.dashboard);
    id.dash.hc2dayNight = ESPUI.addControl(Label, "", "--", None, hc2dayNight_Group);
    ESPUI.setElementStyle(id.dash.hc2dayNight, LABLE_STYLE_DASH);
  }
  
  // HC Summer-Winter
  if (config.km271.use_hc1) {
    auto hc1summerWinter_Group = addGroupHelper(webText.HC1_SUMMER_WINTER[config.lang], Dark, id.tab.dashboard);
    id.dash.hc1summerWinter = ESPUI.addControl(Label, "", "--", None, hc1summerWinter_Group);
    ESPUI.setElementStyle(id.dash.hc1summerWinter, LABLE_STYLE_DASH);
  }
  if (config.km271.use_hc2) {
    auto hc2summerWinter_Group = addGroupHelper(webText.HC2_SUMMER_WINTER[config.lang], Dark, id.tab.dashboard);
    id.dash.hc2summerWinter = ESPUI.addControl(Label, "", "--", None, hc2summerWinter_Group);
    ESPUI.setElementStyle(id.dash.hc2summerWinter, LABLE_STYLE_DASH);
  }

  // Burner
  auto burner_Group = addGroupHelper(webText.BURNER[config.lang], Dark, id.tab.dashboard);
  id.dash.burnerState = ESPUI.addControl(Label, "", "--", None, burner_Group);
  ESPUI.setElementStyle(id.dash.burnerState, LABLE_STYLE_DASH);

  // HC Pump
  if (config.km271.use_hc1) {
    auto hc1_pump_Group = addGroupHelper(webText.HC1_PUMP[config.lang], Dark, id.tab.dashboard);
    id.dash.hc1pumpState = ESPUI.addControl(Label, "", "--", None, hc1_pump_Group);
    ESPUI.setElementStyle(id.dash.hc1pumpState, LABLE_STYLE_DASH);
  }
  if (config.km271.use_hc2) {
    auto hc2_pump_Group = addGroupHelper(webText.HC2_PUMP[config.lang], Dark, id.tab.dashboard);
    id.dash.hc2pumpState = ESPUI.addControl(Label, "", "--", None, hc2_pump_Group);
    ESPUI.setElementStyle(id.dash.hc2pumpState, LABLE_STYLE_DASH);
  }

  // WW-Temp
  if (config.km271.use_ww) {
    auto wwTemp_Group = addGroupHelper(webText.WW[config.lang], Dark, id.tab.dashboard);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.SET_TEMP_C[config.lang], None, wwTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.ACT_TEMP_C[config.lang], None, wwTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    id.dash.wwSetTemp = ESPUI.addControl(Label, "", "--", None, wwTemp_Group);
    ESPUI.setElementStyle(id.dash.wwSetTemp, "width: 45%; font-size: 30px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, wwTemp_Group), "background-color: unset; width: 10%;"); //spacer
    id.dash.wwActTemp = ESPUI.addControl(Label, "", "--", None, wwTemp_Group);
    ESPUI.setElementStyle(id.dash.wwActTemp, "width: 45%; font-size: 30px");
  }
  // Burner-Temp
  auto burnerTemp_Group = addGroupHelper(webText.BURNER_TEMP[config.lang], Dark, id.tab.dashboard);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.SET_TEMP_C[config.lang], None, burnerTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.ACT_TEMP_C[config.lang], None, burnerTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  id.dash.burnerSetTemp = ESPUI.addControl(Label, "", "--", None, burnerTemp_Group);
  ESPUI.setElementStyle(id.dash.burnerSetTemp, "width: 45%; font-size: 30px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, burnerTemp_Group), "background-color: unset; width: 10%;");
  id.dash.burnerActTemp = ESPUI.addControl(Label, "", "--", None, burnerTemp_Group);
  ESPUI.setElementStyle(id.dash.burnerActTemp, "width: 45%; font-size: 30px");
  // Outdoor-Temp
  auto outdoorTemp_Group = addGroupHelper(webText.TEMP_OUT[config.lang], Dark, id.tab.dashboard);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.TEMP_OUT_ACT[config.lang], None, outdoorTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.TEMP_OUT_DMP[config.lang], None, outdoorTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  id.dash.tmp_out_act = ESPUI.addControl(Label, "", "--", None, outdoorTemp_Group);
  ESPUI.setElementStyle(id.dash.tmp_out_act, "width: 45%; font-size: 30px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, outdoorTemp_Group), "background-color: unset; width: 10%;");
  id.dash.tmp_out_act_d = ESPUI.addControl(Label, "", "--", None, outdoorTemp_Group);
  ESPUI.setElementStyle(id.dash.tmp_out_act_d, "width: 45%; font-size: 30px");

  // HC Flow-Temp
  if (config.km271.use_hc1) {
    auto hc1flowTemp_Group = addGroupHelper(webText.HC1_FLOW[config.lang], Dark, id.tab.dashboard);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.SET_TEMP_C[config.lang], None, hc1flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.ACT_TEMP_C[config.lang], None, hc1flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    id.dash.hc1flowSetTemp = ESPUI.addControl(Label, "", "--", None, hc1flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc1flowSetTemp, "width: 45%; font-size: 30px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1flowTemp_Group), "background-color: unset; width: 10%;");
    id.dash.hc1flowActTemp = ESPUI.addControl(Label, "", "--", None, hc1flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc1flowActTemp, "width: 45%; font-size: 30px");
  }
  if (config.km271.use_hc2) {
    auto hc2flowTemp_Group = addGroupHelper(webText.HC2_FLOW[config.lang], Dark, id.tab.dashboard);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.SET_TEMP_C[config.lang], None, hc2flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.ACT_TEMP_C[config.lang], None, hc2flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    id.dash.hc2flowSetTemp = ESPUI.addControl(Label, "", "--", None, hc2flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc2flowSetTemp, "width: 45%; font-size: 30px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2flowTemp_Group), "background-color: unset; width: 10%;");
    id.dash.hc2flowActTemp = ESPUI.addControl(Label, "", "--", None, hc2flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc2flowActTemp, "width: 45%; font-size: 30px");
  }

  // Oilmeter
  if (config.oilmeter.use_hardware_meter || config.oilmeter.use_virtual_meter) {
    auto oilmeter_Group = addGroupHelper(webText.OILMETER[config.lang], Dark, id.tab.dashboard);
    id.dash.oilmeter = ESPUI.addControl(Label, "", "--", None, oilmeter_Group);
    ESPUI.setElementStyle(id.dash.oilmeter, LABLE_STYLE_DASH);
  }
} // End addDashboardElements()


/**
 * *******************************************************************
 * @brief   Elements for Control Tab
 * *******************************************************************/
 void addControlTab(){
    
  // array of operating modes for select control
  strcpy(hc_opmode_optval[0], webText.NIGHT[config.lang]);
  strcpy(hc_opmode_optval[1], webText.DAY[config.lang]);
  strcpy(hc_opmode_optval[2], webText.AUTOMATIC[config.lang]);

  id.tab.control = ESPUI.addControl(Tab, "", webText.CONTROL[config.lang], ControlColor::None, 0, generalCallback);
  ESPUI.addControl(ControlType::Separator, webText.OPMODES[config.lang], "", ControlColor::None, id.tab.control);

  // HC-Operation Mode
  if (config.km271.use_hc1)
  {
    id.ctrl.hc1_opmode = ESPUI.addControl(Select, km271CfgTopics.HC1_OPMODE[config.lang], "", Dark, id.tab.control, generalCallback);
    ESPUI.addControl(Option, hc_opmode_optval[0], "0", None, id.ctrl.hc1_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[1], "1", None, id.ctrl.hc1_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[2], "2", None, id.ctrl.hc1_opmode);
  }
  if (config.km271.use_hc2) {
    id.ctrl.hc2_opmode = ESPUI.addControl(Select, km271CfgTopics.HC2_OPMODE[config.lang], "", Dark, id.tab.control, generalCallback);
    ESPUI.addControl(Option, hc_opmode_optval[0], "0", None, id.ctrl.hc2_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[1], "1", None, id.ctrl.hc2_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[2], "2", None, id.ctrl.hc2_opmode);
  }
    
  // WW-Operation Mode
  if (config.km271.use_ww) {
    id.ctrl.ww_opmode = ESPUI.addControl(Select, km271CfgTopics.WW_OPMODE[config.lang], "", Dark, id.tab.control, generalCallback);
    ESPUI.addControl(Option, hc_opmode_optval[0], "0", None, id.ctrl.ww_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[1], "1", None, id.ctrl.ww_opmode);
    ESPUI.addControl(Option, hc_opmode_optval[2], "2", None, id.ctrl.ww_opmode);   
  }

  ESPUI.addControl(ControlType::Separator, webText.PROGRAMS[config.lang], "", ControlColor::None, id.tab.control);

  // HC-Program
  if (config.km271.use_hc1) {
    id.ctrl.hc1_program = ESPUI.addControl(Select, km271CfgTopics.HC1_PROGRAM[config.lang], "", Dark, id.tab.control, generalCallback);
    for (int i = 0; i < 9; i++) {
      ESPUI.addControl(Option, cfgArrayTexts.HC_PROGRAM[config.lang][i], int8ToString(i), None, id.ctrl.hc1_program);
    }
  }
  if (config.km271.use_hc2) {
    id.ctrl.hc2_program = ESPUI.addControl(Select, km271CfgTopics.HC2_PROGRAM[config.lang], "", Dark, id.tab.control, generalCallback);
    for (int i = 0; i < 9; i++) {
      ESPUI.addControl(Option, cfgArrayTexts.HC_PROGRAM[config.lang][i], int8ToString(i), None, id.ctrl.hc2_program);
    }
  }
  
  // HC-Holidays
  if (config.km271.use_hc1) {
    id.ctrl.hc1_holiday_days = ESPUI.addControl(Number, km271CfgTopics.HC1_HOLIDAY_DAYS[config.lang], "", Dark, id.tab.control, generalCallback);
  }
  if (config.km271.use_hc2) {
    id.ctrl.hc2_holiday_days = ESPUI.addControl(Number, km271CfgTopics.HC2_HOLIDAY_DAYS[config.lang], "", Dark, id.tab.control, generalCallback);
  }
  
  ESPUI.addControl(ControlType::Separator, webText.TEMPERATURES[config.lang], "", ControlColor::None, id.tab.control);

  // Frost Threshold
  if (config.km271.use_hc1) {
    auto frost_control = addGroupHelper(km271CfgTopics.HC1_FROST_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc1_frost_mode_threshold = ESPUI.addControl(Slider, "", "-20", Dark, frost_control, generalCallback);
    ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc1_frost_mode_threshold);
    ESPUI.addControl(Max, "", "10", None, id.ctrl.hc1_frost_mode_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_FROST[config.lang], None, frost_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, frost_control), LABLE_STYLE_DESCRIPTION);
  }
  if (config.km271.use_hc2) {
    auto frost_control = addGroupHelper(km271CfgTopics.HC2_FROST_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc2_frost_mode_threshold = ESPUI.addControl(Slider, "", "-20", Dark, frost_control, generalCallback);
    ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc2_frost_mode_threshold);
    ESPUI.addControl(Max, "", "10", None, id.ctrl.hc2_frost_mode_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_FROST[config.lang], None, frost_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, frost_control), LABLE_STYLE_DESCRIPTION);
  }
  
  // Summer Threshold
  if (config.km271.use_hc1) {
    auto summer_control = addGroupHelper(km271CfgTopics.HC1_SUMMER_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc1_summer_mode_threshold = ESPUI.addControl(Slider, "", "9", Dark, summer_control, generalCallback);
    ESPUI.addControl(Min, "", "9", None, id.ctrl.hc1_summer_mode_threshold);
    ESPUI.addControl(Max, "", "31", None, id.ctrl.hc1_summer_mode_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SUMMER1[config.lang], None, summer_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, " ", webText.INFO_SUMMER2[config.lang], None, summer_control), LABLE_STYLE_DESCRIPTION);
  }
  if (config.km271.use_hc2) {
    auto summer_control = addGroupHelper(km271CfgTopics.HC2_SUMMER_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc2_summer_mode_threshold = ESPUI.addControl(Slider, "", "9", Dark, summer_control, generalCallback);
    ESPUI.addControl(Min, "", "9", None, id.ctrl.hc2_summer_mode_threshold);
    ESPUI.addControl(Max, "", "31", None, id.ctrl.hc2_summer_mode_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SUMMER1[config.lang], None, summer_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, " ", webText.INFO_SUMMER2[config.lang], None, summer_control), LABLE_STYLE_DESCRIPTION);
  }
    
  // HC-DesignTemp
  if (config.km271.use_hc1) {
    auto hc1_designtemp_control = addGroupHelper(km271CfgTopics.HC1_INTERPR[config.lang], Dark, id.tab.control);
    id.ctrl.hc1_interpretation = ESPUI.addControl(Slider, "", "30", Dark, hc1_designtemp_control, generalCallback);
    ESPUI.addControl(Min, "", "30", None, id.ctrl.hc1_interpretation);
    ESPUI.addControl(Max, "", "90", None, id.ctrl.hc1_interpretation);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_DESIGNTEMP[config.lang], None, hc1_designtemp_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, hc1_designtemp_control), LABLE_STYLE_DESCRIPTION);
  }
  if (config.km271.use_hc2) {
    auto hc2_designtemp_control = addGroupHelper(km271CfgTopics.HC2_INTERPR[config.lang], Dark, id.tab.control);
    id.ctrl.hc2_interpretation = ESPUI.addControl(Slider, "", "30", Dark, hc2_designtemp_control, generalCallback);
    ESPUI.addControl(Min, "", "30", None, id.ctrl.hc2_interpretation);
    ESPUI.addControl(Max, "", "90", None, id.ctrl.hc2_interpretation);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_DESIGNTEMP[config.lang], None, hc2_designtemp_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, hc2_designtemp_control), LABLE_STYLE_DESCRIPTION);
  }

  // HC-Switch Off Theshold
  if (config.km271.use_hc1) {
    auto hc1_switchoff_control = addGroupHelper(km271CfgTopics.HC1_SWITCH_OFF_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc1_switch_off_threshold = ESPUI.addControl(Slider, "", "-20", Dark, hc1_switchoff_control, generalCallback);
    ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc1_switch_off_threshold);
    ESPUI.addControl(Max, "", "10", None, id.ctrl.hc1_switch_off_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SWITCHOFF[config.lang], None, hc1_switchoff_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, hc1_switchoff_control), LABLE_STYLE_DESCRIPTION);
  }
  if (config.km271.use_hc2) {
    auto hc2_switchoff_control = addGroupHelper(km271CfgTopics.HC2_SWITCH_OFF_THRESHOLD[config.lang], Dark, id.tab.control);
    id.ctrl.hc2_switch_off_threshold = ESPUI.addControl(Slider, "", "-20", Dark, hc2_switchoff_control, generalCallback);
    ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc2_switch_off_threshold);
    ESPUI.addControl(Max, "", "10", None, id.ctrl.hc2_switch_off_threshold);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SWITCHOFF[config.lang], None, hc2_switchoff_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, hc2_switchoff_control), LABLE_STYLE_DESCRIPTION);
  }
  
  if (config.km271.use_ww) {
    auto ww_temp_control = addGroupHelper(km271CfgTopics.WW_TEMP[config.lang], Dark, id.tab.control);
    id.ctrl.ww_setpoint = ESPUI.addControl(Slider, "", "30", Dark, ww_temp_control, generalCallback);
    ESPUI.addControl(Min, "", "30", None, id.ctrl.ww_setpoint);
    ESPUI.addControl(Max, "", "60", None, id.ctrl.ww_setpoint);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_WWTEMP[config.lang], None, ww_temp_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[config.lang], None, ww_temp_control), LABLE_STYLE_DESCRIPTION);

    auto ww_pump_cycles_control = addGroupHelper(km271CfgTopics.WW_CIRCULATION[config.lang], Dark, id.tab.control);
    id.ctrl.ww_pump_cycles = ESPUI.addControl(Slider, "", "0", Dark, ww_pump_cycles_control, generalCallback);
    ESPUI.addControl(Min, "", "0", None, id.ctrl.ww_pump_cycles);
    ESPUI.addControl(Max, "", "7", None, id.ctrl.ww_pump_cycles);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_WW_PUMP_CIRC1[config.lang], None, ww_pump_cycles_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_WW_PUMP_CIRC2[config.lang], None, ww_pump_cycles_control), LABLE_STYLE_DESCRIPTION);
  }

  if (config.oilmeter.use_hardware_meter) {
    ESPUI.addControl(ControlType::Separator, webText.OILMETER[config.lang], "", ControlColor::None, id.tab.control);
    auto oilmeter_control = addGroupHelper(webText.OILMETER[config.lang], Dark, id.tab.control);
    id.ctrl.oilmeter_input = ESPUI.addControl(Text, "", "0", Dark, oilmeter_control, generalCallback);
    ESPUI.setElementStyle(id.ctrl.oilmeter_input, "width: 50%; font-size: 24px; color: black");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, oilmeter_control), "background-color: unset; width: 10%"); // only spacer
    id.ctrl.oilmeter_button = ESPUI.addControl(Button, "", webText.BUTTON_SET[config.lang], Dark, oilmeter_control, generalCallback);
    ESPUI.setElementStyle(id.ctrl.oilmeter_button, "width: 38%");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, oilmeter_control), "background-color: unset; width: 100%"); // only spacer
    id.ctrl.oilmeter_output = addGroupValueHelper(webText.OILMETER_ACT[config.lang], "--", "Liter", oilmeter_control);
  }
} // end addControlElements()

/**
 * *******************************************************************
 * @brief   Elements for Heating Circuit 1 Tab
 * *******************************************************************/
void addHc1Tab(){
    
  id.tab.hc1 = ESPUI.addControl(Tab, "", webText.HC1[config.lang], ControlColor::None, 0, generalCallback);
  
  // table: hc1 config
  auto hc1_config = addGroupHelper(webText.CONFIG[config.lang], Dark, id.tab.hc1);
  id.tables.hc1_config = addEmtyElement(hc1_config);

  // table: hc1 status
  auto hc1_status = addGroupHelper(webText.STATUS[config.lang], Dark, id.tab.hc1);
  id.tables.hc1_status = addEmtyElement(hc1_status);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc1);

  // table: hc1 bw1
  auto hc1_bw1 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.hc1);
  id.tables.hc1_bw1 = addEmtyElement(hc1_bw1);

  // table: hc1 bw2
  auto hc1_bw2 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.hc1);
  id.tables.hc1_bw2 = addEmtyElement(hc1_bw2);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc1);

  // table: hc1 programs
  auto hc1_prog = addGroupHelper(webText.PROGRAMS[config.lang], Dark, id.tab.hc1);
  id.tables.hc1_prog = addEmtyElement(hc1_prog);
  ESPUI.setPanelWide(hc1_prog, true);

}

/**
 * *******************************************************************
 * @brief   Elements for Heating Circuit 2 Tab
 * *******************************************************************/
void addHc2Tab(){
  
  id.tab.hc2 = ESPUI.addControl(Tab, "", webText.HC2[config.lang], ControlColor::None, 0, generalCallback);
  
  // table: hc2 config
  auto hc2_config = addGroupHelper(webText.CONFIG[config.lang], Dark, id.tab.hc2);
  id.tables.hc2_config = addEmtyElement(hc2_config);

  // table: hc2 status
  auto hc2_status = addGroupHelper(webText.STATUS[config.lang], Dark, id.tab.hc2);
  id.tables.hc2_status = addEmtyElement(hc2_status);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc2);

  // table: hc2 bw1
  auto hc2_bw1 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.hc2);
  id.tables.hc2_bw1 = addEmtyElement(hc2_bw1);

  // table: hc2 bw2
  auto hc2_bw2 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.hc2);
  id.tables.hc2_bw2 = addEmtyElement(hc2_bw2);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc2);

  // table: hc2 programs
  auto hc2_prog = addGroupHelper(webText.PROGRAMS[config.lang], Dark, id.tab.hc2);
  id.tables.hc2_prog = addEmtyElement(hc2_prog);
  ESPUI.setPanelWide(hc2_prog, true);
}

/**
 * *******************************************************************
 * @brief   Elements for Warm-Water Tab
 * *******************************************************************/
void addWWTab(){
  id.tab.ww = ESPUI.addControl(Tab, "", webText.WW[config.lang], ControlColor::None, 0, generalCallback);

  // table: ww config
  auto ww_config = addGroupHelper(webText.CONFIG[config.lang], Dark, id.tab.ww);
  id.tables.ww_config = addEmtyElement(ww_config);

  // table: hc2 status
  auto ww_status = addGroupHelper(webText.STATUS[config.lang], Dark, id.tab.ww);
  id.tables.ww_status = addEmtyElement(ww_status);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.ww);

  // table: ww bw1
  auto ww_bw1 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.ww);
  id.tables.ww_bw1 = addEmtyElement(ww_bw1);

  // table: ww bw2
  auto ww_bw2 = addGroupHelper(webText.OP_VALUES[config.lang], Dark, id.tab.ww);
  id.tables.ww_bw2 = addEmtyElement(ww_bw2);

}

/**
 * *******************************************************************
 * @brief   Elements for Burner Tab
 * *******************************************************************/
void addBurnerTab(){
  id.tab.boiler = ESPUI.addControl(Tab, "", webText.BURNER[config.lang], ControlColor::None, 0, generalCallback);

  // table: boiler status
  auto boiler_status = addGroupHelper(webText.STATUS[config.lang], Dark, id.tab.boiler);
  id.tables.boiler_status = addEmtyElement(boiler_status);

  // table: boiler states
  auto boiler_stages = addGroupHelper(webText.OPERATION[config.lang], Dark, id.tab.boiler);
  id.tables.boiler_stages = addEmtyElement(boiler_stages);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.boiler);

  // table: boiler error-flags
  auto boiler_err = addGroupHelper(webText.ERROR_FLAGS[config.lang], Dark, id.tab.boiler);
  id.tables.boiler_error = addEmtyElement(boiler_err);

  // table: boiler error-flags
  auto boiler_lifetime = addGroupHelper(webText.LIFETIMES[config.lang], Dark, id.tab.boiler);
  id.tables.boiler_lifetime = addEmtyElement(boiler_lifetime);
}

/**
 * *******************************************************************
 * @brief   Elements for General Tab
 * *******************************************************************/
void addGeneralTab(){
  id.tab.general = ESPUI.addControl(Tab, "", webText.GENERAL[config.lang], ControlColor::None, 0, generalCallback);

  auto general_config = addGroupHelper(webText.CONFIG[config.lang], Dark, id.tab.general);
  id.tables.general_config = addEmtyElement(general_config);

  auto general_limits = addGroupHelper(webText.LIMITS[config.lang], Dark, id.tab.general);
  id.tables.general_limits = addEmtyElement(general_limits);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.general);

  auto general_temp = addGroupHelper(webText.TEMPERATURES[config.lang], Dark, id.tab.general);
  id.tables.general_temp = addEmtyElement(general_temp);
}

/**
 * *******************************************************************
 * @brief   Elements for Alarm Tab
 * *******************************************************************/
void addAlarmTab(){
  id.tab.alarm = ESPUI.addControl(Tab, "", webText.ALARM[config.lang], ControlColor::None, 0, generalCallback);
  auto alarmGroup = addGroupHelper(webText.ALARMINFO[config.lang], Dark, id.tab.alarm);
  id.tables.alarm = addEmtyElement(alarmGroup);
  ESPUI.setPanelWide(alarmGroup, true);
}

/**
 * *******************************************************************
 * @brief   Elements for System Tab
 * *******************************************************************/
void addSystemTab(){
  id.tab.system = ESPUI.addControl(Tab, "", webText.SYSTEM[config.lang], ControlColor::None, 0, generalCallback);
  
  auto wiFiGroup = addGroupHelper(webText.WIFI_INFO[config.lang], Dark, id.tab.system);
  id.tables.system_wifi = addEmtyElement(wiFiGroup);

  auto versionGroup = addGroupHelper(webText.VERSION_INFO[config.lang], Dark, id.tab.system);
  id.tables.system_version = addEmtyElement(versionGroup);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.system);

  auto EspGroup = addGroupHelper(webText.ESP_INFO[config.lang], Dark, id.tab.system);
  id.tables.system_esp = addEmtyElement(EspGroup);

  auto dateTimeGroup = addGroupHelper(webText.DATETIME[config.lang], Dark, id.tab.system);
  
  // NTP Date&Time
  if (config.ntp.enable) {
    id.sys.date  = ESPUI.addControl(Label, "", "", None, dateTimeGroup); // control: output date
    ESPUI.setElementStyle(id.sys.date, "width: 25%; font-size: 20px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, dateTimeGroup), "background-color: unset; width: 5%"); // spacer
    id.sys.time  = ESPUI.addControl(Label, "", "", None, dateTimeGroup); // control: output time
    ESPUI.setElementStyle(id.sys.time, "width: 25%; font-size: 20px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, dateTimeGroup), "background-color: unset; width: 5%"); // spacer
    id.sys.ntp_button = ESPUI.addControl(Button, "", webText.BUTTON_NTP[config.lang], Dark, dateTimeGroup, generalCallback); // control: button
    ESPUI.setElementStyle(id.sys.ntp_button, "width: 30%");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, dateTimeGroup), "background-color: unset; width: 100%"); // spacer
  }
  // manual Date&Time
  id.sys.date_input = ESPUI.addControl(Text, "", "", Dark, dateTimeGroup, generalCallback); // control: input date
  ESPUI.setInputType(id.sys.date_input, "date"); // input control type: date
  ESPUI.setElementStyle(id.sys.date_input, "width: 25%; color: black; font-size: 20px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, dateTimeGroup), "background-color: unset; width: 5%"); // spacer
  id.sys.time_input = ESPUI.addControl(Text, "", "", Dark, dateTimeGroup, generalCallback); // control: input time
  ESPUI.setInputType(id.sys.time_input, "time"); // input control type: time
  ESPUI.setElementStyle(id.sys.time_input, "width: 25%; color: black; font-size: 20px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, dateTimeGroup), "background-color: unset; width: 5%"); // spacer
  id.sys.dti_button = ESPUI.addControl(Button, "", webText.BUTTON_DTI[config.lang], Dark, dateTimeGroup, generalCallback); // control: button
  ESPUI.setElementStyle(id.sys.dti_button, "width: 30%");
}


/**
 * *******************************************************************
 * @brief   Elements for Dashboard
 * *******************************************************************/
void addSettingsTab(){
  id.tab.settings = ESPUI.addControl(Tab, "", webText.SETTINGS[config.lang], ControlColor::None, 0, generalCallback);

  // OTA Info
  auto setOTAGroup = addGroupHelper(webText.OTA_1[config.lang], Dark, id.tab.settings);
  id.settings.wifi_otaIP = ESPUI.addControl(Label, "", "for OTA update go to: <IP-Address>:8080/update", None, setOTAGroup);
  ESPUI.setElementStyle(id.settings.wifi_otaIP, LABLE_STYLE_DESCRIPTION);
  ESPUI.setPanelWide(setOTAGroup, true);

  // WiFi-Settings
  auto setWiFiGroup = addGroupHelper(webText.WIFI[config.lang], Dark, id.tab.settings);
  id.settings.wifi_hostname = addTextInputHelper(webText.HOSTNAME[config.lang], setWiFiGroup);
  id.settings.wifi_ssid = addTextInputHelper(webText.SSID[config.lang], setWiFiGroup);
  id.settings.wifi_passw = addTextInputHelper(webText.PASSWORD[config.lang], setWiFiGroup);
  ESPUI.setInputType(id.settings.wifi_passw, "password"); // input control type: password
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.WIFI_AP_INFO_1[config.lang], None, setWiFiGroup), LABLE_STYLE_DESCRIPTION);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.WIFI_AP_INFO_2[config.lang], None, setWiFiGroup), LABLE_STYLE_DESCRIPTION);

  // MQTT-Settings
  auto setMqttGroup = addGroupHelper(webText.MQTT[config.lang], Dark, id.tab.settings);
  id.settings.mqtt_enable = addSwitcherInputHelper(webText.ACTIVATE[config.lang], setMqttGroup);
  id.settings.mqtt_config_retain = addSwitcherInputHelper(webText.MQTT_CFG_RET[config.lang], setMqttGroup);
  id.settings.mqtt_server = addTextInputHelper(webText.SERVER[config.lang], setMqttGroup);
  id.settings.mqtt_port = addTextInputHelper(webText.PORT[config.lang], setMqttGroup);
  id.settings.mqtt_topic = addTextInputHelper(webText.TOPIC[config.lang], setMqttGroup);
  id.settings.mqtt_user = addTextInputHelper(webText.USER[config.lang], setMqttGroup);
  id.settings.mqtt_passw = addTextInputHelper(webText.PASSWORD[config.lang], setMqttGroup);
  ESPUI.setInputType(id.settings.mqtt_passw, "password"); // input control type: password


  // NTP-Settings
  auto setNtpGroup = addGroupHelper(webText.NTP[config.lang], Dark, id.tab.settings);
  id.settings.ntp_enable = addSwitcherInputHelper(webText.NTP[config.lang], setNtpGroup);
  id.settings.ntp_server = addTextInputHelper(webText.SERVER[config.lang], setNtpGroup);
  id.settings.ntp_tz = addTextInputHelper(webText.NTP_TZ[config.lang], setNtpGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, setNtpGroup), "background-color: unset; width: 100%"); // spacer

  // Logamatic-Settings
  auto setKM271Group = addGroupHelper(webText.LOGAMATIC[config.lang], Dark, id.tab.settings);
  id.settings.km271_useHc1 = addSwitcherInputHelper(webText.HC1[config.lang], setKM271Group);
  id.settings.km271_useHc2 = addSwitcherInputHelper(webText.HC2[config.lang], setKM271Group);
  id.settings.km271_useWW = addSwitcherInputHelper(webText.WW[config.lang], setKM271Group);
  id.settings.km271_useAlarm = addSwitcherInputHelper(webText.ALARM[config.lang], setKM271Group);

  // Oilcounter-Settings
  auto setOilGroup = addGroupHelper(webText.OILMETER[config.lang], Dark, id.tab.settings);
  id.settings.oil_useHardware = addSwitcherInputHelper(webText.OIL_HARDWARE[config.lang], setOilGroup);
  id.settings.oil_useVirtual = addSwitcherInputHelper(webText.OIL_VIRTUAL[config.lang], setOilGroup);
  id.settings.oil_consumption_kg_h = addTextInputHelper(webText.OIL_PAR1_KG_H[config.lang], setOilGroup);
  id.settings.oil_oil_density_kg_l= addTextInputHelper(webText.OIL_PAR2_KG_L[config.lang], setOilGroup);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", "\n\n", None, setOilGroup), "background-color: unset; width: 100%"); // spacer

  // GPIO-Settings
  auto setGpioGroup = addGroupHelper(webText.GPIO[config.lang], Dark, id.tab.settings);

  id.settings.gpio_board = ESPUI.addControl(Select, webText.PREDEFINE[config.lang], "", Dark, setGpioGroup, generalCallback);
  ESPUI.addControl(Option, BOARDS[0], "0", None, id.settings.gpio_board);
  ESPUI.addControl(Option, BOARDS[1], "1", None, id.settings.gpio_board);
  ESPUI.addControl(Option, BOARDS[2], "2", None, id.settings.gpio_board);
  ESPUI.addControl(Option, BOARDS[3], "3", None, id.settings.gpio_board);

  id.settings.gpio_km271_rx = addNumberInputHelper(webText.KM271_RX[config.lang], setGpioGroup);
  id.settings.gpio_km271_tx = addNumberInputHelper(webText.KM271_TX[config.lang], setGpioGroup);
  id.settings.gpio_led_heatbeat = addNumberInputHelper(webText.LED_HEARTBEAT[config.lang], setGpioGroup);
  id.settings.gpio_led_logmode = addNumberInputHelper(webText.LED_LOGMODE[config.lang], setGpioGroup);
  id.settings.gpio_led_wifi = addNumberInputHelper(webText.LED_WIFI[config.lang], setGpioGroup);
  id.settings.gpio_led_oilcounter= addNumberInputHelper(webText.LED_OILCOUNTER[config.lang], setGpioGroup);
  id.settings.gpio_trigger_oilcounter = addNumberInputHelper(webText.TRIG_OILCOUNTER[config.lang], setGpioGroup);

  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.GPIO_UNUSED[config.lang], None, setGpioGroup), LABLE_STYLE_DESCRIPTION);

  // Language-Settings
  id.settings.language = ESPUI.addControl(Select, km271CfgTopics.LANGUAGE[config.lang], "", Dark, id.tab.settings, generalCallback);
  ESPUI.addControl(Option, LANGUAGES[0], "0", None, id.settings.language);
  ESPUI.addControl(Option, LANGUAGES[1], "1", None, id.settings.language);
  
  // Buttons
  auto btnGroup = addGroupHelper(webText.SETTINGS[config.lang], Dark, id.tab.settings);
  id.settings.btnSave = ESPUI.addControl(Button, "", webText.SAVE_RESTART[config.lang], Dark, btnGroup, generalCallback);
 } 

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - creates all webUI elements
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUISetup(){
 
  /*-------------------------------------------------------------------------
  // initialize structs
  -------------------------------------------------------------------------*/
  memset(&kmStatusCpy, 0, sizeof(s_km271_status));
  memset(&kmConfigNumCpy, 0, sizeof(s_km271_config_num));
  memset(&kmConfigStrCpy, 0, sizeof(s_km271_config_num));

  // add additional css styles to a hidden label
  ESPUI.setPanelStyle(ESPUI.label("Refresh your browser if you read this", ControlColor::None, CUSTOM_CSS), "display: none");
  
  // Log-Mode
  ESPUI.setVerbosity(Verbosity::Verbose);

  if (setupMode){
    addSettingsTab();
    ESPUI.begin("Buderus Logamatic - ⚠️ SETUP MODE ⚠️");
  }
  else{
    addDashboardTab();
    addControlTab();
    if (config.km271.use_hc1)
      addHc1Tab();
    if (config.km271.use_hc2)
      addHc2Tab();
    if (config.km271.use_ww)
      addWWTab();
  
    addBurnerTab();
    addGeneralTab();
  
    if (config.km271.use_alarmMsg)
      addAlarmTab();

    addSystemTab();
    addSettingsTab();
    
    ESPUI.begin("Buderus Logamatic");
  }
  
  Serial.println("Webserver started");

  Serial.print("Widgets: ");
  Serial.println(id.settings.btnSave);

  // Update Settings
  updateSettingsValues();

} // END SETUP

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUICylic(){

  if (valeCompareTimer.cycleTrigger(3000) && !setupMode){

    // chek if config values changed
    if(memcmp(&kmStatusCpy, km271GetStatusValueAdr() ,sizeof(s_km271_status))) {
      km271GetStatusValueCopy(&kmStatusCpy);
      updateStatusValues();
    }

    // chek if config values changed
    if(memcmp(&kmConfigStrCpy, km271GetConfigStringsAdr(), sizeof(s_km271_config_str))) {
      km271GetConfigStringsCopy(&kmConfigStrCpy);
      km271GetConfigNumValueCopy(&kmConfigNumCpy);
      updateConfigValues();
    }

    // chek if alarm messages changed
    if(memcmp(&kmAlarmMsgCpy, km271GetAlarmMsgAdr(), sizeof(s_km271_alarm_str))) {
      km271GetAlarmMsgCopy(&kmAlarmMsgCpy);
      updateAlarmTab();
    }

    // update System Informations
    updateSystemInfo();

    // check if oilcounter value changed
    if (config.oilmeter.use_hardware_meter) {
      oilcounter = getOilmeter();
      if (oilcounter!=oilcounterOld) {
        oilcounterOld = oilcounter;
        updateOilmeter();
      } 
    }

  } // end if valeCompareTimer.cycleTrigger(3000) &&!setupMode

  // IP-Address for OTA Update Page
  if (setupMode && tmpIpAddress != WiFi.softAPIP()){
    tmpIpAddress = WiFi.softAPIP();
    snprintf(tmpMessage, sizeof(tmpMessage), "🔄 %s<a href=\"http://%s:8080/update\">http://%s:8080/update</a>", webText.OTA_2[config.lang], WiFi.softAPIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
    ESPUI.updateLabel(id.settings.wifi_otaIP, tmpMessage);
  }
  else if (!setupMode && tmpIpAddress != WiFi.localIP()){
    tmpIpAddress = WiFi.localIP();
    snprintf(tmpMessage, sizeof(tmpMessage), "🔄 %s<a href=\"http://%s:8080/update\">http://%s:8080/update</a>", webText.OTA_2[config.lang], WiFi.localIP().toString().c_str(), WiFi.localIP().toString().c_str());
    ESPUI.updateLabel(id.settings.wifi_otaIP, tmpMessage);
  }

}


/**
 * *******************************************************************
 * @brief   update Status values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateStatusValues(){
  // HC1-Values
  if (config.km271.use_hc1) {  
    
    // HC-Operating States 1
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)) {              // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[config.lang]);
    }
    else {                                                            // MANUAL
      if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1)) {            // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      }
      else {                                                          // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    ESPUI.updateLabel(id.dash.hc1_opmode, tmpMessage);

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔆", webText.SUMMER[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ❄️", webText.WINTER[config.lang]);
    
    ESPUI.updateLabel(id.dash.hc1summerWinter, tmpMessage);

    // Day / Night
    if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ☀️", webText.DAY[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🌙", webText.NIGHT[config.lang]);

    ESPUI.updateLabel(id.dash.hc1dayNight, tmpMessage);

    // Pump
    if (kmStatusCpy.HC1_PumpPower==0)
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🟡", webText.ON[config.lang]);
  
    ESPUI.updateLabel(id.dash.hc1pumpState, tmpMessage);
    ESPUI.updateLabel(id.dash.hc1flowSetTemp, uint8ToString(kmStatusCpy.HC1_HeatingForwardTargetTemp));
    ESPUI.updateLabel(id.dash.hc1flowActTemp, uint8ToString(kmStatusCpy.HC1_HeatingForwardActualTemp));   

    // table: hc1 status
    initElements();
    addElementUnit(km271StatTopics.HC1_FLOW_SETPOINT[config.lang],   uint8ToString(kmStatusCpy.HC1_HeatingForwardTargetTemp), "°C");
    addElementUnit(km271StatTopics.HC1_FLOW_TEMP[config.lang],       uint8ToString(kmStatusCpy.HC1_HeatingForwardActualTemp), "°C"); 
    addElementUnit(km271StatTopics.HC1_HEAT_CURVE1[config.lang],     uint8ToString(kmStatusCpy.HC1_HeatingCurvePlus10), "°C"); 
    addElementUnit(km271StatTopics.HC1_HEAT_CURVE2[config.lang],     uint8ToString(kmStatusCpy.HC1_HeatingCurve0), "°C"); 
    addElementUnit(km271StatTopics.HC1_HEAT_CURVE3[config.lang],     uint8ToString(kmStatusCpy.HC1_HeatingCurveMinus10), "°C"); 
    addElementUnit(km271StatTopics.HC1_MIXER[config.lang],           uint8ToString(kmStatusCpy.HC1_MixingValue), "%"); 
    addElementUnit(km271StatTopics.HC1_OFF_TIME_OPT[config.lang],    uint8ToString(kmStatusCpy.HC1_SwitchOffOptimizationTime), "min"); 
    addElementUnit(km271StatTopics.HC1_ON_TIME_OPT[config.lang],     uint8ToString(kmStatusCpy.HC1_SwitchOnOptimizationTime), "min"); 
    addElementUnit(km271StatTopics.HC1_PUMP[config.lang],            uint8ToString(kmStatusCpy.HC1_PumpPower), "%"); 
    addElementUnit(km271StatTopics.HC1_ROOM_SETPOINT[config.lang],   uint8ToString(kmStatusCpy.HC1_RoomTargetTemp), "°C"); 
    addElementUnit(km271StatTopics.HC1_ROOM_TEMP[config.lang],       uint8ToString(kmStatusCpy.HC1_RoomActualTemp), "°C"); 
    updateElements(id.tables.hc1_status);


    // table: hc1 ov1
    initElements();
    addElement(km271StatTopics.HC1_OV1_OFFTIME_OPT[config.lang], onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0))); 
    addElement(km271StatTopics.HC1_OV1_ONTIME_OPT[config.lang],  onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1))); 
    addElement(km271StatTopics.HC1_OV1_AUTOMATIC[config.lang],   onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)));  
    addElement(km271StatTopics.HC1_OV1_WW_PRIO[config.lang],     onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)));  
    addElement(km271StatTopics.HC1_OV1_SCREED_DRY[config.lang],  onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)));  
    addElement(km271StatTopics.HC1_OV1_HOLIDAY[config.lang],     onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)));     
    addElement(km271StatTopics.HC1_OV1_FROST[config.lang],       onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)));  
    updateElements(id.tables.hc1_bw1);

    // table: hc1 ov2
    initElements();
    addElement(km271StatTopics.HC1_OV2_SUMMER[config.lang],         onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0))); 
    addElement(km271StatTopics.HC1_OV2_DAY[config.lang],            onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1))); 
    addElement(km271StatTopics.HC1_OV2_NO_COM_REMOTE[config.lang],  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2))); 
    addElement(km271StatTopics.HC1_OV2_REMOTE_ERR[config.lang],     errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)));
    addElement(km271StatTopics.HC1_OV2_FLOW_SENS_ERR[config.lang],  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)));
    addElement(km271StatTopics.HC1_OV2_FLOW_AT_MAX[config.lang],    errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)));
    addElement(km271StatTopics.HC1_OV2_EXT_SENS_ERR[config.lang],   errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6))); 
    updateElements(id.tables.hc1_bw2);

  }

  // HC2-Values
  if (config.km271.use_hc2) {  

    // HC2-Operating State
    if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)) {              // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[config.lang]);
    }
    else {                                                            // MANUAL
      if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1)) {            // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      }
      else {                                                          // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    ESPUI.updateLabel(id.dash.hc2_opmode, tmpMessage);

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔆", webText.SUMMER[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ❄️", webText.WINTER[config.lang]);
    
    ESPUI.updateLabel(id.dash.hc2summerWinter, tmpMessage);

    // Day / Night
    if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ☀️", webText.DAY[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🌙", webText.NIGHT[config.lang]);

    ESPUI.updateLabel(id.dash.hc2dayNight, tmpMessage);

    // Pump
    if (kmStatusCpy.HC2_PumpPower==0)
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[config.lang]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🟡", webText.ON[config.lang]);
    
    ESPUI.updateLabel(id.dash.hc2pumpState, tmpMessage);
    ESPUI.updateLabel(id.dash.hc2flowSetTemp, uint8ToString(kmStatusCpy.HC2_HeatingForwardTargetTemp));
    ESPUI.updateLabel(id.dash.hc2flowActTemp, uint8ToString(kmStatusCpy.HC2_HeatingForwardActualTemp));

    // table hc2 status values
    initElements();
    addElementUnit(km271StatTopics.HC2_FLOW_SETPOINT[config.lang],   uint8ToString(kmStatusCpy.HC2_HeatingForwardTargetTemp), "°C");  
    addElementUnit(km271StatTopics.HC2_FLOW_TEMP[config.lang],       uint8ToString(kmStatusCpy.HC2_HeatingForwardActualTemp), "°C"); 
    addElementUnit(km271StatTopics.HC2_HEAT_CURVE1[config.lang],     uint8ToString(kmStatusCpy.HC2_HeatingCurvePlus10), "°C"); 
    addElementUnit(km271StatTopics.HC2_HEAT_CURVE2[config.lang],     uint8ToString(kmStatusCpy.HC2_HeatingCurve0), "°C"); 
    addElementUnit(km271StatTopics.HC2_HEAT_CURVE3[config.lang],     uint8ToString(kmStatusCpy.HC2_HeatingCurveMinus10), "°C"); 
    addElementUnit(km271StatTopics.HC2_MIXER[config.lang],           uint8ToString(kmStatusCpy.HC2_MixingValue), "%"); 
    addElementUnit(km271StatTopics.HC2_OFF_TIME_OPT[config.lang],    uint8ToString(kmStatusCpy.HC2_SwitchOffOptimizationTime), "min"); 
    addElementUnit(km271StatTopics.HC2_ON_TIME_OPT[config.lang],     uint8ToString(kmStatusCpy.HC2_SwitchOnOptimizationTime), "min"); 
    addElementUnit(km271StatTopics.HC2_PUMP[config.lang],            uint8ToString(kmStatusCpy.HC2_PumpPower), "%"); 
    addElementUnit(km271StatTopics.HC2_ROOM_SETPOINT[config.lang],   uint8ToString(kmStatusCpy.HC2_RoomTargetTemp), "°C"); 
    addElementUnit(km271StatTopics.HC2_ROOM_TEMP[config.lang],       uint8ToString(kmStatusCpy.HC2_RoomActualTemp), "°C"); 
    updateElements(id.tables.hc2_status);

    // table: hc2 ov1
    initElements();
    addElement(km271StatTopics.HC2_OV1_OFFTIME_OPT[config.lang], onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0))); 
    addElement(km271StatTopics.HC2_OV1_ONTIME_OPT[config.lang],  onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1))); 
    addElement(km271StatTopics.HC2_OV1_AUTOMATIC[config.lang],   onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)));  
    addElement(km271StatTopics.HC2_OV1_WW_PRIO[config.lang],     onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)));  
    addElement(km271StatTopics.HC2_OV1_SCREED_DRY[config.lang],  onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)));  
    addElement(km271StatTopics.HC2_OV1_HOLIDAY[config.lang],     onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)));     
    addElement(km271StatTopics.HC2_OV1_FROST[config.lang],       onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)));  
    updateElements(id.tables.hc2_bw1);

    // table: hc2 ov2
    initElements();
    addElement(km271StatTopics.HC2_OV2_SUMMER[config.lang],         onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0))); 
    addElement(km271StatTopics.HC2_OV2_DAY[config.lang],            onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1))); 
    addElement(km271StatTopics.HC2_OV2_NO_COM_REMOTE[config.lang],  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2))); 
    addElement(km271StatTopics.HC2_OV2_REMOTE_ERR[config.lang],     errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)));
    addElement(km271StatTopics.HC2_OV2_FLOW_SENS_ERR[config.lang],  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)));
    addElement(km271StatTopics.HC2_OV2_FLOW_AT_MAX[config.lang],    errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)));
    addElement(km271StatTopics.HC2_OV2_EXT_SENS_ERR[config.lang],   errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6))); 
    updateElements(id.tables.hc2_bw2);   

  }
  
  if (config.km271.use_ww) {  

    // table ww status
    initElements();
    addElement(km271StatTopics.WW_TEMP[config.lang],              uint8ToString(kmStatusCpy.HotWaterActualTemp));              
    addElement(km271StatTopics.WW_SETPOINT[config.lang],          uint8ToString(kmStatusCpy.HotWaterTargetTemp));     
    addElement(km271StatTopics.WW_ONTIME_OPT[config.lang],        onOffString(kmStatusCpy.HotWaterOptimizationTime));     
    addElement(km271StatTopics.WW_PUMP_CHARGE[config.lang],       onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0))); 
    addElement(km271StatTopics.WW_PUMP_CIRC[config.lang],         onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1))); 
    addElement(km271StatTopics.WW_PUMP_SOLAR[config.lang],        onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)));       
    updateElements(id.tables.ww_status);

    // table ww bw1
    initElements();
    addElement(km271StatTopics.WW_OV1_AUTO[config.lang],           onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)));   
    addElement(km271StatTopics.WW_OV1_DESINFECT[config.lang],      onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)));  
    addElement(km271StatTopics.WW_OV1_RELOAD[config.lang],         onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)));  
    addElement(km271StatTopics.WW_OV1_HOLIDAY[config.lang],        onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)));  
    addElement(km271StatTopics.WW_OV1_ERR_DESINFECT[config.lang],  errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4)));  
    addElement(km271StatTopics.WW_OV1_ERR_SENSOR[config.lang],     errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)));  
    addElement(km271StatTopics.WW_OV1_WW_STAY_COLD[config.lang],   errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)));  
    addElement(km271StatTopics.WW_OV1_ERR_ANODE[config.lang],      errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7))); 
    updateElements(id.tables.ww_bw1);

    // table ww bw2
    initElements();
    addElement(km271StatTopics.WW_OV2_LOAD[config.lang],           onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)));   
    addElement(km271StatTopics.WW_OV2_MANUAL[config.lang],         onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)));  
    addElement(km271StatTopics.WW_OV2_RELOAD[config.lang],         onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)));  
    addElement(km271StatTopics.WW_OV2_OFF_TIME_OPT[config.lang],   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)));  
    addElement(km271StatTopics.WW_OV2_ON_TIME_OPT[config.lang],    onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)));  
    addElement(km271StatTopics.WW_OV2_DAY[config.lang],            onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)));  
    addElement(km271StatTopics.WW_OV2_HOT[config.lang],            onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)));  
    addElement(km271StatTopics.WW_OV2_PRIO[config.lang],           onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)));  
    updateElements(id.tables.ww_bw2);

  }

  // Update Boiler status
  initElements();
  addElementUnit(km271StatTopics.BOILER_CONTROL[config.lang], cfgArrayTexts.BURNER_STATE[config.lang][kmStatusCpy.BurnerStates],""); 
  addElementUnit(km271StatTopics.BOILER_TEMP[config.lang],     uint8ToString(kmStatusCpy.BoilerForwardActualTemp), "°C"); 
  addElementUnit(km271StatTopics.BOILER_SETPOINT[config.lang], uint8ToString(kmStatusCpy.BoilerForwardTargetTemp), "°C"); 
  addElementUnit(km271StatTopics.BOILER_OFF_TEMP[config.lang], uint8ToString(kmStatusCpy.BurnerSwitchOffTemp), "°C"); 
  addElementUnit(km271StatTopics.BOILER_ON_TEMP[config.lang],  uint8ToString(kmStatusCpy.BurnerSwitchOnTemp), "°C");  
  addElementUnit(km271CfgTopics.MAX_BOILER_TEMP[config.lang],  kmConfigStrCpy.max_boiler_temperature, "");  
  updateElements(id.tables.boiler_status);

  // Update Boiler stages
  initElements();
  addElement(km271StatTopics.BOILER_STATE_GASTEST[config.lang],   onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)));  
  addElement(km271StatTopics.BOILER_STATE_STAGE1[config.lang],    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1))); 
  addElement(km271StatTopics.BOILER_STATE_PROTECT[config.lang],   onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)));  
  addElement(km271StatTopics.BOILER_STATE_ACTIVE[config.lang],    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)));  
  addElement(km271StatTopics.BOILER_STATE_PER_FREE[config.lang],  onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4))); 
  addElement(km271StatTopics.BOILER_STATE_PER_HIGH[config.lang],  onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5))); 
  addElement(km271StatTopics.BOILER_STATE_STAGE2[config.lang],    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)));  
  updateElements(id.tables.boiler_stages);

  // Update Boiler Errors
  initElements();
  addElement(km271StatTopics.BOILER_ERR_BURNER[config.lang],      errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)));   
  addElement(km271StatTopics.BOILER_ERR_SENSOR[config.lang],      errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1))); 
  addElement(km271StatTopics.BOILER_ERR_AUX_SENS[config.lang],    errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2))); 
  addElement(km271StatTopics.BOILER_ERR_STAY_COLD[config.lang],   errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)));
  addElement(km271StatTopics.BOILER_ERR_GAS_SENS[config.lang],    errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4))); 
  addElement(km271StatTopics.BOILER_ERR_EXHAUST[config.lang],     errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5))); 
  addElement(km271StatTopics.BOILER_ERR_SAFETY[config.lang],      errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6))); 
  addElement(km271StatTopics.BOILER_ERR_EXT[config.lang],         errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)));  
  updateElements(id.tables.boiler_error);

  // Update Boiler Lifetimes
  initElements();
  addElement(km271StatTopics.BOILER_LIFETIME_1[config.lang],       uint8ToString(kmStatusCpy.BurnerOperatingDuration_0));     
  addElement(km271StatTopics.BOILER_LIFETIME_2[config.lang],       uint8ToString(kmStatusCpy.BurnerOperatingDuration_1));      
  addElement(km271StatTopics.BOILER_LIFETIME_3[config.lang],       uint8ToString(kmStatusCpy.BurnerOperatingDuration_2));      
  addElement(km271StatTopics.BOILER_LIFETIME_4[config.lang],       uint64ToString(kmStatusCpy.BurnerOperatingDuration_Sum)); 
  updateElements(id.tables.boiler_lifetime);

  if(config.oilmeter.use_virtual_meter && !config.oilmeter.use_hardware_meter) {
    // Oilmeter value in controlTab
    snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f", kmStatusCpy.BurnerCalcOilConsumption);
    ESPUI.updateLabel(id.ctrl.oilmeter_output, tmpMessage);
    // Oilmeter value in dashboardTab
    snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L  🩸", kmStatusCpy.BurnerCalcOilConsumption);
    ESPUI.updateLabel(id.dash.oilmeter, tmpMessage); 
  }

  // allgemiene Werte
  initElements();
  addElementUnit(km271StatTopics.OUTSIDE_TEMP[config.lang],         int8ToString(kmStatusCpy.OutsideTemp), "°C");       
  addElementUnit(km271StatTopics.OUTSIDE_TEMP_DAMPED[config.lang],  int8ToString(kmStatusCpy.OutsideDampedTemp), "°C");  
  addElementUnit(km271StatTopics.EXHAUST_TEMP[config.lang],         uint8ToString(kmStatusCpy.ExhaustTemp), "°C");        
  updateElements(id.tables.general_temp);

  // Outdoor Temp
  ESPUI.updateLabel(id.dash.tmp_out_act,  uint8ToString(kmStatusCpy.OutsideTemp));
  ESPUI.updateLabel(id.dash.tmp_out_act_d,  uint8ToString(kmStatusCpy.OutsideDampedTemp));
  
  // Boiler Temperatures
  ESPUI.updateLabel(id.dash.burnerSetTemp,  uint8ToString(kmStatusCpy.BoilerForwardTargetTemp));
  ESPUI.updateLabel(id.dash.burnerActTemp,  uint8ToString(kmStatusCpy.BoilerForwardActualTemp));

  // Burner Status
  if (kmStatusCpy.BurnerStates==0)
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[config.lang]);
  else
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔥", webText.ON[config.lang]);

  ESPUI.updateLabel(id.dash.burnerState, tmpMessage);

  // WW-Updates
  if (config.km271.use_ww) {
    
    // WW-Operating State
    if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)) {       // AUTOMATIC
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[config.lang]);
      }
    else {                                                         // MANUAL
      if (bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5))       // DAY
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.DAY[config.lang]);
      else {                                                       // NIGHT
        snprintf(tmpMessage, sizeof(tmpMessage), "✋ %s : %s", webText.MANUAL[config.lang], webText.NIGHT[config.lang]);
      }
    }
    ESPUI.updateLabel(id.dash.ww_opmode, tmpMessage);

    // WW-Temperatures
    ESPUI.updateLabel(id.dash.wwSetTemp,      uint8ToString(kmStatusCpy.HotWaterTargetTemp));
    ESPUI.updateLabel(id.dash.wwActTemp,      uint8ToString(kmStatusCpy.HotWaterActualTemp));

  }

}

/**
 * *******************************************************************
 * @brief   update Config values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateConfigValues(){
  
  if (config.km271.use_hc1) {
   
    // table: hc1 config
    initElements();
    addElement(km271CfgTopics.HC1_NIGHT_TEMP[config.lang],           kmConfigStrCpy.hc1_night_temp);	           
    addElement(km271CfgTopics.HC1_DAY_TEMP[config.lang],             kmConfigStrCpy.hc1_day_temp);             
    addElement(km271CfgTopics.HC1_OPMODE[config.lang],               kmConfigStrCpy.hc1_operation_mode);	  
    addElement(km271CfgTopics.HC1_HOLIDAY_TEMP[config.lang],         kmConfigStrCpy.hc1_holiday_temp);	  
    addElement(km271CfgTopics.HC1_MAX_TEMP[config.lang],             kmConfigStrCpy.hc1_max_temp);	  
    addElement(km271CfgTopics.HC1_INTERPR[config.lang],              kmConfigStrCpy.hc1_interpretation);		  
    addElement(km271CfgTopics.HC1_SWITCH_ON_TEMP[config.lang],       kmConfigStrCpy.hc1_switch_on_temperature);  
    addElement(km271CfgTopics.HC1_SWITCH_OFF_THRESHOLD[config.lang], kmConfigStrCpy.hc1_switch_off_threshold);
    addElement(km271CfgTopics.HC1_REDUCTION_MODE[config.lang],       kmConfigStrCpy.hc1_reduction_mode);	  
    addElement(km271CfgTopics.HC1_HEATING_SYSTEM[config.lang],       kmConfigStrCpy.hc1_heating_system);	
    addElement(km271CfgTopics.HC1_TEMP_OFFSET[config.lang],          kmConfigStrCpy.hc1_temp_offset);	
    addElement(km271CfgTopics.HC1_REMOTECTRL[config.lang],           kmConfigStrCpy.hc1_remotecontrol);	
    addElement(km271CfgTopics.HC1_FROST_THRESHOLD[config.lang],      kmConfigStrCpy.hc1_frost_protection_threshold);
    addElement(km271CfgTopics.HC1_SUMMER_THRESHOLD[config.lang],     kmConfigStrCpy.hc1_summer_mode_threshold);
    updateElements(id.tables.hc1_config);

    // table: hc1 programs
    initElements();
    addElementWide(km271CfgTopics.HC1_TIMER01[config.lang], kmConfigStrCpy.hc1_timer01);
    addElementWide(km271CfgTopics.HC1_TIMER02[config.lang], kmConfigStrCpy.hc1_timer02);
    addElementWide(km271CfgTopics.HC1_TIMER03[config.lang], kmConfigStrCpy.hc1_timer03);
    addElementWide(km271CfgTopics.HC1_TIMER04[config.lang], kmConfigStrCpy.hc1_timer04);
    addElementWide(km271CfgTopics.HC1_TIMER05[config.lang], kmConfigStrCpy.hc1_timer05);
    addElementWide(km271CfgTopics.HC1_TIMER06[config.lang], kmConfigStrCpy.hc1_timer06);
    addElementWide(km271CfgTopics.HC1_TIMER07[config.lang], kmConfigStrCpy.hc1_timer07);
    addElementWide(km271CfgTopics.HC1_TIMER08[config.lang], kmConfigStrCpy.hc1_timer08);
    addElementWide(km271CfgTopics.HC1_TIMER09[config.lang], kmConfigStrCpy.hc1_timer09);
    addElementWide(km271CfgTopics.HC1_TIMER10[config.lang], kmConfigStrCpy.hc1_timer10);
    addElementWide(km271CfgTopics.HC1_TIMER11[config.lang], kmConfigStrCpy.hc1_timer11);
    addElementWide(km271CfgTopics.HC1_TIMER12[config.lang], kmConfigStrCpy.hc1_timer12);
    addElementWide(km271CfgTopics.HC1_TIMER13[config.lang], kmConfigStrCpy.hc1_timer13);
    addElementWide(km271CfgTopics.HC1_TIMER14[config.lang], kmConfigStrCpy.hc1_timer14);
    updateElements(id.tables.hc1_prog);

    // update control elements
    ESPUI.updateSelect(id.ctrl.hc1_opmode, int8ToString(kmConfigNumCpy.hc1_operation_mode));
    ESPUI.updateSelect(id.ctrl.hc1_program, int8ToString(kmConfigNumCpy.hc1_program));
    ESPUI.updateSlider(id.ctrl.hc1_interpretation, kmConfigNumCpy.hc1_interpretation);
    ESPUI.updateSlider(id.ctrl.hc1_switch_off_threshold, kmConfigNumCpy.hc1_switch_off_threshold);
    ESPUI.updateNumber(id.ctrl.hc1_holiday_days, kmConfigNumCpy.hc1_holiday_days);
    ESPUI.updateSlider(id.ctrl.hc1_frost_mode_threshold, kmConfigNumCpy.hc1_frost_protection_threshold);
    ESPUI.updateSlider(id.ctrl.hc1_summer_mode_threshold, kmConfigNumCpy.hc1_summer_mode_threshold);

  }

  if (config.km271.use_hc2) {
    
    // table: hc2 config
    initElements();
    addElement(km271CfgTopics.HC2_NIGHT_TEMP[config.lang],           kmConfigStrCpy.hc2_night_temp);	           
    addElement(km271CfgTopics.HC2_DAY_TEMP[config.lang],             kmConfigStrCpy.hc2_day_temp);             
    addElement(km271CfgTopics.HC2_OPMODE[config.lang],               kmConfigStrCpy.hc2_operation_mode);	  
    addElement(km271CfgTopics.HC2_HOLIDAY_TEMP[config.lang],         kmConfigStrCpy.hc2_holiday_temp);	  
    addElement(km271CfgTopics.HC2_MAX_TEMP[config.lang],             kmConfigStrCpy.hc2_max_temp);	  
    addElement(km271CfgTopics.HC2_INTERPR[config.lang],              kmConfigStrCpy.hc2_interpretation);		  
    addElement(km271CfgTopics.HC2_SWITCH_ON_TEMP[config.lang],       kmConfigStrCpy.hc2_switch_on_temperature);  
    addElement(km271CfgTopics.HC2_SWITCH_OFF_THRESHOLD[config.lang], kmConfigStrCpy.hc2_switch_off_threshold);
    addElement(km271CfgTopics.HC2_REDUCTION_MODE[config.lang],       kmConfigStrCpy.hc2_reduction_mode);	  
    addElement(km271CfgTopics.HC2_HEATING_SYSTEM[config.lang],       kmConfigStrCpy.hc2_heating_system);	
    addElement(km271CfgTopics.HC2_TEMP_OFFSET[config.lang],          kmConfigStrCpy.hc2_temp_offset);	
    addElement(km271CfgTopics.HC2_REMOTECTRL[config.lang],           kmConfigStrCpy.hc2_remotecontrol);	
    addElement(km271CfgTopics.HC2_FROST_THRESHOLD[config.lang],      kmConfigStrCpy.hc2_frost_protection_threshold);
    addElement(km271CfgTopics.HC2_SUMMER_THRESHOLD[config.lang],     kmConfigStrCpy.hc2_summer_mode_threshold);
    updateElements(id.tables.hc2_config);

    // table: hc2 programs
    initElements();
    addElementWide(km271CfgTopics.HC2_TIMER01[config.lang], kmConfigStrCpy.hc2_timer01);
    addElementWide(km271CfgTopics.HC2_TIMER02[config.lang], kmConfigStrCpy.hc2_timer02);
    addElementWide(km271CfgTopics.HC2_TIMER03[config.lang], kmConfigStrCpy.hc2_timer03);
    addElementWide(km271CfgTopics.HC2_TIMER04[config.lang], kmConfigStrCpy.hc2_timer04);
    addElementWide(km271CfgTopics.HC2_TIMER05[config.lang], kmConfigStrCpy.hc2_timer05);
    addElementWide(km271CfgTopics.HC2_TIMER06[config.lang], kmConfigStrCpy.hc2_timer06);
    addElementWide(km271CfgTopics.HC2_TIMER07[config.lang], kmConfigStrCpy.hc2_timer07);
    addElementWide(km271CfgTopics.HC2_TIMER08[config.lang], kmConfigStrCpy.hc2_timer08);
    addElementWide(km271CfgTopics.HC2_TIMER09[config.lang], kmConfigStrCpy.hc2_timer09);
    addElementWide(km271CfgTopics.HC2_TIMER10[config.lang], kmConfigStrCpy.hc2_timer10);
    addElementWide(km271CfgTopics.HC2_TIMER11[config.lang], kmConfigStrCpy.hc2_timer11);
    addElementWide(km271CfgTopics.HC2_TIMER12[config.lang], kmConfigStrCpy.hc2_timer12);
    addElementWide(km271CfgTopics.HC2_TIMER13[config.lang], kmConfigStrCpy.hc2_timer13);
    addElementWide(km271CfgTopics.HC2_TIMER14[config.lang], kmConfigStrCpy.hc2_timer14);
    updateElements(id.tables.hc2_prog);

    // control elements
    ESPUI.updateSelect(id.ctrl.hc2_opmode, int8ToString(kmConfigNumCpy.hc2_operation_mode));
    ESPUI.updateSelect(id.ctrl.hc2_program, int8ToString(kmConfigNumCpy.hc2_program));
    ESPUI.updateSlider(id.ctrl.hc2_interpretation, kmConfigNumCpy.hc2_interpretation);
    ESPUI.updateSlider(id.ctrl.hc2_switch_off_threshold, kmConfigNumCpy.hc2_switch_off_threshold);
    ESPUI.updateNumber(id.ctrl.hc2_holiday_days, kmConfigNumCpy.hc2_holiday_days);
    ESPUI.updateSlider(id.ctrl.hc2_frost_mode_threshold, kmConfigNumCpy.hc2_frost_protection_threshold);
    ESPUI.updateSlider(id.ctrl.hc2_summer_mode_threshold, kmConfigNumCpy.hc2_summer_mode_threshold);

  }

  if (config.km271.use_ww) {
    // table: ww config
    initElements();
    addElement(km271CfgTopics.WW_PRIO[config.lang], kmConfigStrCpy.ww_priority);
    addElement(km271CfgTopics.WW_TEMP[config.lang], kmConfigStrCpy.ww_temp);
    addElement(km271CfgTopics.WW_OPMODE[config.lang], kmConfigStrCpy.ww_operation_mode);
    addElement(km271CfgTopics.WW_PROCESSING[config.lang], kmConfigStrCpy.ww_processing);
    addElement(km271CfgTopics.WW_CIRCULATION[config.lang], kmConfigStrCpy.ww_circulation);
    updateElements(id.tables.ww_config);

    // update control elements
    ESPUI.updateSelect(id.ctrl.ww_opmode, int8ToString(kmConfigNumCpy.ww_operation_mode));
    ESPUI.updateSlider(id.ctrl.ww_setpoint, kmConfigNumCpy.ww_temp);
    ESPUI.updateSlider(id.ctrl.ww_pump_cycles, kmConfigNumCpy.ww_circulation);
  }

  // table: general values - config
  initElements();
  addElement(km271CfgTopics.BUILDING_TYP[config.lang], kmConfigStrCpy.building_type);
  addElement(km271CfgTopics.LANGUAGE[config.lang], kmConfigStrCpy.language);
  addElement(km271CfgTopics.SCREEN[config.lang], kmConfigStrCpy.display);
  addElement(km271CfgTopics.TIME_OFFSET[config.lang], kmConfigStrCpy.time_offset);
  addElement(km271CfgTopics.BURNER_TYP[config.lang], kmConfigStrCpy.burner_type);
  updateElements(id.tables.general_config);

  // table: general values - limits
  initElements();
  addElement(km271CfgTopics.PUMP_LOGIC[config.lang], kmConfigStrCpy.pump_logic_temp);  
  addElement(km271CfgTopics.BURNER_MIN_MOD[config.lang], kmConfigStrCpy.burner_min_modulation);
  addElement(km271CfgTopics.BURNER_MOD_TIME[config.lang], kmConfigStrCpy.burner_modulation_runtime);
  addElement(km271CfgTopics.EXHAUST_THRESHOLD[config.lang], kmConfigStrCpy.exhaust_gas_temperature_threshold);
  updateElements(id.tables.general_limits);

}

/**
 * *******************************************************************
 * @brief   update System values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfo(){
  
  // WiFi
  initElements();
  addElementUnit(webText.WIFI_IP[config.lang],   wifi.ipAddress, "");
  addElementUnit("WiFi-Signal",   uint8ToString(wifi.signal), "%");
  addElementUnit("WiFi-Rssi",   int8ToString(wifi.rssi), "dbm");
  updateElements(id.tables.system_wifi);

  // Version informations
  initElements();
  addElement(webText.SW_VERSION[config.lang], VERSION);
  snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
  addElement(webText.LOGAMATIC_VERSION[config.lang], tmpMessage);
  addElement(webText.LOGAMATIC_MODUL[config.lang], uint8ToString(kmStatusCpy.Modul));
  updateElements(id.tables.system_version);  

  // ESP informations
  initElements();
  addElementUnit(webText.ESP_HEAPSIZE[config.lang],floatToString((float)ESP.getHeapSize()/1000.0), "KB");
  addElementUnit(webText.ESP_FREEHEAP[config.lang], floatToString((float)ESP.getFreeHeap()/1000.0), "KB");
  addElementUnit(webText.ESP_MAXALLOCHEAP[config.lang], floatToString((float)ESP.getMaxAllocHeap()/1000.0), "KB");
  addElementUnit(webText.ESP_MINFREEHEAP[config.lang], floatToString((float)ESP.getMinFreeHeap()/1000.0), "KB");
  updateElements(id.tables.system_esp);

  // Date and Time
  if (config.ntp.enable) {
    ESPUI.updateLabel(id.sys.date, getDateString());
    ESPUI.updateLabel(id.sys.time, getTimeString());
  }
}

/**
 * *******************************************************************
 * @brief   update alarm messages
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAlarmTab(){
  if (config.km271.use_alarmMsg) {
    
    char alarmLabel1[50]={'\0'};
    char alarmLabel2[50]={'\0'};
    char alarmLabel3[50]={'\0'};
    char alarmLabel4[50]={'\0'};

    snprintf(alarmLabel1, sizeof(alarmLabel1), "⚠️ %s 1", webText.MESSAGE[config.lang]);
    snprintf(alarmLabel2, sizeof(alarmLabel2), "⚠️ %s 2", webText.MESSAGE[config.lang]);
    snprintf(alarmLabel3, sizeof(alarmLabel3), "⚠️ %s 3", webText.MESSAGE[config.lang]);
    snprintf(alarmLabel4, sizeof(alarmLabel4), "⚠️ %s 4", webText.MESSAGE[config.lang]);

    initElements();
    addElementWide(alarmLabel1, kmAlarmMsgCpy.alarm1);  
    addElementWide(alarmLabel2, kmAlarmMsgCpy.alarm2);
    addElementWide(alarmLabel3, kmAlarmMsgCpy.alarm3);
    addElementWide(alarmLabel4, kmAlarmMsgCpy.alarm4);
    updateElements(id.tables.alarm);
  }
}

/**
 * *******************************************************************
 * @brief   update oilcounter value in webUI
 * @param   none 
 * @return  none
 * *******************************************************************/
void updateOilmeter(){
  // Oilmeter value in controlTab
  snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f", float(oilcounter)/100);
  ESPUI.updateLabel(id.ctrl.oilmeter_output, tmpMessage);
  // Oilmeter value in dashboardTab
  snprintf(tmpMessage, sizeof(tmpMessage), "%0.2f  L  🩸", float(oilcounter)/100);
  ESPUI.updateLabel(id.dash.oilmeter, tmpMessage);
}

/**
 * *******************************************************************
 * @brief   update Settings values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSettingsValues(){

  ESPUI.updateText(id.settings.wifi_ssid, config.wifi.ssid);
  ESPUI.updateText(id.settings.wifi_passw, config.wifi.password);
  ESPUI.updateText(id.settings.wifi_hostname, config.wifi.hostname);

  ESPUI.updateSwitcher(id.settings.mqtt_enable, config.mqtt.enable);
  ESPUI.updateText(id.settings.mqtt_server, config.mqtt.server);
  ESPUI.updateText(id.settings.mqtt_port, uint16ToString(config.mqtt.port));
  ESPUI.updateText(id.settings.mqtt_topic, config.mqtt.topic);
  ESPUI.updateText(id.settings.mqtt_user, config.mqtt.user);
  ESPUI.updateText(id.settings.mqtt_passw, config.mqtt.password);
  ESPUI.updateSwitcher(id.settings.mqtt_config_retain, config.mqtt.config_retain);

  ESPUI.updateSwitcher(id.settings.ntp_enable, config.ntp.enable);
  ESPUI.updateText(id.settings.ntp_server, config.ntp.server);
  ESPUI.updateText(id.settings.ntp_tz, config.ntp.tz);

  ESPUI.updateSwitcher(id.settings.km271_useHc1, config.km271.use_hc1);
  ESPUI.updateSwitcher(id.settings.km271_useHc2, config.km271.use_hc2);
  ESPUI.updateSwitcher(id.settings.km271_useAlarm, config.km271.use_alarmMsg);
  ESPUI.updateSwitcher(id.settings.km271_useWW, config.km271.use_ww);

  ESPUI.updateSwitcher(id.settings.oil_useHardware, config.oilmeter.use_hardware_meter);
  ESPUI.updateSwitcher(id.settings.oil_useVirtual, config.oilmeter.use_virtual_meter);
  
  ESPUI.updateText(id.settings.oil_consumption_kg_h, floatToString4(config.oilmeter.consumption_kg_h));
  ESPUI.updateText(id.settings.oil_oil_density_kg_l, floatToString4(config.oilmeter.oil_density_kg_l));

  ESPUI.updateNumber(id.settings.gpio_km271_rx, config.gpio.km271_RX);
  ESPUI.updateNumber(id.settings.gpio_km271_tx, config.gpio.km271_TX);
  ESPUI.updateNumber(id.settings.gpio_led_heatbeat, config.gpio.led_heartbeat);
  ESPUI.updateNumber(id.settings.gpio_led_logmode, config.gpio.led_logmode);
  ESPUI.updateNumber(id.settings.gpio_led_wifi, config.gpio.led_wifi);
  ESPUI.updateNumber(id.settings.gpio_led_oilcounter, config.gpio.led_oilcounter);
  ESPUI.updateNumber(id.settings.gpio_trigger_oilcounter, config.gpio.trigger_oilcounter);

  ESPUI.updateSelect(id.settings.language, uint64ToString(config.lang));

}

/**
 * *******************************************************************
 * @brief   general Callback function
 * @param   sender id from sender
 * @param   type responce type 
 * @return  none
 * *******************************************************************/
void generalCallback(Control *sender, int type) {

  // HC1-OPMODE
  if(sender->id == id.ctrl.hc1_opmode) {
    km271sendCmd(KM271_SENDCMD_HC1_OPMODE, sender->value.toInt());
    Serial.println(sender->value.toInt());
  }

  // HC2-OPMODE
  if(sender->id == id.ctrl.hc2_opmode) {
    km271sendCmd(KM271_SENDCMD_HC2_OPMODE, sender->value.toInt());
    Serial.println(sender->value.toInt());
  }

  // WW-OPMODE
  if(sender->id == id.ctrl.ww_opmode) {
    km271sendCmd(KM271_SENDCMD_WW_OPMODE, sender->value.toInt());
    Serial.println(sender->value.toInt());
  }

  // HC1-Program
  if(sender->id == id.ctrl.hc1_program) {
    km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, sender->value.toInt());
    Serial.println(sender->value.toInt());
  }

  // HC2-Program
  if(sender->id == id.ctrl.hc2_program) {
    km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, sender->value.toInt());
    Serial.println(sender->value.toInt());
  }
  
  // HC1-Frost Threshold
  if(sender->id == id.ctrl.hc1_frost_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_HC1_FROST, sender->value.toInt());
  }

  // HC1-Summer Threshold
  if(sender->id == id.ctrl.hc1_summer_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_HC1_SUMMER, sender->value.toInt());
  }

  // HC2-Frost Threshold
  if(sender->id == id.ctrl.hc2_frost_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_HC2_FROST, sender->value.toInt());
  }

  // HC2-Summer Threshold
  if(sender->id == id.ctrl.hc2_summer_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_HC2_SUMMER, sender->value.toInt());
  }

  // HC1-DesignTemp
  if(sender->id == id.ctrl.hc1_interpretation) {
    km271sendCmd(KM271_SENDCMD_HC1_DESIGN_TEMP, sender->value.toInt());
  }

  // HC2-DesignTemp
  if(sender->id == id.ctrl.hc2_interpretation) {
    km271sendCmd(KM271_SENDCMD_HC2_DESIGN_TEMP, sender->value.toInt());
  }
  
  // HC1-SwitchOffTemp
  if(sender->id == id.ctrl.hc1_switch_off_threshold) {
    km271sendCmd(KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, sender->value.toInt());
  }

  // HC2-SwitchOffTemp
  if(sender->id == id.ctrl.hc2_switch_off_threshold) {
    km271sendCmd(KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, sender->value.toInt());
  }

  // HC1-Holiday days
  if(sender->id == id.ctrl.hc1_holiday_days) {
    km271sendCmd(KM271_SENDCMD_HC1_HOLIDAYS, sender->value.toInt());
  }

  // HC2-Holiday days
  if(sender->id == id.ctrl.hc2_holiday_days) {
    km271sendCmd(KM271_SENDCMD_HC2_HOLIDAYS, sender->value.toInt());
  }

  // WW-Temp
  if(sender->id == id.ctrl.ww_setpoint) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, sender->value.toInt());
  }

  // WW-Pump Cycles
  if(sender->id == id.ctrl.ww_pump_cycles) {
    km271sendCmd(KM271_SENDCMD_WW_PUMP_CYCLES, sender->value.toInt());
  }

  // Set new Oilcounter value
  if(sender->id == id.ctrl.oilmeter_button && type==B_UP) {
    long setval = ESPUI.getControl(id.ctrl.oilmeter_input)->value.toInt();
    cmdSetOilmeter(setval);
  }

  // set date and time manually
  if(sender->id == id.sys.dti_button && type==B_UP) {
    char tmp1[12] = {'\0'};
    char tmp2[12] = {'\0'};
    tm dti;
    memset(&dti, 0, sizeof(dti));
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

    // get date
    strncpy(tmp1, ESPUI.getControl(id.sys.date_input)->value.c_str(), sizeof(tmp1));
    // extract year
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 4);
    dti.tm_year = atoi(tmp2)-1900;
    // extract month
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1+5, 2);
    dti.tm_mon = atoi(tmp2)-1;
    // extract day
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1+8, 2);
    dti.tm_mday = atoi(tmp2);
    // calculate day of week
    int d    = dti.tm_mday;       //Day     1-31
    int m    = dti.tm_mon+1;      //Month   1-12
    int y    = dti.tm_year+1900;  //Year    2022 
    dti.tm_wday = (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7; // calculate day of week 

    // get time
    strncpy(tmp1, ESPUI.getControl(id.sys.time_input)->value.c_str(), sizeof(tmp1));
    // extract hour
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1, 2);
    dti.tm_hour = atoi(tmp2);
    // extract minutes
    memset(tmp2, 0, sizeof(tmp2));
    strncpy(tmp2, tmp1+3, 2);
    dti.tm_min = atoi(tmp2);

    km271SetDateTimeDTI(dti); // set date and time on Logamatic

  }
  
  // set date and time manually
  if(sender->id == id.sys.ntp_button && type==B_UP) {
    km271SetDateTimeNTP(); // set date and time on Logamatic
  }

  // Settings Save & Restart
  if(sender->id == id.settings.btnSave && type==B_UP) {
    
    // Settings: Language
    config.lang = ESPUI.getControl(id.settings.language)->value.toInt();

    // Settings: WiFi
    snprintf(config.wifi.hostname, sizeof(config.wifi.hostname), ESPUI.getControl(id.settings.wifi_hostname)->value.c_str());
    snprintf(config.wifi.ssid, sizeof(config.wifi.ssid), ESPUI.getControl(id.settings.wifi_ssid)->value.c_str());
    snprintf(config.wifi.password, sizeof(config.wifi.password), ESPUI.getControl(id.settings.wifi_passw)->value.c_str());

    // Settings: MQTT
    config.mqtt.enable = ESPUI.getControl(id.settings.mqtt_enable)->value.toInt();
    snprintf(config.mqtt.server, sizeof(config.mqtt.server), ESPUI.getControl(id.settings.mqtt_server)->value.c_str());
    config.mqtt.port = ESPUI.getControl(id.settings.mqtt_port)->value.toInt();
    snprintf(config.mqtt.topic, sizeof(config.mqtt.topic), ESPUI.getControl(id.settings.mqtt_topic)->value.c_str());
    snprintf(config.mqtt.user, sizeof(config.mqtt.user), ESPUI.getControl(id.settings.mqtt_user)->value.c_str());
    snprintf(config.mqtt.password, sizeof(config.mqtt.password), ESPUI.getControl(id.settings.mqtt_passw)->value.c_str());
    config.mqtt.config_retain = ESPUI.getControl(id.settings.mqtt_config_retain)->value.toInt();

    // Settings: NTP
    config.ntp.enable = ESPUI.getControl(id.settings.ntp_enable)->value.toInt();
    snprintf(config.ntp.server, sizeof(config.ntp.server), ESPUI.getControl(id.settings.ntp_server)->value.c_str());
    snprintf(config.ntp.tz, sizeof(config.ntp.tz), ESPUI.getControl(id.settings.ntp_tz)->value.c_str());

    // Settings: Oilcounter
    config.oilmeter.use_hardware_meter = ESPUI.getControl(id.settings.oil_useHardware)->value.toInt();
    config.oilmeter.use_virtual_meter = ESPUI.getControl(id.settings.oil_useVirtual)->value.toInt();  
    config.oilmeter.consumption_kg_h = ESPUI.getControl(id.settings.oil_consumption_kg_h)->value.toFloat();
    config.oilmeter.oil_density_kg_l = ESPUI.getControl(id.settings.oil_oil_density_kg_l)->value.toFloat();

    // Settings: Logamatic
    config.km271.use_hc1 = ESPUI.getControl(id.settings.km271_useHc1)->value.toInt();
    config.km271.use_hc2 = ESPUI.getControl(id.settings.km271_useHc2)->value.toInt();
    config.km271.use_alarmMsg = ESPUI.getControl(id.settings.km271_useAlarm)->value.toInt();
    config.km271.use_ww = ESPUI.getControl(id.settings.km271_useWW)->value.toInt();

    // Settings: GPIO
    config.gpio.km271_RX = ESPUI.getControl(id.settings.gpio_km271_rx)->value.toInt();
    config.gpio.km271_TX = ESPUI.getControl(id.settings.gpio_km271_tx)->value.toInt();
    config.gpio.led_heartbeat = ESPUI.getControl(id.settings.gpio_led_heatbeat)->value.toInt();
    config.gpio.led_logmode = ESPUI.getControl(id.settings.gpio_led_logmode)->value.toInt();
    config.gpio.led_wifi = ESPUI.getControl(id.settings.gpio_led_wifi)->value.toInt();
    config.gpio.led_oilcounter = ESPUI.getControl(id.settings.gpio_led_oilcounter)->value.toInt();
    config.gpio.trigger_oilcounter = ESPUI.getControl(id.settings.gpio_trigger_oilcounter)->value.toInt();

    configSaveToFile();
    storeData();
    ESP.restart();
  }

  // predefined gpio settings
  if(sender->id == id.settings.gpio_board) {
    switch (sender->value.toInt())
    {
    case 1: // generic ESP32
      config.gpio.km271_RX = 16;
      config.gpio.km271_TX = 17;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = -1;
      config.gpio.led_logmode = -1;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;

    case 2: // KM271-WiFi v0.0.5
      config.gpio.km271_RX = 4;
      config.gpio.km271_TX = 2;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = 22;
      config.gpio.led_logmode = 23;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;
    
    case 3: // KM271-WiFi v0.0.6
      config.gpio.km271_RX = 4;
      config.gpio.km271_TX = 2;
      config.gpio.led_wifi = 21;
      config.gpio.led_heartbeat = 22;
      config.gpio.led_logmode = 17;
      config.gpio.led_oilcounter = -1;
      config.gpio.trigger_oilcounter = -1;
      break;
    }
    // update controls
    ESPUI.updateNumber(id.settings.gpio_km271_rx, config.gpio.km271_RX);
    ESPUI.updateNumber(id.settings.gpio_km271_tx, config.gpio.km271_TX);
    ESPUI.updateNumber(id.settings.gpio_led_heatbeat, config.gpio.led_heartbeat);
    ESPUI.updateNumber(id.settings.gpio_led_logmode, config.gpio.led_logmode);
    ESPUI.updateNumber(id.settings.gpio_led_wifi, config.gpio.led_wifi);
    ESPUI.updateNumber(id.settings.gpio_led_oilcounter, config.gpio.led_oilcounter);
    ESPUI.updateNumber(id.settings.gpio_trigger_oilcounter, config.gpio.trigger_oilcounter);

  }


} // end generalCallback()
