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

muTimer cfgTabRefreshTimer = muTimer();   // timer to refresh id.tab.config
muTimer valeCompareTimer = muTimer();     // timer to refresh id.tab.config

const char * hc_opmode_optval[3] = {webText.NIGHT[LANG], webText.DAY[LANG], webText.AUTOMATIC[LANG]};    // array of operating modes for select control
char tmpMessage[300]={'\0'};

long oilcounter, oilcounterOld;           // actual and old oilcounter value


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
 * @brief   helper function to add config value to a group
 * @param   title   value title
 * @param   value   value
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addGroupCfgHelper(const char * title, String value, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_CLEAR_CFG);
	uint16_t value_id = ESPUI.addControl(Label, "", value, None, group);
  ESPUI.setElementStyle(value_id, LABLE_STYLE_VALUE_CFG);
  return value_id;
}

/**
 * *******************************************************************
 * @brief   helper function to add values with wide field to a group
 * @param   title   value title
 * @param   value   value
 * @param   group   parent group
 * @return  none
 * *******************************************************************/
uint16_t addGroupWideHelper(const char * title, String value, uint16_t group) {
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", title, None, group), LABLE_STYLE_CLEAR_WIDE);
	uint16_t value_id = ESPUI.addControl(Label, "", value, None, group);
  ESPUI.setElementStyle(value_id, LABLE_STYLE_VALUE_WIDE);
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
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ON[LANG]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OFF[LANG]);

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
    snprintf(ret_str, sizeof(ret_str), "%s", webText.ERROR[LANG]);
  else
    snprintf(ret_str, sizeof(ret_str), "%s", webText.OK[LANG]);

  return ret_str;
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

  /*-------------------------------------------------------------------------
  // TAB: Dashboard
  -------------------------------------------------------------------------*/
  id.tab.dashboard = ESPUI.addControl(Tab, "", webText.DASHBORAD[LANG], ControlColor::None, 0, generalCallback);

  // HC1 OperationMode
  #ifdef USE_HC1
    auto hc1_opmode_Group = addGroupHelper(webText.HC1_OPMODE[LANG], Dark, id.tab.dashboard);
    id.dash.hc1_opmode = ESPUI.addControl(Label, "", "--", None, hc1_opmode_Group);
    ESPUI.setElementStyle(id.dash.hc1_opmode, LABLE_STYLE_DASH);
  #else
    // HC2 OperationMode
    #ifdef USE_HC2
    auto hc2_opmode_Group = addGroupHelper(webText.HC2_OPMODE[LANG], Dark, id.tab.dashboard);
    id.dash.hc2_opmode = ESPUI.addControl(Label, "", "--", None, hc2_opmode_Group);
    ESPUI.setElementStyle(id.dash.hc2_opmode, LABLE_STYLE_DASH);
    #endif
  #endif

  // WW OperationMode
  auto ww_opmode_Group = addGroupHelper(webText.WW_OPMODE[LANG], Dark, id.tab.dashboard);
  id.dash.ww_opmode = ESPUI.addControl(Label, "", "--", None, ww_opmode_Group);
  ESPUI.setElementStyle(id.dash.ww_opmode, LABLE_STYLE_DASH);

  // HC Day-Night
  #ifdef USE_HC1
    auto hc1dayNight_Group = addGroupHelper(webText.HC1_DAY_NIGHT[LANG], Dark, id.tab.dashboard);
    id.dash.hc1dayNight = ESPUI.addControl(Label, "", "--", None, hc1dayNight_Group);
    ESPUI.setElementStyle(id.dash.hc1dayNight, LABLE_STYLE_DASH);
  #else
    #ifdef USE_HC2
      auto hc2dayNight_Group = addGroupHelper(webText.HC2_DAY_NIGHT[LANG], Dark, id.tab.dashboard);
      id.dash.hc2dayNight = ESPUI.addControl(Label, "", "--", None, hc2dayNight_Group);
      ESPUI.setElementStyle(id.dash.hc2dayNight, LABLE_STYLE_DASH);
    #endif
  #endif

  // HC Summer-Winter
  #ifdef USE_HC1
    auto hc1summerWinter_Group = addGroupHelper(webText.HC1_SUMMER_WINTER[LANG], Dark, id.tab.dashboard);
    id.dash.hc1summerWinter = ESPUI.addControl(Label, "", "--", None, hc1summerWinter_Group);
    ESPUI.setElementStyle(id.dash.hc1summerWinter, LABLE_STYLE_DASH);
  #else
    #ifdef USE_HC2
      auto hc2summerWinter_Group = addGroupHelper(webText.HC2_SUMMER_WINTER[LANG], Dark, id.tab.dashboard);
      id.dash.hc2summerWinter = ESPUI.addControl(Label, "", "--", None, hc2summerWinter_Group);
      ESPUI.setElementStyle(id.dash.hc2summerWinter, LABLE_STYLE_DASH);
    #endif
  #endif

  // Burner
  auto burner_Group = addGroupHelper(webText.BURNER[LANG], Dark, id.tab.dashboard);
  id.dash.burnerState = ESPUI.addControl(Label, "", "--", None, burner_Group);
  ESPUI.setElementStyle(id.dash.burnerState, LABLE_STYLE_DASH);

  // HC Pump
  #ifdef USE_HC1
    auto hc1_pump_Group = addGroupHelper(webText.HC1_PUMP[LANG], Dark, id.tab.dashboard);
    id.dash.hc1pumpState = ESPUI.addControl(Label, "", "--", None, hc1_pump_Group);
    ESPUI.setElementStyle(id.dash.hc1pumpState, LABLE_STYLE_DASH);
  #else
    #ifdef USE_HC2
      auto hc2_pump_Group = addGroupHelper(webText.HC2_PUMP[LANG], Dark, id.tab.dashboard);
      id.dash.hc2pumpState = ESPUI.addControl(Label, "", "--", None, hc2_pump_Group);
      ESPUI.setElementStyle(id.dash.hc2pumpState, LABLE_STYLE_DASH);
    #endif
  #endif

  // WW-Temp
  auto wwTemp_Group = addGroupHelper(webText.WW[LANG], Dark, id.tab.dashboard);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.SET_TEMP[LANG])+ String(" °C"), None, wwTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.ACT_TEMP[LANG])+ String(" °C"), None, wwTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  id.dash.wwSetTemp = ESPUI.addControl(Label, "", "--", None, wwTemp_Group);
  ESPUI.setElementStyle(id.dash.wwSetTemp, "width: 45%; font-size: 30px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, wwTemp_Group), "background-color: unset; width: 10%;");
  id.dash.wwActTemp = ESPUI.addControl(Label, "", "--", None, wwTemp_Group);
  ESPUI.setElementStyle(id.dash.wwActTemp, "width: 45%; font-size: 30px");
  
  // Burner-Temp
  auto burnerTemp_Group = addGroupHelper(webText.BURNER_TEMP[LANG], Dark, id.tab.dashboard);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.SET_TEMP[LANG])+ String(" °C"), None, burnerTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.ACT_TEMP[LANG])+ String(" °C"), None, burnerTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
  id.dash.burnerSetTemp = ESPUI.addControl(Label, "", "--", None, burnerTemp_Group);
  ESPUI.setElementStyle(id.dash.burnerSetTemp, "width: 45%; font-size: 30px");
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, burnerTemp_Group), "background-color: unset; width: 10%;");
  id.dash.burnerActTemp = ESPUI.addControl(Label, "", "--", None, burnerTemp_Group);
  ESPUI.setElementStyle(id.dash.burnerActTemp, "width: 45%; font-size: 30px");
  
  // HC Flow-Temp
  #ifdef USE_HC1
    auto hc1flowTemp_Group = addGroupHelper(webText.HC1_FLOW[LANG], Dark, id.tab.dashboard);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.SET_TEMP[LANG])+ String(" °C"), None, hc1flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.ACT_TEMP[LANG])+ String(" °C"), None, hc1flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
    id.dash.hc1flowSetTemp = ESPUI.addControl(Label, "", "--", None, hc1flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc1flowSetTemp, "width: 45%; font-size: 30px");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1flowTemp_Group), "background-color: unset; width: 10%;");
    id.dash.hc1flowActTemp = ESPUI.addControl(Label, "", "--", None, hc1flowTemp_Group);
    ESPUI.setElementStyle(id.dash.hc1flowActTemp, "width: 45%; font-size: 30px");
  #else
    #ifdef USE_HC2
      auto hc2flowTemp_Group = addGroupHelper(webText.HC2_FLOW[LANG], Dark, id.tab.dashboard);
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.SET_TEMP[LANG])+ String(" °C"), None, hc2flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", String(webText.ACT_TEMP[LANG])+ String(" °C"), None, hc2flowTemp_Group), "background-color: unset; width: 50%; font-size: 15px; text-align: center");
      id.dash.hc2flowSetTemp = ESPUI.addControl(Label, "", "--", None, hc2flowTemp_Group);
      ESPUI.setElementStyle(id.dash.hc2flowSetTemp, "width: 45%; font-size: 30px");
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2flowTemp_Group), "background-color: unset; width: 10%;");
      id.dash.hc2flowActTemp = ESPUI.addControl(Label, "", "--", None, hc2flowTemp_Group);
      ESPUI.setElementStyle(id.dash.hc2flowActTemp, "width: 45%; font-size: 30px");
    #endif
  #endif

  // Oilmeter
  #ifdef USE_OILMETER
    auto oilmeter_Group = addGroupHelper(webText.OILMETER[LANG], Dark, id.tab.dashboard);
    id.dash.oilmeter = ESPUI.addControl(Label, "", "--", None, oilmeter_Group);
    ESPUI.setElementStyle(id.dash.oilmeter, LABLE_STYLE_DASH);
  #endif

  /*-------------------------------------------------------------------------
  // TAB: Control
  -------------------------------------------------------------------------*/
  id.tab.control = ESPUI.addControl(Tab, "", webText.CONTROL[LANG], ControlColor::None, 0, generalCallback);

  ESPUI.addControl(ControlType::Separator, webText.OPMODES[LANG], "", ControlColor::None, id.tab.control);

  // HC-Operation Mode
  #ifdef USE_HC1
    id.ctrl.hc1_opmode = ESPUI.addControl(Select, km271CfgTopics.HC1_OPMODE[LANG], "", Dark, id.tab.control, generalCallback);
    for(auto const& v : hc_opmode_optval) {
      ESPUI.addControl(Option, v, v, None, id.ctrl.hc1_opmode);
    }
  #else
    #ifdef USE_HC2
    id.ctrl.hc2_opmode = ESPUI.addControl(Select, km271CfgTopics.HC2_OPMODE[LANG], "", Dark, id.tab.control, generalCallback);
    for(auto const& v : hc_opmode_optval) {
      ESPUI.addControl(Option, v, v, None, id.ctrl.hc2_opmode);
    }
    #endif
  #endif

  // WW-Operation Mode
	id.ctrl.ww_opmode = ESPUI.addControl(Select, km271CfgTopics.WW_OPMODE[LANG], "", Dark, id.tab.control, generalCallback);
	for(auto const& v : hc_opmode_optval) {
		ESPUI.addControl(Option, v, v, None, id.ctrl.ww_opmode);
	}

  ESPUI.addControl(ControlType::Separator, webText.PROGRAMS[LANG], "", ControlColor::None, id.tab.control);

  // HC-Program
  #ifdef USE_HC1
    id.ctrl.hc1_program = ESPUI.addControl(Select, km271CfgTopics.HC1_PROGRAM[LANG], "", Dark, id.tab.control, generalCallback);
    for(auto const& v : cfgArrayTexts.HC_PROGRAM) {
      ESPUI.addControl(Option, v, v, None, id.ctrl.hc1_program);
    }
  #else
    #ifdef USE_HC2
      id.ctrl.hc2_program = ESPUI.addControl(Select, km271CfgTopics.HC2_PROGRAM[LANG], "", Dark, id.tab.control, generalCallback);
      for(auto const& v : cfgArrayTexts.HC_PROGRAM) {
        ESPUI.addControl(Option, v, v, None, id.ctrl.hc2_program);
      }
    #endif
  #endif

  ESPUI.addControl(ControlType::Separator, webText.TEMPERATURES[LANG], "", ControlColor::None, id.tab.control);

  // Frost Threshold
  auto frost_control = addGroupHelper(km271CfgTopics.FROST_THRESHOLD[LANG], Dark, id.tab.control);
  id.ctrl.frost_mode_threshold = ESPUI.addControl(Slider, "", "-20", Dark, frost_control, generalCallback);
	ESPUI.addControl(Min, "", "-20", None, id.ctrl.frost_mode_threshold);
	ESPUI.addControl(Max, "", "10", None, id.ctrl.frost_mode_threshold);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_FROST[LANG], None, frost_control), LABLE_STYLE_DESCRIPTION);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, frost_control), LABLE_STYLE_DESCRIPTION);

  // Summer Threshold
  auto summer_control = addGroupHelper(km271CfgTopics.SUMMER_THRESHOLD[LANG], Dark, id.tab.control);
  id.ctrl.summer_mode_threshold = ESPUI.addControl(Slider, "", "9", Dark, summer_control, generalCallback);
	ESPUI.addControl(Min, "", "9", None, id.ctrl.summer_mode_threshold);
	ESPUI.addControl(Max, "", "31", None, id.ctrl.summer_mode_threshold);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SUMMER1[LANG], None, summer_control), LABLE_STYLE_DESCRIPTION);
  ESPUI.setElementStyle(ESPUI.addControl(Label, " ", webText.INFO_SUMMER2[LANG], None, summer_control), LABLE_STYLE_DESCRIPTION);

  // HC-DesignTemp
  #ifdef USE_HC1
    auto hc1_designtemp_control = addGroupHelper(km271CfgTopics.HC1_INTERPR[LANG], Dark, id.tab.control);
    id.ctrl.hc1_interpretation = ESPUI.addControl(Slider, "", "30", Dark, hc1_designtemp_control, generalCallback);
    ESPUI.addControl(Min, "", "30", None, id.ctrl.hc1_interpretation);
    ESPUI.addControl(Max, "", "90", None, id.ctrl.hc1_interpretation);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_DESIGNTEMP[LANG], None, hc1_designtemp_control), LABLE_STYLE_DESCRIPTION);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, hc1_designtemp_control), LABLE_STYLE_DESCRIPTION);
  #else
    #ifdef USE_HC2
      auto hc2_designtemp_control = addGroupHelper(km271CfgTopics.HC2_INTERPR[LANG], Dark, id.tab.control);
      id.ctrl.hc2_interpretation = ESPUI.addControl(Slider, "", "30", Dark, hc2_designtemp_control, generalCallback);
      ESPUI.addControl(Min, "", "30", None, id.ctrl.hc2_interpretation);
      ESPUI.addControl(Max, "", "90", None, id.ctrl.hc2_interpretation);
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_DESIGNTEMP[LANG], None, hc2_designtemp_control), LABLE_STYLE_DESCRIPTION);
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, hc2_designtemp_control), LABLE_STYLE_DESCRIPTION);
    #endif
  #endif

  // HC-Switch Off Theshold
  #ifdef USE_HC1
  auto hc1_switchoff_control = addGroupHelper(km271CfgTopics.HC1_SWITCH_OFF_THRESHOLD[LANG], Dark, id.tab.control);
  id.ctrl.hc1_switch_off_threshold = ESPUI.addControl(Slider, "", "-20", Dark, hc1_switchoff_control, generalCallback);
	ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc1_switch_off_threshold);
	ESPUI.addControl(Max, "", "10", None, id.ctrl.hc1_switch_off_threshold);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SWITCHOFF[LANG], None, hc1_switchoff_control), LABLE_STYLE_DESCRIPTION);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, hc1_switchoff_control), LABLE_STYLE_DESCRIPTION);
  #else
    #ifdef USE_HC2
      auto hc2_switchoff_control = addGroupHelper(km271CfgTopics.HC2_SWITCH_OFF_THRESHOLD[LANG], Dark, id.tab.control);
      id.ctrl.hc2_switch_off_threshold = ESPUI.addControl(Slider, "", "-20", Dark, hc2_switchoff_control, generalCallback);
      ESPUI.addControl(Min, "", "-20", None, id.ctrl.hc2_switch_off_threshold);
      ESPUI.addControl(Max, "", "10", None, id.ctrl.hc2_switch_off_threshold);
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_SWITCHOFF[LANG], None, hc2_switchoff_control), LABLE_STYLE_DESCRIPTION);
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, hc2_switchoff_control), LABLE_STYLE_DESCRIPTION);
    #endif
  #endif

  auto ww_temp_control = addGroupHelper(km271CfgTopics.WW_TEMP[LANG], Dark, id.tab.control);
  id.ctrl.ww_setpoint = ESPUI.addControl(Slider, "", "30", Dark, ww_temp_control, generalCallback);
	ESPUI.addControl(Min, "", "30", None, id.ctrl.ww_setpoint);
	ESPUI.addControl(Max, "", "60", None, id.ctrl.ww_setpoint);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_WWTEMP[LANG], None, ww_temp_control), LABLE_STYLE_DESCRIPTION);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", webText.INFO_UNIT_C[LANG], None, ww_temp_control), LABLE_STYLE_DESCRIPTION);


  #ifdef USE_OILMETER
    ESPUI.addControl(ControlType::Separator, webText.OILMETER[LANG], "", ControlColor::None, id.tab.control);
    auto oilmeter_control = addGroupHelper(webText.OILMETER[LANG], Dark, id.tab.control);
    id.ctrl.oilmeter_input = ESPUI.addControl(Text, "", "0", Dark, oilmeter_control, generalCallback);
    ESPUI.setElementStyle(id.ctrl.oilmeter_input, "width: 50%; font-size: 24px; color: black");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, oilmeter_control), "background-color: unset; width: 10%"); // only spacer
    id.ctrl.oilmeter_button = ESPUI.addControl(Button, "", webText.BUTTON_SET[LANG], Dark, oilmeter_control, generalCallback);
    ESPUI.setElementStyle(id.ctrl.oilmeter_button, "width: 38%");
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, oilmeter_control), "background-color: unset; width: 100%"); // only spacer
    id.ctrl.oilmeter_output = addGroupValueHelper(webText.OILMETER_ACT[LANG], "--", "Liter", oilmeter_control);
  #endif

  /*-------------------------------------------------------------------------
  // TAB: Heating Circuit 1
  -------------------------------------------------------------------------*/
  #ifdef USE_HC1
    
    id.tab.hc1 = ESPUI.addControl(Tab, "", String(webText.HC1[LANG]), ControlColor::None, 0, generalCallback);
    
    // config
    auto hc1_config                       = addGroupHelper(webText.CONFIG[LANG], Dark, id.tab.hc1);
    id.cfg.hc1_night_temp                 = addGroupCfgHelper(km271CfgTopics.HC1_NIGHT_TEMP[LANG], "--", hc1_config);
    id.cfg.hc1_day_temp                   = addGroupCfgHelper(km271CfgTopics.HC1_DAY_TEMP[LANG], "--", hc1_config);
    id.cfg.hc1_operation_mode             = addGroupCfgHelper(km271CfgTopics.HC1_OPMODE[LANG], "--", hc1_config);
    id.cfg.hc1_holiday_temp               = addGroupCfgHelper(km271CfgTopics.HC1_HOLIDAY_TEMP[LANG], "--", hc1_config);
    id.cfg.hc1_max_temp                   = addGroupCfgHelper(km271CfgTopics.HC1_MAX_TEMP[LANG], "--", hc1_config);
    id.cfg.hc1_interpretation             = addGroupCfgHelper(km271CfgTopics.HC1_INTERPR[LANG], "--", hc1_config);
    id.cfg.hc1_switch_on_temperature      = addGroupCfgHelper(km271CfgTopics.HC1_SWITCH_ON_TEMP[LANG], "--", hc1_config);
    id.cfg.hc1_switch_off_threshold       = addGroupCfgHelper(km271CfgTopics.HC1_SWITCH_OFF_THRESHOLD[LANG], "--", hc1_config);
    id.cfg.hc1_reduction_mode             = addGroupCfgHelper(km271CfgTopics.HC1_REDUCTION_MODE[LANG], "--", hc1_config);
    id.cfg.hc1_heating_system             = addGroupCfgHelper(km271CfgTopics.HC1_HEATING_SYSTEM[LANG], "--", hc1_config);
    id.cfg.hc1_temp_offset                = addGroupCfgHelper(km271CfgTopics.HC1_TEMP_OFFSET[LANG], "--", hc1_config);
    id.cfg.hc1_remotecontrol              = addGroupCfgHelper(km271CfgTopics.HC1_REMOTECTRL[LANG], "--", hc1_config);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1_config), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1_config), "background-color: unset; width: 100%"); // only spacer

    // status
    auto hc1_status = addGroupHelper(webText.STATUS[LANG], Dark, id.tab.hc1);
    id.stat.hc1_flow_setpoint      = addGroupValueHelper(km271StatTopics.HC1_FLOW_SETPOINT[LANG], "--", "°C", hc1_status);
    id.stat.hc1_flow_temp          = addGroupValueHelper(km271StatTopics.HC1_FLOW_TEMP[LANG], "--", "°C", hc1_status);
    id.stat.hc1_heatcurve1         = addGroupValueHelper(km271StatTopics.HC1_HEAT_CURVE1[LANG], "--", "°C", hc1_status);
    id.stat.hc1_heatcurve2         = addGroupValueHelper(km271StatTopics.HC1_HEAT_CURVE2[LANG], "--", "°C", hc1_status);
    id.stat.hc1_heatcurve3         = addGroupValueHelper(km271StatTopics.HC1_HEAT_CURVE3[LANG], "--", "°C", hc1_status);
    id.stat.hc1_mixer              = addGroupValueHelper(km271StatTopics.HC1_MIXER[LANG], "--", "%", hc1_status);
    id.stat.hc1_off_time_opt       = addGroupValueHelper(km271StatTopics.HC1_OFF_TIME_OPT[LANG], "--", "min", hc1_status);
    id.stat.hc1_on_time_opt        = addGroupValueHelper(km271StatTopics.HC1_ON_TIME_OPT[LANG], "--", "min", hc1_status);
    id.stat.hc1_pump               = addGroupValueHelper(km271StatTopics.HC1_PUMP[LANG], "--", "%", hc1_status);
    id.stat.hc1_room_setpoint      = addGroupValueHelper(km271StatTopics.HC1_ROOM_SETPOINT[LANG], "--", "°C", hc1_status);
    id.stat.hc1_room_temp          = addGroupValueHelper(km271StatTopics.HC1_ROOM_TEMP[LANG], "--", "°C", hc1_status);
    id.stat.hc1_ov1_automatic      = addGroupValueHelper(km271StatTopics.HC1_OV1_AUTOMATIC[LANG], "--", "", hc1_status);
    id.stat.hc1_frost              = addGroupValueHelper(km271StatTopics.HC1_OV1_FROST[LANG], "--", "", hc1_status);
    id.stat.hc1_holiday            = addGroupValueHelper(km271StatTopics.HC1_OV1_HOLIDAY[LANG], "--", "", hc1_status);
    
    ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc1);

    // operation states
    auto hc1_opstates1 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.hc1);
    id.stat.hc1_ov1_off_time_opt   = addGroupValueHelper(km271StatTopics.HC1_OV1_OFFTIME_OPT[LANG], "--", "", hc1_opstates1);
    id.stat.hc1_ov1_on_time_opt    = addGroupValueHelper(km271StatTopics.HC1_OV1_ONTIME_OPT[LANG], "--", "", hc1_opstates1);
    id.stat.hc1_ov1_screed_dry     = addGroupValueHelper(km271StatTopics.HC1_OV1_SCREED_DRY[LANG], "--", "", hc1_opstates1);
    id.stat.hc1_ov1_ww_prio        = addGroupValueHelper(km271StatTopics.HC1_OV1_WW_PRIO[LANG], "--", "", hc1_opstates1);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1_opstates1), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1_opstates1), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc1_opstates1), "background-color: unset; width: 100%"); // only spacer
    
    // operation states
    auto hc1_opstates2 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.hc1);
    id.stat.hc1_ov2_day            = addGroupValueHelper(km271StatTopics.HC1_OV2_DAY[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_summer         = addGroupValueHelper(km271StatTopics.HC1_OV2_SUMMER[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_ext_sens_err   = addGroupValueHelper(km271StatTopics.HC1_OV2_EXT_SENS_ERR[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_flow_at_max    = addGroupValueHelper(km271StatTopics.HC1_OV2_FLOW_AT_MAX[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_flow_sens_err  = addGroupValueHelper(km271StatTopics.HC1_OV2_FLOW_SENS_ERR[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_no_com_remote  = addGroupValueHelper(km271StatTopics.HC1_OV2_NO_COM_REMOTE[LANG], "--", "", hc1_opstates2);
    id.stat.hc1_ov2_remote_err     = addGroupValueHelper(km271StatTopics.HC1_OV2_REMOTE_ERR[LANG], "--", "", hc1_opstates2);

    ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc1);

    // Program
    auto hc1prg_config  = addGroupHelper(webText.PROGRAMS[LANG], Dark, id.tab.hc1);
    id.cfg.hc1_program  = addGroupWideHelper(km271CfgTopics.HC1_PROGRAM[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer01  = addGroupWideHelper(km271CfgTopics.HC1_TIMER01[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer02  = addGroupWideHelper(km271CfgTopics.HC1_TIMER02[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer03  = addGroupWideHelper(km271CfgTopics.HC1_TIMER03[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer04  = addGroupWideHelper(km271CfgTopics.HC1_TIMER04[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer05  = addGroupWideHelper(km271CfgTopics.HC1_TIMER05[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer06  = addGroupWideHelper(km271CfgTopics.HC1_TIMER06[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer07  = addGroupWideHelper(km271CfgTopics.HC1_TIMER07[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer08  = addGroupWideHelper(km271CfgTopics.HC1_TIMER08[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer09  = addGroupWideHelper(km271CfgTopics.HC1_TIMER09[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer10  = addGroupWideHelper(km271CfgTopics.HC1_TIMER10[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer11  = addGroupWideHelper(km271CfgTopics.HC1_TIMER11[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer12  = addGroupWideHelper(km271CfgTopics.HC1_TIMER12[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer13  = addGroupWideHelper(km271CfgTopics.HC1_TIMER13[LANG], "--", hc1prg_config);
    id.cfg.hc1_timer14  = addGroupWideHelper(km271CfgTopics.HC1_TIMER14[LANG], "--", hc1prg_config);
    ESPUI.setPanelWide(hc1prg_config, true);

  #endif /* USE_HC1 */

  /*-------------------------------------------------------------------------
  // TAB: Heating Circuit 2
  -------------------------------------------------------------------------*/
  #ifndef USE_HC1
  
    #ifdef USE_HC2
    
    id.tab.hc2 = ESPUI.addControl(Tab, "", String(webText.HC2[LANG]), ControlColor::None, 0, generalCallback);

    // config values
    auto hc2_config                       = addGroupHelper(webText.CONFIG[LANG], Dark, id.tab.hc2);
    id.cfg.hc2_night_temp                 = addGroupCfgHelper(km271CfgTopics.HC2_NIGHT_TEMP[LANG], "--", hc2_config);
    id.cfg.hc2_day_temp                   = addGroupCfgHelper(km271CfgTopics.HC2_DAY_TEMP[LANG], "--", hc2_config);
    id.cfg.hc2_operation_mode             = addGroupCfgHelper(km271CfgTopics.HC2_OPMODE[LANG], "--", hc2_config);
    id.cfg.hc2_holiday_temp               = addGroupCfgHelper(km271CfgTopics.HC2_HOLIDAY_TEMP[LANG], "--", hc2_config);
    id.cfg.hc2_max_temp                   = addGroupCfgHelper(km271CfgTopics.HC2_MAX_TEMP[LANG], "--", hc2_config);
    id.cfg.hc2_interpretation             = addGroupCfgHelper(km271CfgTopics.HC2_INTERPR[LANG], "--", hc2_config);
    id.cfg.hc2_switch_on_temperature      = addGroupCfgHelper(km271CfgTopics.HC2_SWITCH_ON_TEMP[LANG], "--", hc2_config);
    id.cfg.hc2_switch_off_threshold       = addGroupCfgHelper(km271CfgTopics.HC2_SWITCH_OFF_THRESHOLD[LANG], "--", hc2_config);
    id.cfg.hc2_reduction_mode             = addGroupCfgHelper(km271CfgTopics.HC2_REDUCTION_MODE[LANG], "--", hc2_config);
    id.cfg.hc2_heating_system             = addGroupCfgHelper(km271CfgTopics.HC2_HEATING_SYSTEM[LANG], "--", hc2_config);
    id.cfg.hc2_temp_offset                = addGroupCfgHelper(km271CfgTopics.HC2_TEMP_OFFSET[LANG], "--", hc2_config);
    id.cfg.hc2_remotecontrol              = addGroupCfgHelper(km271CfgTopics.HC2_REMOTECTRL[LANG], "--", hc2_config);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2_config), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2_config), "background-color: unset; width: 100%"); // only spacer
    
    // status
    auto hc2_status = addGroupHelper(webText.STATUS[LANG], Dark, id.tab.hc2);
    id.stat.hc2_flow_setpoint      = addGroupValueHelper(km271StatTopics.HC2_FLOW_SETPOINT[LANG], "--", "°C", hc2_status);
    id.stat.hc2_flow_temp          = addGroupValueHelper(km271StatTopics.HC2_FLOW_TEMP[LANG], "--", "°C", hc2_status);
    id.stat.hc2_heatcurve1         = addGroupValueHelper(km271StatTopics.HC2_HEAT_CURVE1[LANG], "--", "°C", hc2_status);
    id.stat.hc2_heatcurve2         = addGroupValueHelper(km271StatTopics.HC2_HEAT_CURVE2[LANG], "--", "°C", hc2_status);
    id.stat.hc2_heatcurve3         = addGroupValueHelper(km271StatTopics.HC2_HEAT_CURVE3[LANG], "--", "°C", hc2_status);
    id.stat.hc2_mixer              = addGroupValueHelper(km271StatTopics.HC2_MIXER[LANG], "--", "%", hc2_status);
    id.stat.hc2_off_time_opt       = addGroupValueHelper(km271StatTopics.HC2_OFF_TIME_OPT[LANG], "--", "min", hc2_status);
    id.stat.hc2_on_time_opt        = addGroupValueHelper(km271StatTopics.HC2_ON_TIME_OPT[LANG], "--", "min", hc2_status);
    id.stat.hc2_pump               = addGroupValueHelper(km271StatTopics.HC2_PUMP[LANG], "--", "%", hc2_status);
    id.stat.hc2_room_setpoint      = addGroupValueHelper(km271StatTopics.HC2_ROOM_SETPOINT[LANG], "--", "°C", hc2_status);
    id.stat.hc2_room_temp          = addGroupValueHelper(km271StatTopics.HC2_ROOM_TEMP[LANG], "--", "°C", hc2_status);
    id.stat.hc2_ov1_automatic      = addGroupValueHelper(km271StatTopics.HC2_OV1_AUTOMATIC[LANG], "--", "", hc2_status);
    id.stat.hc2_frost              = addGroupValueHelper(km271StatTopics.HC2_OV1_FROST[LANG], "--", "", hc2_status);
    id.stat.hc2_holiday            = addGroupValueHelper(km271StatTopics.HC2_OV1_HOLIDAY[LANG], "--", "", hc2_status);
    
    ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc2);

    // operation states
    auto hc2_opstates1 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.hc2);
    id.stat.hc2_ov1_off_time_opt   = addGroupValueHelper(km271StatTopics.HC2_OV1_OFFTIME_OPT[LANG], "--", "", hc2_opstates1);
    id.stat.hc2_ov1_on_time_opt    = addGroupValueHelper(km271StatTopics.HC2_OV1_ONTIME_OPT[LANG], "--", "", hc2_opstates1);
    id.stat.hc2_ov1_screed_dry     = addGroupValueHelper(km271StatTopics.HC2_OV1_SCREED_DRY[LANG], "--", "", hc2_opstates1);
    id.stat.hc2_ov1_ww_prio        = addGroupValueHelper(km271StatTopics.HC2_OV1_WW_PRIO[LANG], "--", "", hc2_opstates1);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2_opstates1), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2_opstates1), "background-color: unset; width: 100%"); // only spacer
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, hc2_opstates1), "background-color: unset; width: 100%"); // only spacer
    
    auto hc2_opstates2 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.hc2);
    id.stat.hc2_ov2_day            = addGroupValueHelper(km271StatTopics.HC2_OV2_DAY[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_summer         = addGroupValueHelper(km271StatTopics.HC2_OV2_SUMMER[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_ext_sens_err   = addGroupValueHelper(km271StatTopics.HC2_OV2_EXT_SENS_ERR[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_flow_at_max    = addGroupValueHelper(km271StatTopics.HC2_OV2_FLOW_AT_MAX[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_flow_sens_err  = addGroupValueHelper(km271StatTopics.HC2_OV2_FLOW_SENS_ERR[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_no_com_remote  = addGroupValueHelper(km271StatTopics.HC2_OV2_NO_COM_REMOTE[LANG], "--", "", hc2_opstates2);
    id.stat.hc2_ov2_remote_err     = addGroupValueHelper(km271StatTopics.HC2_OV2_REMOTE_ERR[LANG], "--", "", hc2_opstates2);

    ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.hc2);

    // Program
    auto hc2prg_config  = addGroupHelper(webText.PROGRAMS[LANG], Dark, id.tab.hc2);
    id.cfg.hc2_program  = addGroupWideHelper(km271CfgTopics.HC2_PROGRAM[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer01  = addGroupWideHelper(km271CfgTopics.HC2_TIMER01[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer02  = addGroupWideHelper(km271CfgTopics.HC2_TIMER02[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer03  = addGroupWideHelper(km271CfgTopics.HC2_TIMER03[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer04  = addGroupWideHelper(km271CfgTopics.HC2_TIMER04[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer05  = addGroupWideHelper(km271CfgTopics.HC2_TIMER05[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer06  = addGroupWideHelper(km271CfgTopics.HC2_TIMER06[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer07  = addGroupWideHelper(km271CfgTopics.HC2_TIMER07[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer08  = addGroupWideHelper(km271CfgTopics.HC2_TIMER08[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer09  = addGroupWideHelper(km271CfgTopics.HC2_TIMER09[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer10  = addGroupWideHelper(km271CfgTopics.HC2_TIMER10[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer11  = addGroupWideHelper(km271CfgTopics.HC2_TIMER11[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer12  = addGroupWideHelper(km271CfgTopics.HC2_TIMER12[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer13  = addGroupWideHelper(km271CfgTopics.HC2_TIMER13[LANG], "--", hc2prg_config);
    id.cfg.hc2_timer14  = addGroupWideHelper(km271CfgTopics.HC2_TIMER14[LANG], "--", hc2prg_config);
    ESPUI.setPanelWide(hc2prg_config, true);
    #endif /* USE_HC2 */
  #endif /* USE_HC1 */

  /*-------------------------------------------------------------------------
  // TAB: Warm-Water
  -------------------------------------------------------------------------*/
  id.tab.ww = ESPUI.addControl(Tab, "", String(webText.WW[LANG]), ControlColor::None, 0, generalCallback);

  // config values
  auto ww_config                        = addGroupHelper(webText.CONFIG[LANG], Dark, id.tab.ww);
  id.cfg.ww_priority                    = addGroupCfgHelper(km271CfgTopics.WW_PRIO[LANG], "--", ww_config);
  id.cfg.ww_temp                        = addGroupCfgHelper(km271CfgTopics.WW_TEMP[LANG], "--", ww_config);
  id.cfg.ww_operation_mode              = addGroupCfgHelper(km271CfgTopics.WW_OPMODE[LANG], "--", ww_config);
  id.cfg.ww_processing                  = addGroupCfgHelper(km271CfgTopics.WW_PROCESSING[LANG], "--", ww_config);
  id.cfg.ww_circulation                 = addGroupCfgHelper(km271CfgTopics.WW_CIRCULATION[LANG], "--", ww_config);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, ww_config), "background-color: unset; width: 100%"); // only spacer
  
  // status
  auto ww_status = addGroupHelper(webText.STATUS[LANG], Dark, id.tab.ww);
  id.stat.ww_temp               = addGroupValueHelper(km271StatTopics.WW_TEMP[LANG], "--", "°C", ww_status);
  id.stat.ww_setpoint           = addGroupValueHelper(km271StatTopics.WW_SETPOINT[LANG], "--", "°C", ww_status); 
  id.stat.ww_pump_solar         = addGroupValueHelper(km271StatTopics.WW_PUMP_SOLAR[LANG], "--", "", ww_status);
  id.stat.ww_pump_circ          = addGroupValueHelper(km271StatTopics.WW_PUMP_CIRC[LANG], "--", "", ww_status);
  id.stat.ww_pump_charge        = addGroupValueHelper(km271StatTopics.WW_PUMP_CHARGE[LANG], "--", "", ww_status);
  id.stat.ww_ontime_opt         = addGroupValueHelper(km271StatTopics.WW_ONTIME_OPT[LANG], "--", "", ww_status);
  
  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.ww);

  // operation states
  auto ww_opvalues1 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.ww);
  id.stat.ww_ov1_auto           = addGroupValueHelper(km271StatTopics.WW_OV1_AUTO[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_reload         = addGroupValueHelper(km271StatTopics.WW_OV1_RELOAD[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_holiday        = addGroupValueHelper(km271StatTopics.WW_OV1_HOLIDAY[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_desinfect      = addGroupValueHelper(km271StatTopics.WW_OV1_DESINFECT[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_stay_cold      = addGroupValueHelper(km271StatTopics.WW_OV1_WW_STAY_COLD[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_err_sens       = addGroupValueHelper(km271StatTopics.WW_OV1_ERR_SENSOR[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_err_desinfect  = addGroupValueHelper(km271StatTopics.WW_OV1_ERR_DESINFECT[LANG], "--", "", ww_opvalues1);
  id.stat.ww_ov1_err_anode      = addGroupValueHelper(km271StatTopics.WW_OV1_ERR_ANODE[LANG], "--", "", ww_opvalues1);
  
  
  // operation states
  auto ww_opvalues2 = addGroupHelper(webText.OP_VALUES[LANG], Dark, id.tab.ww);
  id.stat.ww_ov2_reload         = addGroupValueHelper(km271StatTopics.WW_OV2_RELOAD[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_prio           = addGroupValueHelper(km271StatTopics.WW_OV2_PRIO[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_ontime_opt     = addGroupValueHelper(km271StatTopics.WW_OV2_ON_TIME_OPT[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_offtime_opt    = addGroupValueHelper(km271StatTopics.WW_OV2_OFF_TIME_OPT[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_manual         = addGroupValueHelper(km271StatTopics.WW_OV2_MANUAL[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_load           = addGroupValueHelper(km271StatTopics.WW_OV2_LOAD[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_hot            = addGroupValueHelper(km271StatTopics.WW_OV2_HOT[LANG], "--", "", ww_opvalues2);
  id.stat.ww_ov2_day            = addGroupValueHelper(km271StatTopics.WW_OV2_DAY[LANG], "--", "", ww_opvalues2);

  /*-------------------------------------------------------------------------
  // TAB: Burner
  -------------------------------------------------------------------------*/
  id.tab.boiler = ESPUI.addControl(Tab, "", String(webText.BURNER[LANG]), ControlColor::None, 0, generalCallback);

  auto boiler_status = addGroupHelper(webText.STATUS[LANG], Dark, id.tab.boiler);
  id.stat.boiler_ctrl                = addGroupValueHelper(km271StatTopics.BOILER_CONTROL[LANG], "--", "", boiler_status);
  id.stat.boiler_temp                = addGroupValueHelper(km271StatTopics.BOILER_TEMP[LANG], "--", "°C", boiler_status);
  id.stat.boiler_setpoint            = addGroupValueHelper(km271StatTopics.BOILER_SETPOINT[LANG], "--", "°C", boiler_status);
  id.stat.boiler_off_temp            = addGroupValueHelper(km271StatTopics.BOILER_OFF_TEMP[LANG], "--", "°C", boiler_status);
  id.stat.boiler_on_temp             = addGroupValueHelper(km271StatTopics.BOILER_ON_TEMP[LANG], "--", "°C", boiler_status);
  
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, boiler_status), "background-color: unset; width: 100%"); // only spacer
  id.cfg.max_boiler_temperature      = addGroupCfgHelper(km271CfgTopics.MAX_BOILER_TEMP[LANG], "--", boiler_status);

  auto boiler_stages = addGroupHelper(webText.OPERATION[LANG], Dark, id.tab.boiler);
  id.stat.boiler_state_stage1        = addGroupValueHelper(km271StatTopics.BOILER_STATE_STAGE1[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_stage2        = addGroupValueHelper(km271StatTopics.BOILER_STATE_STAGE2[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_active        = addGroupValueHelper(km271StatTopics.BOILER_STATE_ACTIVE[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_gastest       = addGroupValueHelper(km271StatTopics.BOILER_STATE_GASTEST[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_per_free      = addGroupValueHelper(km271StatTopics.BOILER_STATE_PER_FREE[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_per_high      = addGroupValueHelper(km271StatTopics.BOILER_STATE_PER_HIGH[LANG], "--", "", boiler_stages);
  id.stat.boiler_state_protect       = addGroupValueHelper(km271StatTopics.BOILER_STATE_PROTECT[LANG], "--", "", boiler_stages);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.boiler);

  auto boiler_error = addGroupHelper(webText.ERROR_FLAGS[LANG], Dark, id.tab.boiler);
  id.stat.boiler_err_aux_sens        = addGroupValueHelper(km271StatTopics.BOILER_ERR_AUX_SENS[LANG], "--", "", boiler_error);
  id.stat.boiler_err_burner          = addGroupValueHelper(km271StatTopics.BOILER_ERR_BURNER[LANG], "--", "", boiler_error); 
  id.stat.boiler_err_exhaust         = addGroupValueHelper(km271StatTopics.BOILER_ERR_EXHAUST[LANG], "--", "", boiler_error);
  id.stat.boiler_err_ext             = addGroupValueHelper(km271StatTopics.BOILER_ERR_EXT[LANG], "--", "", boiler_error);
  id.stat.boiler_err_gas_sens        = addGroupValueHelper(km271StatTopics.BOILER_ERR_GAS_SENS[LANG], "--", "", boiler_error);
  id.stat.boiler_err_safety          = addGroupValueHelper(km271StatTopics.BOILER_ERR_SAFETY[LANG], "--", "", boiler_error);
  id.stat.boiler_err_sens            = addGroupValueHelper(km271StatTopics.BOILER_ERR_SENSOR[LANG], "--", "", boiler_error);
  id.stat.boiler_err_stay_cold       = addGroupValueHelper(km271StatTopics.BOILER_ERR_STAY_COLD[LANG], "--", "", boiler_error);

  auto boiler_lifetimes = addGroupHelper(webText.LIFETIMES[LANG], Dark, id.tab.boiler);
  id.stat.boiler_lifetime1           = addGroupValueHelper(km271StatTopics.BOILER_LIFETIME_1[LANG], "--", "min", boiler_lifetimes);
  id.stat.boiler_lifetime2           = addGroupValueHelper(km271StatTopics.BOILER_LIFETIME_2[LANG], "--", "min", boiler_lifetimes);
  id.stat.boiler_lifetime3           = addGroupValueHelper(km271StatTopics.BOILER_LIFETIME_3[LANG], "--", "min", boiler_lifetimes);
  id.stat.boiler_lifetime4           = addGroupValueHelper(km271StatTopics.BOILER_LIFETIME_4[LANG], "--", "min", boiler_lifetimes);
  #ifdef USE_CALCULATED_CONSUMPTION
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, boiler_lifetimes), "background-color: unset; width: 100%"); // only spacer
    id.stat.oilconsumption             = addGroupValueHelper(km271StatTopics.BOILER_CONSUMPTION[LANG], "--", "L", boiler_lifetimes);
  #endif

  /*-------------------------------------------------------------------------
  // TAB: General
  -------------------------------------------------------------------------*/
  id.tab.general = ESPUI.addControl(Tab, "", String(webText.GENERAL[LANG]), ControlColor::None, 0, generalCallback);

  auto general_config                         = addGroupHelper(webText.CONFIG[LANG], Dark, id.tab.general);
  id.cfg.building_type                        = addGroupCfgHelper(km271CfgTopics.BUILDING_TYP[LANG], "--", general_config);
  id.cfg.language                             = addGroupCfgHelper(km271CfgTopics.LANGUAGE[LANG], "--", general_config);
  id.cfg.display                              = addGroupCfgHelper(km271CfgTopics.SCREEN[LANG], "--", general_config);
  id.cfg.time_offset                          = addGroupCfgHelper(km271CfgTopics.TIME_OFFSET[LANG], "--", general_config);
  id.cfg.burner_type                          = addGroupCfgHelper(km271CfgTopics.BURNER_TYP[LANG], "--", general_config);
  ESPUI.setElementStyle(ESPUI.addControl(Label, "", " ", None, general_config), "background-color: unset; width: 100%"); // only spacer

  auto general_limits = addGroupHelper(webText.LIMITS[LANG], Dark, id.tab.general);
  id.cfg.frost_protection_threshold           = addGroupCfgHelper(km271CfgTopics.FROST_THRESHOLD[LANG], "--", general_limits);
  id.cfg.summer_mode_threshold                = addGroupCfgHelper(km271CfgTopics.SUMMER_THRESHOLD[LANG], "--", general_limits);
  id.cfg.pump_logic_temp                      = addGroupCfgHelper(km271CfgTopics.PUMP_LOGIC[LANG], "--", general_limits);
  id.cfg.exhaust_gas_temperature_threshold    = addGroupCfgHelper(km271CfgTopics.EXHAUST_THRESHOLD[LANG], "--", general_limits);
  id.cfg.burner_min_modulation                = addGroupCfgHelper(km271CfgTopics.BURNER_MIN_MOD[LANG], "--", general_limits);
  id.cfg.burner_modulation_runtime            = addGroupCfgHelper(km271CfgTopics.BURNER_MOD_TIME[LANG], "--", general_limits);

  auto general_temp = addGroupHelper(webText.TEMPERATURES[LANG], Dark, id.tab.general);
  id.stat.outside_temp                = addGroupValueHelper(km271StatTopics.OUTSIDE_TEMP[LANG], "--", "°C", general_temp);
  id.stat.outside_temp_damped         = addGroupValueHelper(km271StatTopics.OUTSIDE_TEMP_DAMPED[LANG], "--", "°C", general_temp);
  id.stat.exhaust_temp                = addGroupValueHelper(km271StatTopics.EXHAUST_TEMP[LANG], "--", "°C", general_temp);

  /*-------------------------------------------------------------------------
  // TAB: Alarm
  -------------------------------------------------------------------------*/
  #ifdef USE_ALARM_MSG
    char alarmLabel1[50]={'\0'};
    char alarmLabel2[50]={'\0'};
    char alarmLabel3[50]={'\0'};
    char alarmLabel4[50]={'\0'};
    
    id.tab.alarm = ESPUI.addControl(Tab, "", webText.ALARM[LANG], ControlColor::None, 0, generalCallback);
    
    auto alarmGroup = addGroupHelper(webText.ALARMINFO[LANG], Dark, id.tab.alarm);
    snprintf(alarmLabel1, sizeof(alarmLabel1), "⚠️ %s 1", webText.MESSAGE[LANG]);
    snprintf(alarmLabel2, sizeof(alarmLabel2), "⚠️ %s 2", webText.MESSAGE[LANG]);
    snprintf(alarmLabel3, sizeof(alarmLabel3), "⚠️ %s 3", webText.MESSAGE[LANG]);
    snprintf(alarmLabel4, sizeof(alarmLabel4), "⚠️ %s 4", webText.MESSAGE[LANG]);
    id.alarm.alarm1  = addGroupWideHelper(alarmLabel1, "--", alarmGroup);
    id.alarm.alarm2  = addGroupWideHelper(alarmLabel2, "--", alarmGroup);
    id.alarm.alarm3  = addGroupWideHelper(alarmLabel3, "--", alarmGroup);
    id.alarm.alarm4  = addGroupWideHelper(alarmLabel4, "--", alarmGroup);
    ESPUI.setPanelWide(alarmGroup, true);
  #endif

  /*-------------------------------------------------------------------------
  // TAB: System
  -------------------------------------------------------------------------*/
  id.tab.system = ESPUI.addControl(Tab, "", webText.SYSTEM[LANG], ControlColor::None, 0, generalCallback);
  
  auto wiFiGroup = addGroupHelper(webText.WIFI_INFO[LANG], Dark, id.tab.system);
  id.sys.wifiIP      = addGroupValueHelper(webText.WIFI_IP[LANG], "--", "", wiFiGroup);
  id.sys.wifiSignal  = addGroupValueHelper("WiFi-Signal", "--", "%", wiFiGroup);
  id.sys.wifiRssi    = addGroupValueHelper("WiFi-Rssi", "--", "dbm", wiFiGroup);

  auto versionGroup = addGroupHelper(webText.VERSION_INFO[LANG], Dark, id.tab.system);
  id.sys.sw_version         = addGroupValueHelper(webText.SW_VERSION[LANG], "--", "", versionGroup);
  id.sys.logamatic_version  = addGroupValueHelper(webText.LOGAMATIC_VERSION[LANG], "--", "", versionGroup);
  id.sys.logamatic_modul    = addGroupValueHelper(webText.LOGAMATIC_MODUL[LANG], "--", "", versionGroup);

  ESPUI.addControl(ControlType::Separator, "", "", ControlColor::None, id.tab.system);

  auto EspGroup = addGroupHelper(webText.ESP_INFO[LANG], Dark, id.tab.system);
  id.sys.espHeapSize        = addGroupValueHelper(webText.ESP_HEAPSIZE[LANG], "--", "KB", EspGroup);
  id.sys.espFreeHeap        = addGroupValueHelper(webText.ESP_FREEHEAP[LANG], "--", "KB", EspGroup);
  id.sys.espMaxAllocHeap    = addGroupValueHelper(webText.ESP_MAXALLOCHEAP[LANG], "--", "KB", EspGroup);
  id.sys.espMinFreeHeap     = addGroupValueHelper(webText.ESP_MINFREEHEAP[LANG], "--", "KB", EspGroup);


  auto dateTimeGroup = addGroupHelper(webText.DATETIME[LANG], Dark, id.tab.system);
  id.sys.date  = addGroupValueHelper(webText.DATE[LANG], "--", "", dateTimeGroup);
  id.sys.time  = addGroupValueHelper(webText.TIME[LANG], "--", "", dateTimeGroup);


  // ESPUI.setVerbosity(Verbosity::Verbose);  // enable debug informations

  // create webUI
  ESPUI.begin("Buderus Logamatic");
  Serial.println("Webserver started");
  
}

/**
 * *******************************************************************
 * @brief   cyclic call for webUI - refresh elements by change
 * @param   none 
 * @return  none
 * *******************************************************************/
void webUICylic(){

  if (valeCompareTimer.cycleTrigger(2000)){
  
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

    // refresh id.tab.config every XX-Time
    if (cfgTabRefreshTimer.cycleTrigger(CONFIG_TAB_REFRESH_TIME) ){
      updateSystemInfo();
    }

    // check if oilcounter value changed
    #ifdef USE_OILMETER
      oilcounter = getOilmeter();
      if (oilcounter!=oilcounterOld) {
        oilcounterOld = oilcounter;
        updateOilmeter();
      } 
    #endif

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
  #ifdef USE_HC1  
    
    // HC-Operating States 1
    if (bitRead(kmStatusCpy.HC1_OperatingStates_1, 2))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[LANG]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ✋", webText.MANUAL[LANG]);
    
    ESPUI.updateLabel(id.dash.hc1_opmode, tmpMessage);

    // Summer / Winter
    if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 0))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔆", webText.SUMMER[LANG]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ❄️", webText.WINTER[LANG]);
    
    ESPUI.updateLabel(id.dash.hc1summerWinter, tmpMessage);

    // Day / Night
    if (bitRead(kmStatusCpy.HC1_OperatingStates_2, 1))
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ☀️", webText.DAY[LANG]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🌙", webText.NIGHT[LANG]);

    ESPUI.updateLabel(id.dash.hc1dayNight, tmpMessage);

    // Pump
    if (kmStatusCpy.HC1_PumpPower==0)
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[LANG]);
    else
      snprintf(tmpMessage, sizeof(tmpMessage), "%s  🟡", webText.ON[LANG]);
  
    ESPUI.updateLabel(id.dash.hc1pumpState, tmpMessage);

    ESPUI.updateLabel(id.stat.hc1_ov1_off_time_opt,   onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 0))); 
    ESPUI.updateLabel(id.stat.hc1_ov1_on_time_opt,    onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 1))); 
    ESPUI.updateLabel(id.stat.hc1_ov1_automatic,      onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 2)));  
    ESPUI.updateLabel(id.stat.hc1_ov1_ww_prio,        onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 3)));  
    ESPUI.updateLabel(id.stat.hc1_ov1_screed_dry,     onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 4)));   
    ESPUI.updateLabel(id.stat.hc1_ov2_summer,         onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 0))); 
    ESPUI.updateLabel(id.stat.hc1_ov2_day,            onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 1))); 
    ESPUI.updateLabel(id.stat.hc1_ov2_no_com_remote,  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 2))); 
    ESPUI.updateLabel(id.stat.hc1_ov2_remote_err,     errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 3)));
    ESPUI.updateLabel(id.stat.hc1_ov2_flow_sens_err,  errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 4)));
    ESPUI.updateLabel(id.stat.hc1_ov2_flow_at_max,    errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 5)));
    ESPUI.updateLabel(id.stat.hc1_ov2_ext_sens_err,   errOkString(bitRead(kmStatusCpy.HC1_OperatingStates_2, 6))); 
    ESPUI.updateLabel(id.stat.hc1_flow_setpoint,      uint8ToString(kmStatusCpy.HC1_HeatingForwardTargetTemp));    
    ESPUI.updateLabel(id.stat.hc1_flow_temp,          uint8ToString(kmStatusCpy.HC1_HeatingForwardActualTemp));        
    ESPUI.updateLabel(id.stat.hc1_heatcurve1,         uint8ToString(kmStatusCpy.HC1_HeatingCurvePlus10));       
    ESPUI.updateLabel(id.stat.hc1_heatcurve2,         uint8ToString(kmStatusCpy.HC1_HeatingCurve0));       
    ESPUI.updateLabel(id.stat.hc1_heatcurve3,         uint8ToString(kmStatusCpy.HC1_HeatingCurveMinus10));       
    ESPUI.updateLabel(id.stat.hc1_mixer,              uint8ToString(kmStatusCpy.HC1_MixingValue));             
    ESPUI.updateLabel(id.stat.hc1_off_time_opt,       uint8ToString(kmStatusCpy.HC1_SwitchOffOptimizationTime));      
    ESPUI.updateLabel(id.stat.hc1_on_time_opt,        uint8ToString(kmStatusCpy.HC1_SwitchOnOptimizationTime));    
    ESPUI.updateLabel(id.stat.hc1_frost,              onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 6)));            
    ESPUI.updateLabel(id.stat.hc1_holiday,            onOffString(bitRead(kmStatusCpy.HC1_OperatingStates_1, 5)));                
    ESPUI.updateLabel(id.stat.hc1_pump,               uint8ToString(kmStatusCpy.HC1_PumpPower));           
    ESPUI.updateLabel(id.stat.hc1_room_setpoint,      floatToString(kmStatusCpy.HC1_RoomTargetTemp));    
    ESPUI.updateLabel(id.stat.hc1_room_temp,          floatToString(kmStatusCpy.HC1_RoomActualTemp));        

    ESPUI.updateLabel(id.dash.hc1flowSetTemp, uint8ToString(kmStatusCpy.HC1_HeatingForwardTargetTemp));
    ESPUI.updateLabel(id.dash.hc1flowActTemp, uint8ToString(kmStatusCpy.HC1_HeatingForwardActualTemp));

  #else

    // HC2-Values
    #ifdef USE_HC2  

      if (bitRead(kmStatusCpy.HC2_OperatingStates_1, 2))
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[LANG]);
      else
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  ✋", webText.MANUAL[LANG]);
      
      ESPUI.updateLabel(id.dash.hc2_opmode, tmpMessage);

      // Summer / Winter
      if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 0))
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔆", webText.SUMMER[LANG]);
      else
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  ❄️", webText.WINTER[LANG]);
      
      ESPUI.updateLabel(id.dash.hc2summerWinter, tmpMessage);

      // Day / Night
      if (bitRead(kmStatusCpy.HC2_OperatingStates_2, 1))
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  ☀️", webText.DAY[LANG]);
      else
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  🌙", webText.NIGHT[LANG]);

      ESPUI.updateLabel(id.dash.hc2dayNight, tmpMessage);

      // Pump
      if (kmStatusCpy.HC2_PumpPower==0)
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[LANG]);
      else
        snprintf(tmpMessage, sizeof(tmpMessage), "%s  🟡", webText.ON[LANG]);
      
      ESPUI.updateLabel(id.dash.hc2pumpState, tmpMessage);

      ESPUI.updateLabel(id.stat.hc2_ov1_off_time_opt,   onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 0))); 
      ESPUI.updateLabel(id.stat.hc2_ov1_on_time_opt,    onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 1))); 
      ESPUI.updateLabel(id.stat.hc2_ov1_automatic,      onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 2)));  
      ESPUI.updateLabel(id.stat.hc2_ov1_ww_prio,        onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 3)));  
      ESPUI.updateLabel(id.stat.hc2_ov1_screed_dry,     onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 4)));   
      ESPUI.updateLabel(id.stat.hc2_ov2_summer,         onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 0))); 
      ESPUI.updateLabel(id.stat.hc2_ov2_day,            onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 1))); 
      ESPUI.updateLabel(id.stat.hc2_ov2_no_com_remote,  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 2))); 
      ESPUI.updateLabel(id.stat.hc2_ov2_remote_err,     errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 3)));
      ESPUI.updateLabel(id.stat.hc2_ov2_flow_sens_err,  errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 4)));
      ESPUI.updateLabel(id.stat.hc2_ov2_flow_at_max,    errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 5)));
      ESPUI.updateLabel(id.stat.hc2_ov2_ext_sens_err,   errOkString(bitRead(kmStatusCpy.HC2_OperatingStates_2, 6))); 
      ESPUI.updateLabel(id.stat.hc2_flow_setpoint,      uint8ToString(kmStatusCpy.HC2_HeatingForwardTargetTemp));    
      ESPUI.updateLabel(id.stat.hc2_flow_temp,          uint8ToString(kmStatusCpy.HC2_HeatingForwardActualTemp));        
      ESPUI.updateLabel(id.stat.hc2_heatcurve1,         uint8ToString(kmStatusCpy.HC2_HeatingCurvePlus10));       
      ESPUI.updateLabel(id.stat.hc2_heatcurve2,         uint8ToString(kmStatusCpy.HC2_HeatingCurve0));       
      ESPUI.updateLabel(id.stat.hc2_heatcurve3,         uint8ToString(kmStatusCpy.HC2_HeatingCurveMinus10));       
      ESPUI.updateLabel(id.stat.hc2_mixer,              uint8ToString(kmStatusCpy.HC2_MixingValue));             
      ESPUI.updateLabel(id.stat.hc2_off_time_opt,       uint8ToString(kmStatusCpy.HC2_SwitchOffOptimizationTime));      
      ESPUI.updateLabel(id.stat.hc2_on_time_opt,        uint8ToString(kmStatusCpy.HC2_SwitchOnOptimizationTime));    
      ESPUI.updateLabel(id.stat.hc2_frost,              onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 6)));            
      ESPUI.updateLabel(id.stat.hc2_holiday,            onOffString(bitRead(kmStatusCpy.HC2_OperatingStates_1, 5)));                
      ESPUI.updateLabel(id.stat.hc2_pump,               uint8ToString(kmStatusCpy.HC2_PumpPower));           
      ESPUI.updateLabel(id.stat.hc2_room_setpoint,      floatToString(kmStatusCpy.HC2_RoomTargetTemp));    
      ESPUI.updateLabel(id.stat.hc2_room_temp,          floatToString(kmStatusCpy.HC2_RoomActualTemp));   

      ESPUI.updateLabel(id.dash.hc2flowSetTemp, uint8ToString(kmStatusCpy.HC2_HeatingForwardTargetTemp));
      ESPUI.updateLabel(id.dash.hc2flowActTemp, uint8ToString(kmStatusCpy.HC2_HeatingForwardActualTemp));

    #endif
  
  #endif

  ESPUI.updateLabel(id.stat.ww_temp,              uint8ToString(kmStatusCpy.HotWaterActualTemp));              
  ESPUI.updateLabel(id.stat.ww_setpoint,          uint8ToString(kmStatusCpy.HotWaterTargetTemp));         
  ESPUI.updateLabel(id.stat.ww_pump_solar,        onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 2)));       
  ESPUI.updateLabel(id.stat.ww_pump_circ,         onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 1)));        
  ESPUI.updateLabel(id.stat.ww_pump_charge,       onOffString(bitRead(kmStatusCpy.HotWaterPumpStates, 0)));      
  ESPUI.updateLabel(id.stat.ww_ov2_reload,        onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 2)));       
  ESPUI.updateLabel(id.stat.ww_ov2_prio,          onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 7)));          
  ESPUI.updateLabel(id.stat.ww_ov2_ontime_opt,    onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 4)));    
  ESPUI.updateLabel(id.stat.ww_ov2_offtime_opt,   onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 3)));   
  ESPUI.updateLabel(id.stat.ww_ov2_manual,        onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 1)));        
  ESPUI.updateLabel(id.stat.ww_ov2_load,          onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 0)));          
  ESPUI.updateLabel(id.stat.ww_ov2_hot,           onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 6)));           
  ESPUI.updateLabel(id.stat.ww_ov2_day,           onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_2, 5)));           
  ESPUI.updateLabel(id.stat.ww_ov1_stay_cold,     errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 6)));     
  ESPUI.updateLabel(id.stat.ww_ov1_reload,        onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 2)));        
  ESPUI.updateLabel(id.stat.ww_ov1_holiday,       onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 3)));       
  ESPUI.updateLabel(id.stat.ww_ov1_err_sens,      errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 5)));      
  ESPUI.updateLabel(id.stat.ww_ov1_err_desinfect, errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 4))); 
  ESPUI.updateLabel(id.stat.ww_ov1_err_anode,     errOkString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 7)));     
  ESPUI.updateLabel(id.stat.ww_ov1_desinfect,     onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 1)));     
  ESPUI.updateLabel(id.stat.ww_ov1_auto,          onOffString(bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0)));          
  ESPUI.updateLabel(id.stat.ww_ontime_opt,        onOffString(kmStatusCpy.HotWaterOptimizationTime));       

  ESPUI.updateLabel(id.stat.boiler_ctrl,            cfgArrayTexts.BURNER_STATE[kmStatusCpy.BurnerStates]);          
  ESPUI.updateLabel(id.stat.boiler_temp,            uint8ToString(kmStatusCpy.BoilerForwardActualTemp));          
  ESPUI.updateLabel(id.stat.boiler_setpoint,        uint8ToString(kmStatusCpy.BoilerForwardTargetTemp));      
  ESPUI.updateLabel(id.stat.boiler_err_aux_sens,    errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 2)));  
  ESPUI.updateLabel(id.stat.boiler_err_burner,      errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 0)));      
  ESPUI.updateLabel(id.stat.boiler_err_exhaust,     errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 5)));     
  ESPUI.updateLabel(id.stat.boiler_err_ext,         errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 7)));         
  ESPUI.updateLabel(id.stat.boiler_err_gas_sens,    errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 4)));    
  ESPUI.updateLabel(id.stat.boiler_err_safety,      errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 6)));     
  ESPUI.updateLabel(id.stat.boiler_err_sens,        errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 1)));        
  ESPUI.updateLabel(id.stat.boiler_err_stay_cold,   errOkString(bitRead(kmStatusCpy.BoilerErrorStates, 3)));   
  ESPUI.updateLabel(id.stat.boiler_lifetime1,       uint8ToString(kmStatusCpy.BurnerOperatingDuration_0));     
  ESPUI.updateLabel(id.stat.boiler_lifetime2,       uint8ToString(kmStatusCpy.BurnerOperatingDuration_1));      
  ESPUI.updateLabel(id.stat.boiler_lifetime3,       uint8ToString(kmStatusCpy.BurnerOperatingDuration_2));      
  ESPUI.updateLabel(id.stat.boiler_lifetime4,       uint64ToString(kmStatusCpy.BurnerOperatingDuration_Sum));     
  ESPUI.updateLabel(id.stat.boiler_off_temp,        uint8ToString(kmStatusCpy.BurnerSwitchOffTemp));      
  ESPUI.updateLabel(id.stat.boiler_on_temp,         uint8ToString(kmStatusCpy.BurnerSwitchOnTemp));       
  ESPUI.updateLabel(id.stat.boiler_state_active,    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 3)));  
  ESPUI.updateLabel(id.stat.boiler_state_gastest,   onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 0)));  
  ESPUI.updateLabel(id.stat.boiler_state_per_free,  onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 4))); 
  ESPUI.updateLabel(id.stat.boiler_state_per_high,  onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 5))); 
  ESPUI.updateLabel(id.stat.boiler_state_protect,   onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 2)));  
  ESPUI.updateLabel(id.stat.boiler_state_stage1,    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 1)));   
  ESPUI.updateLabel(id.stat.boiler_state_stage2,    onOffString(bitRead(kmStatusCpy.BoilerOperatingStates, 6)));   

  ESPUI.updateLabel(id.stat.outside_temp,         int8ToString(kmStatusCpy.OutsideTemp));       
  ESPUI.updateLabel(id.stat.outside_temp_damped,  int8ToString(kmStatusCpy.OutsideDampedTemp));
  ESPUI.updateLabel(id.stat.exhaust_temp,         uint8ToString(kmStatusCpy.ExhaustTemp));       

  // Temperatures
  ESPUI.updateLabel(id.dash.wwSetTemp,      uint8ToString(kmStatusCpy.HotWaterTargetTemp));
  ESPUI.updateLabel(id.dash.wwActTemp,      uint8ToString(kmStatusCpy.HotWaterActualTemp));
  ESPUI.updateLabel(id.dash.burnerSetTemp,  uint8ToString(kmStatusCpy.BoilerForwardTargetTemp));
  ESPUI.updateLabel(id.dash.burnerActTemp,  uint8ToString(kmStatusCpy.BoilerForwardActualTemp));

  // Logamaitc
  snprintf(tmpMessage, sizeof(tmpMessage), "%i.%i", kmStatusCpy.ControllerVersionMain, kmStatusCpy.ControllerVersionSub);
  ESPUI.updateLabel(id.sys.logamatic_version, tmpMessage);
  ESPUI.updateLabel(id.sys.logamatic_modul, uint8ToString(kmStatusCpy.Modul));

  // Burner Status
  if (kmStatusCpy.BurnerStates==0)
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  ⚪️", webText.OFF[LANG]);
  else
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔥", webText.ON[LANG]);

  ESPUI.updateLabel(id.dash.burnerState, tmpMessage);

  // WW-Operating State
  if (bitRead(kmStatusCpy.HotWaterOperatingStates_1, 0))
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  🔄", webText.AUTOMATIC[LANG]);
  else
    snprintf(tmpMessage, sizeof(tmpMessage), "%s  ✋", webText.MANUAL[LANG]);
    
  ESPUI.updateLabel(id.dash.ww_opmode, tmpMessage);

  // caclulated oilconsumption
  #ifdef USE_CALCULATED_CONSUMPTION
    ESPUI.updateLabel(id.stat.oilconsumption ,  doubleToString(kmStatusCpy.BurnerCalcOilConsumption));
  #endif

}

/**
 * *******************************************************************
 * @brief   update Config values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateConfigValues(){
  
  #ifdef USE_HC1
    ESPUI.updateLabel(id.cfg.hc1_night_temp,					          kmConfigStrCpy.hc1_night_temp);					          
    ESPUI.updateLabel(id.cfg.hc1_day_temp,					            kmConfigStrCpy.hc1_day_temp);					            
    ESPUI.updateLabel(id.cfg.hc1_operation_mode,					      kmConfigStrCpy.hc1_operation_mode);					      
    ESPUI.updateLabel(id.cfg.hc1_holiday_temp,					        kmConfigStrCpy.hc1_holiday_temp);					        
    ESPUI.updateLabel(id.cfg.hc1_max_temp,					            kmConfigStrCpy.hc1_max_temp);		            
    ESPUI.updateLabel(id.cfg.hc1_interpretation,					      kmConfigStrCpy.hc1_interpretation);					      
    ESPUI.updateLabel(id.cfg.hc1_switch_on_temperature,					kmConfigStrCpy.hc1_switch_on_temperature);				
    ESPUI.updateLabel(id.cfg.hc1_switch_off_threshold,					kmConfigStrCpy.hc1_switch_off_threshold);				
    ESPUI.updateLabel(id.cfg.hc1_reduction_mode,					      kmConfigStrCpy.hc1_reduction_mode);	      
    ESPUI.updateLabel(id.cfg.hc1_heating_system,					      kmConfigStrCpy.hc1_heating_system);				      
    ESPUI.updateLabel(id.cfg.hc1_temp_offset,					          kmConfigStrCpy.hc1_temp_offset);			          
    ESPUI.updateLabel(id.cfg.hc1_remotecontrol,					        kmConfigStrCpy.hc1_remotecontrol);		

    ESPUI.updateLabel(id.cfg.hc1_program,					              kmConfigStrCpy.hc1_program);
    ESPUI.updateLabel(id.cfg.hc1_timer01,					              kmConfigStrCpy.hc1_timer01);
    ESPUI.updateLabel(id.cfg.hc1_timer02,					              kmConfigStrCpy.hc1_timer02);
    ESPUI.updateLabel(id.cfg.hc1_timer03,					              kmConfigStrCpy.hc1_timer03);
    ESPUI.updateLabel(id.cfg.hc1_timer04,					              kmConfigStrCpy.hc1_timer04);
    ESPUI.updateLabel(id.cfg.hc1_timer05,					              kmConfigStrCpy.hc1_timer05);
    ESPUI.updateLabel(id.cfg.hc1_timer06,					              kmConfigStrCpy.hc1_timer06);
    ESPUI.updateLabel(id.cfg.hc1_timer07,					              kmConfigStrCpy.hc1_timer07);
    ESPUI.updateLabel(id.cfg.hc1_timer08,					              kmConfigStrCpy.hc1_timer08);
    ESPUI.updateLabel(id.cfg.hc1_timer09,					              kmConfigStrCpy.hc1_timer09);
    ESPUI.updateLabel(id.cfg.hc1_timer10,					              kmConfigStrCpy.hc1_timer10);
    ESPUI.updateLabel(id.cfg.hc1_timer11,					              kmConfigStrCpy.hc1_timer11);
    ESPUI.updateLabel(id.cfg.hc1_timer12,					              kmConfigStrCpy.hc1_timer12);
    ESPUI.updateLabel(id.cfg.hc1_timer13,					              kmConfigStrCpy.hc1_timer13);
    ESPUI.updateLabel(id.cfg.hc1_timer14,					              kmConfigStrCpy.hc1_timer14);

    ESPUI.updateSelect(id.ctrl.hc1_opmode, hc_opmode_optval[kmConfigNumCpy.hc1_operation_mode]);
    ESPUI.updateSelect(id.ctrl.hc1_program, cfgArrayTexts.HC_PROGRAM[kmConfigNumCpy.hc1_program]);
    ESPUI.updateSlider(id.ctrl.hc1_interpretation, kmConfigNumCpy.hc1_interpretation);
    ESPUI.updateSlider(id.ctrl.hc1_switch_off_threshold, kmConfigNumCpy.hc1_switch_off_threshold);


  #else

    #ifdef USE_HC2
      ESPUI.updateLabel(id.cfg.hc2_night_temp,					          kmConfigStrCpy.hc2_night_temp);			          
      ESPUI.updateLabel(id.cfg.hc2_day_temp,					            kmConfigStrCpy.hc2_day_temp);			            
      ESPUI.updateLabel(id.cfg.hc2_operation_mode,					      kmConfigStrCpy.hc2_operation_mode);					      
      ESPUI.updateLabel(id.cfg.hc2_holiday_temp,					        kmConfigStrCpy.hc2_holiday_temp);			        
      ESPUI.updateLabel(id.cfg.hc2_max_temp,					            kmConfigStrCpy.hc2_max_temp);		            
      ESPUI.updateLabel(id.cfg.hc2_interpretation,					      kmConfigStrCpy.hc2_interpretation);					      
      ESPUI.updateLabel(id.cfg.hc2_switch_on_temperature,					kmConfigStrCpy.hc2_switch_on_temperature);					
      ESPUI.updateLabel(id.cfg.hc2_switch_off_threshold,					kmConfigStrCpy.hc2_switch_off_threshold);				
      ESPUI.updateLabel(id.cfg.hc2_reduction_mode,					      kmConfigStrCpy.hc2_reduction_mode);	      
      ESPUI.updateLabel(id.cfg.hc2_heating_system,					      kmConfigStrCpy.hc2_heating_system);				      
      ESPUI.updateLabel(id.cfg.hc2_temp_offset,					          kmConfigStrCpy.hc2_temp_offset);			          
      ESPUI.updateLabel(id.cfg.hc2_remotecontrol,					        kmConfigStrCpy.hc2_remotecontrol);

      ESPUI.updateLabel(id.cfg.hc2_program,					              kmConfigStrCpy.hc2_program);
      ESPUI.updateLabel(id.cfg.hc2_timer01,					              kmConfigStrCpy.hc2_timer01);
      ESPUI.updateLabel(id.cfg.hc2_timer02,					              kmConfigStrCpy.hc2_timer02);
      ESPUI.updateLabel(id.cfg.hc2_timer03,					              kmConfigStrCpy.hc2_timer03);
      ESPUI.updateLabel(id.cfg.hc2_timer04,					              kmConfigStrCpy.hc2_timer04);
      ESPUI.updateLabel(id.cfg.hc2_timer05,					              kmConfigStrCpy.hc2_timer05);
      ESPUI.updateLabel(id.cfg.hc2_timer06,					              kmConfigStrCpy.hc2_timer06);
      ESPUI.updateLabel(id.cfg.hc2_timer07,					              kmConfigStrCpy.hc2_timer07);
      ESPUI.updateLabel(id.cfg.hc2_timer08,					              kmConfigStrCpy.hc2_timer08);
      ESPUI.updateLabel(id.cfg.hc2_timer09,					              kmConfigStrCpy.hc2_timer09);
      ESPUI.updateLabel(id.cfg.hc2_timer10,					              kmConfigStrCpy.hc2_timer10);
      ESPUI.updateLabel(id.cfg.hc2_timer11,					              kmConfigStrCpy.hc2_timer11);
      ESPUI.updateLabel(id.cfg.hc2_timer12,					              kmConfigStrCpy.hc2_timer12);
      ESPUI.updateLabel(id.cfg.hc2_timer13,					              kmConfigStrCpy.hc2_timer13);
      ESPUI.updateLabel(id.cfg.hc2_timer14,					              kmConfigStrCpy.hc2_timer14);

      ESPUI.updateSelect(id.ctrl.hc2_opmode, hc_opmode_optval[kmConfigNumCpy.hc2_operation_mode]);
      ESPUI.updateSelect(id.ctrl.hc2_program, cfgArrayTexts.HC_PROGRAM[kmConfigNumCpy.hc2_program]);
      ESPUI.updateSlider(id.ctrl.hc2_interpretation, kmConfigNumCpy.hc2_interpretation);
      ESPUI.updateSlider(id.ctrl.hc2_switch_off_threshold, kmConfigNumCpy.hc2_switch_off_threshold);

    #endif

  #endif

  ESPUI.updateLabel(id.cfg.ww_priority,					              kmConfigStrCpy.ww_priority);		              
  ESPUI.updateLabel(id.cfg.ww_temp,					                  kmConfigStrCpy.ww_temp);			                  
  ESPUI.updateLabel(id.cfg.ww_operation_mode,					        kmConfigStrCpy.ww_operation_mode);			        
  ESPUI.updateLabel(id.cfg.ww_processing,					            kmConfigStrCpy.ww_processing);			            
  ESPUI.updateLabel(id.cfg.ww_circulation,					          kmConfigStrCpy.ww_circulation);					          
  ESPUI.updateLabel(id.cfg.frost_protection_threshold,				kmConfigStrCpy.frost_protection_threshold);				
  ESPUI.updateLabel(id.cfg.summer_mode_threshold,					    kmConfigStrCpy.summer_mode_threshold);		    
  ESPUI.updateLabel(id.cfg.max_boiler_temperature,					  kmConfigStrCpy.max_boiler_temperature);					  
  ESPUI.updateLabel(id.cfg.pump_logic_temp,					          kmConfigStrCpy.pump_logic_temp);	          
  ESPUI.updateLabel(id.cfg.building_type,					            kmConfigStrCpy.building_type);				            
  ESPUI.updateLabel(id.cfg.burner_type,					              kmConfigStrCpy.burner_type);			              
  ESPUI.updateLabel(id.cfg.burner_min_modulation,					    kmConfigStrCpy.burner_min_modulation);				    
  ESPUI.updateLabel(id.cfg.burner_modulation_runtime,					kmConfigStrCpy.burner_modulation_runtime);					
  ESPUI.updateLabel(id.cfg.exhaust_gas_temperature_threshold,	kmConfigStrCpy.exhaust_gas_temperature_threshold);
  ESPUI.updateLabel(id.cfg.language,					                kmConfigStrCpy.language);
  ESPUI.updateLabel(id.cfg.display,					                  kmConfigStrCpy.display);				                  
  ESPUI.updateLabel(id.cfg.time_offset,					              kmConfigStrCpy.time_offset);
  
  ESPUI.updateSelect(id.ctrl.ww_opmode, hc_opmode_optval[kmConfigNumCpy.ww_operation_mode]);
  ESPUI.updateSlider(id.ctrl.frost_mode_threshold, kmConfigNumCpy.frost_protection_threshold);
  ESPUI.updateSlider(id.ctrl.summer_mode_threshold, kmConfigNumCpy.summer_mode_threshold);
  ESPUI.updateSlider(id.ctrl.ww_setpoint, kmConfigNumCpy.ww_temp);

}

/**
 * *******************************************************************
 * @brief   update System values
 * @param   none
 * @return  none
 * *******************************************************************/
void updateSystemInfo(){
  
  // WiFi
  ESPUI.updateLabel(id.sys.wifiIP, wifi.ipAddress);
  ESPUI.updateLabel(id.sys.wifiSignal, uint8ToString(wifi.signal));
  ESPUI.updateLabel(id.sys.wifiRssi,    int8ToString(wifi.rssi));


  ESPUI.updateLabel(id.sys.sw_version, VERSION);       

  // ESP informations
  ESPUI.updateLabel(id.sys.espHeapSize,     floatToString((float)ESP.getHeapSize()/1000.0));
  ESPUI.updateLabel(id.sys.espFreeHeap,     floatToString((float)ESP.getFreeHeap()/1000.0));
  ESPUI.updateLabel(id.sys.espMaxAllocHeap, floatToString((float)ESP.getMaxAllocHeap()/1000.0));
  ESPUI.updateLabel(id.sys.espMinFreeHeap,  floatToString((float)ESP.getMinFreeHeap()/1000.0));

  // Date and Time
  ESPUI.updateLabel(id.sys.date, getDateString());
  ESPUI.updateLabel(id.sys.time, getTimeString());

}

/**
 * *******************************************************************
 * @brief   update alarm messages
 * @param   none
 * @return  none
 * *******************************************************************/
void updateAlarmTab(){
  #ifdef USE_ALARM_MSG
    ESPUI.updateLabel(id.alarm.alarm1, kmAlarmMsgCpy.alarm1);
    ESPUI.updateLabel(id.alarm.alarm2, kmAlarmMsgCpy.alarm2);
    ESPUI.updateLabel(id.alarm.alarm3, kmAlarmMsgCpy.alarm3);
    ESPUI.updateLabel(id.alarm.alarm4, kmAlarmMsgCpy.alarm4);
  #endif
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
 * @brief   general Callback function
 * @param   sender id from sender
 * @param   type responce type 
 * @return  none
 * *******************************************************************/
void generalCallback(Control *sender, int type) {

  // HC1-OPMODE
  if(sender->id == id.ctrl.hc1_opmode) {
    for(int i=0; i<=2; i++) {
      if (strcmp(sender->value.c_str(), hc_opmode_optval[i]) ) {
        km271sendCmd(KM271_SENDCMD_HC1_OPMODE, i);
      }
    }
  }

  // HC2-OPMODE
  if(sender->id == id.ctrl.hc2_opmode) {
    for(int i=0; i<=2; i++) {
      if (strcmp(sender->value.c_str(), hc_opmode_optval[i]) ) {
        km271sendCmd(KM271_SENDCMD_HC2_OPMODE, i);
      }
    }
  }

  // WW-OPMODE
  if(sender->id == id.ctrl.ww_opmode) {
    for(int i=0; i<=2; i++) {
      if (strcmp(sender->value.c_str(), hc_opmode_optval[i]) ) {
        km271sendCmd(KM271_SENDCMD_WW_OPMODE, i);
      }
    }
  }

  // HC1-Program
  if(sender->id == id.ctrl.hc1_program) {
    for(int i=0; i<=8; i++) {
      if (strcmp(sender->value.c_str(), cfgArrayTexts.HC_PROGRAM[i]) ) {
        km271sendCmd(KM271_SENDCMD_HC1_PROGRAMM, i);
      }
    }
  }

  // HC2-Program
  if(sender->id == id.ctrl.hc2_program) {
    for(int i=0; i<=8; i++) {
      if (strcmp(sender->value.c_str(), cfgArrayTexts.HC_PROGRAM[i]) ) {
        km271sendCmd(KM271_SENDCMD_HC2_PROGRAMM, i);
      }
    }
  }

  // Frost Threshold
  if(sender->id == id.ctrl.frost_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_FROST, sender->value.toInt());
  }

  // Summer Threshold
  if(sender->id == id.ctrl.summer_mode_threshold) {
    km271sendCmd(KM271_SENDCMD_SUMMER, sender->value.toInt());
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

  // WW-Temp
  if(sender->id == id.ctrl.ww_setpoint) {
    km271sendCmd(KM271_SENDCMD_WW_SETPOINT, sender->value.toInt());
  }

  // Set new Oilcounter value
  if(sender->id == id.ctrl.oilmeter_button && type==B_UP) {
    long setval = ESPUI.getControl(id.ctrl.oilmeter_input)->value.toInt();
    cmdSetOilmeter(setval);
  }

}


