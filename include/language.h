#pragma once
#include <config.h>

#define MAX_LANG 2      // increase if you add more languages

// ======================================================================================
// mqtt commands : texts that are used as topics for KM271 commands
// ======================================================================================
typedef struct {
const char* RESTART[MAX_LANG] =                     {"/cmd/restart",                "/cmd/restart"};
const char* DATETIME[MAX_LANG] =                    {"/setvalue/setdatetime",       "/setvalue/setdatetime"};
const char* OILCNT[MAX_LANG] =                      {"/setvalue/oilcounter",        "/setvalue/oilcounter"};
const char* HC1_OPMODE[MAX_LANG] =                  {"/setvalue/hk1_betriebsart",   "/setvalue/hc1_opmode"};
const char* HC2_OPMODE[MAX_LANG] =                  {"/setvalue/hk2_betriebsart",   "/setvalue/hc2_opmode"};
const char* HC1_PRG[MAX_LANG] =                     {"/setvalue/hk1_programm",      "/setvalue/hc1_program"};
const char* HC2_PRG[MAX_LANG] =                     {"/setvalue/hk2_programm",      "/setvalue/hc2_program"};
const char* HC1_INTERPRET[MAX_LANG] =               {"/setvalue/hk1_auslegung",     "/setvalue/hc1_interpretation"};
const char* HC2_INTERPRET[MAX_LANG] =               {"/setvalue/hk2_auslegung",     "/setvalue/hc2_interpretation"};
const char* HC1_SWITCH_OFF_THRESHOLD[MAX_LANG] =    {"/setvalue/hk1_aussenhalt_ab", "/setvalue/hc1_switch_off_threshold"};
const char* HC2_SWITCH_OFF_THRESHOLD[MAX_LANG] =    {"/setvalue/hk2_aussenhalt_ab", "/setvalue/hc2_switch_off_threshold"};
const char* HC1_DAY_SETPOINT[MAX_LANG] =            {"/setvalue/hk1_tag_soll",      "/setvalue/hc1_day_setpoint"};
const char* HC2_DAY_SETPOINT[MAX_LANG] =            {"/setvalue/hk2_tag_soll",      "/setvalue/hc2_day_setpoint"};
const char* HC1_NIGHT_SETPOINT[MAX_LANG] =          {"/setvalue/hk1_nacht_soll",    "/setvalue/hc1_night_setpoint"};
const char* HC2_NIGHT_SETPOINT[MAX_LANG] =          {"/setvalue/hk2_nacht_soll",    "/setvalue/hc2_night_setpoint"};
const char* HC1_HOLIDAY_SETPOINT[MAX_LANG] =        {"/setvalue/hk1_ferien_soll",   "/setvalue/hc1_holiday_setpoint"};
const char* HC2_HOLIDAY_SETPOINT[MAX_LANG] =        {"/setvalue/hk2_ferien_soll",   "/setvalue/hc2_holiday_setpoint"};
const char* WW_OPMODE[MAX_LANG] =                   {"/setvalue/ww_betriebsart",    "/setvalue/ww_opmode"};
const char* SUMMER[MAX_LANG] =                      {"/setvalue/sommer_ab",         "/setvalue/summer_mode_threshold"};
const char* FROST[MAX_LANG] =                       {"/setvalue/frost_ab",          "/setvalue/frost_mode_threshold"};
const char* WW_SETPOINT[MAX_LANG] =                 {"/setvalue/ww_soll",           "/setvalue/ww_setpoint"};
const char* HC1_HOLIDAYS[MAX_LANG] =                {"/setvalue/hk1_ferien_tage",   "/setvalue/hc1_holidays"};
const char* HC2_HOLIDAYS[MAX_LANG] =                {"/setvalue/hk2_ferien_tage",   "/setvalue/hc2_holidays"};
} s_mqtt_cmds; 


// ======================================================================================
// mqtt messages : texts that are used for mqtt messages from KM271
// ======================================================================================
typedef struct {
const char* DATETIME_CHANGED[MAX_LANG]  =    {"Datum und Uhrzeit geändert auf",  "date and time set to"};
const char* ON[MAX_LANG]                =    {"An",                              "On"};
const char* OFF[MAX_LANG]               =    {"Aus",                             "Off"};
const char* MON[MAX_LANG]               =    {"Mo",                              "Mon"};
const char* TUE[MAX_LANG]               =    {"Di",                              "Tue"};
const char* WED[MAX_LANG]               =    {"Mi",                              "Wed"};
const char* THU[MAX_LANG]               =    {"Do",                              "Thu"};
const char* FRI[MAX_LANG]               =    {"Fr",                              "Fri"};
const char* SAT[MAX_LANG]               =    {"Sa",                              "Sat"};
const char* SUN[MAX_LANG]               =    {"So",                              "Sun"};
const char* DAYS[MAX_LANG]              =    {"Tage",                            "days"};
const char* HOURS[MAX_LANG]             =    {"Stunden",                         "days"};

const char* HC1_OPMODE_RECV[MAX_LANG]                   =    {"setvalue: hk1_betriebsart - empfangen",       "setvalue: hc1_opmode - received"};
const char* HC1_OPMODE_INVALID[MAX_LANG]                =    {"setvalue: hk1_betriebsart - ungültig",        "setvalue: hc1_opmode - invalid"};
const char* HC2_OPMODE_RECV[MAX_LANG]                   =    {"setvalue: hk2_betriebsart - empfangen",       "setvalue: hc2_opmode - received"};
const char* HC2_OPMODE_INVALID[MAX_LANG]                =    {"setvalue: hk2_betriebsart - ungültig",        "setvalue: hc2_opmode - invalid"};
const char* HC1_INTERPRET_RECV[MAX_LANG]                =    {"setvalue: hk1_auslegung - empfangen",         "setvalue: hc1_interpretation - received"};
const char* HC1_INTERPRET_INVALID[MAX_LANG]             =    {"setvalue: hk1_auslegung - ungültig",          "setvalue: hc1_interpretation - invalid"};
const char* HC2_INTERPRET_RECV[MAX_LANG]                =    {"setvalue: hk2_auslegung - empfangen",         "setvalue: hc2_interpretation - received"};
const char* HC2_INTERPRET_INVALID[MAX_LANG]             =    {"setvalue: hk2_auslegung - ungültig",          "setvalue: hc2_interpretation - invalid"};
const char* HC1_PROG_RECV[MAX_LANG]                     =    {"setvalue: hk1_programm - empfangen",          "setvalue: hc1_program - received"};
const char* HC1_PROG_INVALID[MAX_LANG]                  =    {"setvalue: hk1_programm - ungültig",           "setvalue: hc1_program - invalid"};
const char* HC2_PROG_RECV[MAX_LANG]                     =    {"setvalue: hk2_programm - empfangen",          "setvalue: hc2_program - received"};
const char* HC2_PROG_INVALID[MAX_LANG]                  =    {"setvalue: hk2_programm - ungültig",           "setvalue: hc2_program - invalid"};
const char* WW_OPMODE_RECV[MAX_LANG]                    =    {"setvalue: hk2_programm - empfangen",          "setvalue: hc2_program - received"};
const char* WW_OPMODE_INVALID[MAX_LANG]                 =    {"setvalue: hk2_programm - ungültig",           "setvalue: hc2_program - invalid"};
const char* SUMMER_RECV[MAX_LANG]                       =    {"setvalue: sommer_ab - empfangen",             "setvalue: summer_mode_threshold - received"};
const char* SUMMER_INVALID[MAX_LANG]                    =    {"setvalue: sommer_ab - ungültig",              "setvalue: summer_mode_threshold - invalid"};
const char* FROST_RECV[MAX_LANG]                        =    {"setvalue: frost_ab - empfangen",              "setvalue: frost_mode_threshold - received"};
const char* FROST_INVALID[MAX_LANG]                     =    {"setvalue: frost_ab - ungültig",               "setvalue: frost_mode_threshold - invalid"};
const char* HC1_SWITCH_OFF_THRESHOLD_RECV[MAX_LANG]     =    {"setvalue: hk1_aussenhalt_ab - empfangen",     "setvalue: hc1_switch_off_threshold - received"};
const char* HC1_SWITCH_OFF_THRESHOLD_INVALID[MAX_LANG]  =    {"setvalue: hk1_aussenhalt_ab - ungültig",      "setvalue: hc1_switch_off_threshold - invalid"};
const char* HC2_SWITCH_OFF_THRESHOLD_RECV[MAX_LANG]     =    {"setvalue: hk2_aussenhalt_ab - empfangen",     "setvalue: hc2_switch_off_threshold - received"};
const char* HC2_SWITCH_OFF_THRESHOLD_INVALID[MAX_LANG]  =    {"setvalue: hk2_aussenhalt_ab - ungültig",      "setvalue: hc2_switch_off_threshold - invalid"};
const char* WW_SETPOINT_RECV[MAX_LANG]                  =    {"setvalue: ww_soll - empfangen",               "setvalue: ww_setpoint - received"};
const char* WW_SETPOINT_INVALID[MAX_LANG]               =    {"setvalue: ww_soll - ungültig",                "setvalue: ww_setpoint - invalid"};
const char* HC1_DAY_SETPOINT_RECV[MAX_LANG]             =    {"setvalue: hk1_tag_soll - empfangen",          "setvalue: hc1_day_setpoint - received"};
const char* HC1_DAY_SETPOINT_INVALID[MAX_LANG]          =    {"setvalue: hk1_tag_soll - ungültig",           "setvalue: hc1_day_setpoint - invalid"};
const char* HC1_NIGHT_SETPOINT_RECV[MAX_LANG]           =    {"setvalue: hk1_nacht_soll - empfangen",        "setvalue: hc1_night_setpoint - received"};
const char* HC1_NIGHT_SETPOINT_INVALID[MAX_LANG]        =    {"setvalue: hk1_nacht_soll - ungültig",         "setvalue: hc1_night_setpoint - invalid"};
const char* HC2_DAY_SETPOINT_RECV[MAX_LANG]             =    {"setvalue: hk2_tag_soll - empfangen",          "setvalue: hc2_day_setpoint - received"};
const char* HC2_DAY_SETPOINT_INVALID[MAX_LANG]          =    {"setvalue: hk2_tag_soll - ungültig",           "setvalue: hc2_day_setpoint - invalid"};
const char* HC2_NIGHT_SETPOINT_RECV[MAX_LANG]           =    {"setvalue: hk2_nacht_soll - empfangen",        "setvalue: hc2_night_setpoint - received"};
const char* HC2_NIGHT_SETPOINT_INVALID[MAX_LANG]        =    {"setvalue: hk2_nacht_soll - ungültig",         "setvalue: hc2_night_setpoint - invalid"};
const char* HC1_HOLIDAY_SETPOINT_RECV[MAX_LANG]         =    {"setvalue: hk1_ferien_soll - empfangen",       "setvalue: hc1_holiday_setpoint - received"};
const char* HC1_HOLIDAY_SETPOINT_INVALID[MAX_LANG]      =    {"setvalue: hk1_ferien_soll - ungültig",        "setvalue: hc1_holiday_setpoint - invalid"};
const char* HC2_HOLIDAY_SETPOINT_RECV[MAX_LANG]         =    {"setvalue: hk2_ferien_soll - empfangen",       "setvalue: hc2_holiday_setpoint - received"};
const char* HC2_HOLIDAY_SETPOINT_INVALID[MAX_LANG]      =    {"setvalue: hk2_ferien_soll - ungültig",        "setvalue: hc2_holiday_setpoint - invalid"};
const char* HC1_HOLIDAYS_RECV[MAX_LANG]                 =    {"setvalue: hk1_ferien_tage - empfangen",       "setvalue: hc1_holidays - received"};
const char* HC1_HOLIDAYS_INVALID[MAX_LANG]              =    {"setvalue: hk1_ferien_tage - ungültig",        "setvalue: hc1_holidays - invalid"};
const char* HC2_HOLIDAYS_RECV[MAX_LANG]                 =    {"setvalue: hk2_ferien_tage - empfangen",       "setvalue: hc2_holidays - received"};
const char* HC2_HOLIDAYS_INVALID[MAX_LANG]              =    {"setvalue: hk2_ferien_tage - ungültig",        "setvalue: hc2_holidays - invalid"};
} s_mqtt_messags;   


// ======================================================================================
// topic decription for config values from KM271
// ======================================================================================
typedef struct {
const char* FROST_THRESHOLD[MAX_LANG]           =    {"Frost_ab",                    "frost_protection_threshold"};
const char* SUMMER_THRESHOLD[MAX_LANG]          =    {"Sommer_ab",                   "summer_mode_threshold"};
const char* HC1_NIGHT_TEMP[MAX_LANG]            =    {"HK1_Nachttemperatur",         "hc1_night_temp"};
const char* HC1_DAY_TEMP[MAX_LANG]              =    {"HK1_Tagtemperatur",           "hc1_day_temp"};
const char* HC1_OPMODE[MAX_LANG]                =    {"HK1_Betriebsart",             "hc1_operation_mode"};
const char* HC1_HOLIDAY_TEMP[MAX_LANG]          =    {"HK1_Urlaubtemperatur",        "hc1_holiday_temp"};
const char* HC1_MAX_TEMP[MAX_LANG]              =    {"HK1_Max_Temperatur",          "hc1_max_temp"};
const char* HC1_INTERPR[MAX_LANG]               =    {"HK1_Auslegung",               "hc1_interpretation"};
const char* HC1_SWITCH_ON_TEMP[MAX_LANG]        =    {"HK1_Aufschalttemperatur",     "hc1_switch_on_temperature"};
const char* HC1_SWITCH_OFF_THRESHOLD[MAX_LANG]  =    {"HK1_Aussenhalt_ab",           "hc1_switch_off_threshold"};
const char* HC1_REDUCTION_MODE[MAX_LANG]        =    {"HK1_Absenkungsart",           "hc1_reduction_mode"};
const char* HC1_HEATING_SYSTEM[MAX_LANG]        =    {"HK1_Heizsystem",              "hc1_heating_system"};
const char* HC1_TEMP_OFFSET[MAX_LANG]           =    {"HK1_Temperatur_Offset",       "hc1_temp_offset"};
const char* HC1_REMOTECTRL[MAX_LANG]            =    {"HK1_Fernbedienung",           "hc1_remotecontrol"};
const char* HC2_NIGHT_TEMP[MAX_LANG]            =    {"HK2_Nachttemperatur",         "hc2_night_temp"};
const char* HC2_DAY_TEMP[MAX_LANG]              =    {"HK2_Tagtemperatur",           "hc2_day_temp"};
const char* HC2_OPMODE[MAX_LANG]                =    {"HK2_Betriebsart",             "hc2_operation_mode"};
const char* HC2_HOLIDAY_TEMP[MAX_LANG]          =    {"HK2_Urlaubtemperatur",        "hc2_holiday_temp"};
const char* HC2_MAX_TEMP[MAX_LANG]              =    {"HK2_Max_Temperatur",          "hc2_max_temp"};
const char* HC2_INTERPR[MAX_LANG]               =    {"HK2_Auslegung",               "hc2_interpretation"};
const char* WW_PRIO[MAX_LANG]                   =    {"WW_Vorrang",                  "ww_priority"};
const char* HC2_SWITCH_ON_TEMP[MAX_LANG]        =    {"HK2_Aufschalttemperatur",     "hc2_switch_on_temperature"};
const char* HC2_SWITCH_OFF_THRESHOLD[MAX_LANG]  =    {"HK2_Aussenhalt_ab",           "hc2_switch_off_threshold"};
const char* HC2_REDUCTION_MODE[MAX_LANG]        =    {"HK2_Absenkungsart",           "hc2_reduction_mode"};
const char* HC2_HEATING_SYSTEM[MAX_LANG]        =    {"HK2_Heizsystem",              "hc2_heating_system"};
const char* HC2_TEMP_OFFSET[MAX_LANG]           =    {"HK2_Temperatur_Offset",       "hc2_temp_offset"};
const char* HC2_REMOTECTRL[MAX_LANG]            =    {"HK2_Fernbedienung",           "hc2_remotecontrol"};
const char* BUILDING_TYP[MAX_LANG]              =    {"Gebaeudeart",                 "building_type"};
const char* WW_TEMP[MAX_LANG]                   =    {"WW_Temperatur",               "ww_temp"};
const char* WW_OPMODE[MAX_LANG]                 =    {"WW_Betriebsart",              "ww_operation_mode"};
const char* WW_PROCESSING[MAX_LANG]             =    {"WW_Aufbereitung",             "ww_processing"};
const char* WW_CIRCULATION[MAX_LANG]            =    {"WW_Zirkulation",              "ww_circulation"};
const char* LANGUAGE[MAX_LANG]                  =    {"Sprache",                     "language"};
const char* SCREEN[MAX_LANG]                    =    {"Anzeige",                     "display"};
const char* BURNER_TYP[MAX_LANG]                =    {"Brennerart",                  "burner_type"};
const char* MAX_BOILER_TEMP[MAX_LANG]           =    {"Max_Kesseltemperatur",        "max_boiler_temperature"};
const char* PUMP_LOGIC[MAX_LANG]                =    {"Pumplogik",                   "pump_logic_temp"};
const char* EXHAUST_THRESHOLD[MAX_LANG]         =    {"Abgastemperaturschwelle",     "exhaust_gas_temperature_threshold"};
const char* BURNER_MIN_MOD[MAX_LANG]            =    {"Brenner_Min_Modulation",      "burner_min_modulation"};
const char* BURNER_MOD_TIME[MAX_LANG]           =    {"Brenner_Mod_Laufzeit",        "burner_modulation_runtime"};
const char* HC1_PROGRAM[MAX_LANG]               =    {"HK1_Programm",                "hc1_program"};
const char* HC1_TIMER01[MAX_LANG]               =    {"HK1_Timer01",                 "hc1_timer01"};
const char* HC1_TIMER02[MAX_LANG]               =    {"HK1_Timer02",                 "hc1_timer02"};
const char* HC1_TIMER03[MAX_LANG]               =    {"HK1_Timer03",                 "hc1_timer03"};
const char* HC1_TIMER04[MAX_LANG]               =    {"HK1_Timer04",                 "hc1_timer04"};
const char* HC1_TIMER05[MAX_LANG]               =    {"HK1_Timer05",                 "hc1_timer05"};
const char* HC1_TIMER06[MAX_LANG]               =    {"HK1_Timer06",                 "hc1_timer06"};
const char* HC1_TIMER07[MAX_LANG]               =    {"HK1_Timer07",                 "hc1_timer07"};
const char* HC1_TIMER08[MAX_LANG]               =    {"HK1_Timer08",                 "hc1_timer08"};
const char* HC1_TIMER09[MAX_LANG]               =    {"HK1_Timer09",                 "hc1_timer09"};
const char* HC1_TIMER10[MAX_LANG]               =    {"HK1_Timer10",                 "hc1_timer10"};
const char* HC1_TIMER11[MAX_LANG]               =    {"HK1_Timer11",                 "hc1_timer11"};
const char* HC1_TIMER12[MAX_LANG]               =    {"HK1_Timer12",                 "hc1_timer12"};
const char* HC1_TIMER13[MAX_LANG]               =    {"HK1_Timer13",                 "hc1_timer13"};
const char* HC1_TIMER14[MAX_LANG]               =    {"HK1_Timer14",                 "hc1_timer14"};
const char* HC2_PROGRAM[MAX_LANG]               =    {"HK2_Programm",                "hc2_program"};
const char* HC2_TIMER01[MAX_LANG]               =    {"HK2_Timer01",                 "hc2_timer01"};
const char* HC2_TIMER02[MAX_LANG]               =    {"HK2_Timer02",                 "hc2_timer02"};
const char* HC2_TIMER03[MAX_LANG]               =    {"HK2_Timer03",                 "hc2_timer03"};
const char* HC2_TIMER04[MAX_LANG]               =    {"HK2_Timer04",                 "hc2_timer04"};
const char* HC2_TIMER05[MAX_LANG]               =    {"HK2_Timer05",                 "hc2_timer05"};
const char* HC2_TIMER06[MAX_LANG]               =    {"HK2_Timer06",                 "hc2_timer06"};
const char* HC2_TIMER07[MAX_LANG]               =    {"HK2_Timer07",                 "hc2_timer07"};
const char* HC2_TIMER08[MAX_LANG]               =    {"HK2_Timer08",                 "hc2_timer08"};
const char* HC2_TIMER09[MAX_LANG]               =    {"HK2_Timer09",                 "hc2_timer09"};
const char* HC2_TIMER10[MAX_LANG]               =    {"HK2_Timer10",                 "hc2_timer10"};
const char* HC2_TIMER11[MAX_LANG]               =    {"HK2_Timer11",                 "hc2_timer11"};
const char* HC2_TIMER12[MAX_LANG]               =    {"HK2_Timer12",                 "hc2_timer12"};
const char* HC2_TIMER13[MAX_LANG]               =    {"HK2_Timer13",                 "hc2_timer13"};
const char* HC2_TIMER14[MAX_LANG]               =    {"HK2_Timer14",                 "hc2_timer14"};
const char* TIME_OFFSET[MAX_LANG]               =    {"Uhrzeit_Offset",              "time_offset"};
} s_cfg_topics;


// ======================================================================================
// topic decription for config values from KM271
// ======================================================================================
typedef struct {
const char* HC1_OV1_OFFTIME_OPT[MAX_LANG]       =   {"HK1_BW1_Ausschaltoptimierung",                "hc1_ov1_off_time_optimization"};
const char* HC1_OV1_ONTIME_OPT[MAX_LANG]        =   {"HK1_BW1_Einschaltoptimierung",                "hc1_ov1_on_time_optimization"};
const char* HC1_OV1_AUTOMATIC[MAX_LANG]         =   {"HK1_BW1_Automatik",                           "hc1_ov1_automatic"};
const char* HC1_OV1_WW_PRIO[MAX_LANG]           =   {"HK1_BW1_Warmwasservorrang",                   "hc1_ov1_ww_priority"};
const char* HC1_OV1_SCREED_DRY[MAX_LANG]        =   {"HK1_BW1_Estrichtrocknung",                    "hc1_ov1_screed_drying"};
const char* HC1_OV1_HOLIDAY[MAX_LANG]           =   {"HK1_BW1_Ferien",                              "hc1_ov1_holiday"};
const char* HC1_OV1_FROST[MAX_LANG]             =   {"HK1_BW1_Frostschutz",                         "hc1_ov1_frost_protection"};
const char* HC1_OV1_MANUAL[MAX_LANG]            =   {"HK1_BW1_Manuell",                             "hc1_ov1_manual"};
            
const char* HC1_OV2_SUMMER[MAX_LANG]            =   {"HK1_BW2_Sommer",                              "hc1_ov2_summer"};
const char* HC1_OV2_DAY[MAX_LANG]               =   {"HK1_BW2_Tag",                                 "hc1_ov2_day"};
const char* HC1_OV2_NO_COM_REMOTE[MAX_LANG]     =   {"HK1_BW2_Keine_Komm_mit_FB",                   "hc1_ov2_no_conn_to_remotectrl"};
const char* HC1_OV2_REMOTE_ERR[MAX_LANG]        =   {"HK1_BW2_FB_fehlerhaft",                       "hc1_ov2_remotectrl_error"};
const char* HC1_OV2_FLOW_SENS_ERR[MAX_LANG]     =   {"HK1_BW2_Fehler_Vorlauffuehler",               "hc1_ov2_failure_flow_sensor"};
const char* HC1_OV2_FLOW_AT_MAX[MAX_LANG]       =   {"HK1_BW2_Maximaler_Vorlauf",                   "hc1_ov2_flow_at_maximum"};
const char* HC1_OV2_EXT_SENS_ERR[MAX_LANG]      =   {"HK1_BW2_Externer_Stoereingang",               "hc1_ov2_external_signal_input"};
            
const char* HC1_FLOW_SETPOINT[MAX_LANG]         =   {"HK1_Vorlaufsolltemperatur",                   "hc1_flow_setpoint"};
const char* HC1_FLOW_TEMP[MAX_LANG]             =   {"HK1_Vorlaufisttemperatur",                    "hc1_flow_temp"};
const char* HC1_ROOM_SETPOINT[MAX_LANG]         =   {"HK1_Raumsolltemperatur",                      "hc1_room_setpoint"};
const char* HC1_ROOM_TEMP[MAX_LANG]             =   {"HK1_Raumisttemperatur",                       "hc1_room_temp"};
const char* HC1_ON_TIME_OPT[MAX_LANG]           =   {"HK1_Einschaltoptimierung",                    "hc1_on_time_opt_duration"};
const char* HC1_OFF_TIME_OPT[MAX_LANG]          =   {"HK1_Ausschaltoptimierung",                    "hc1_off_time_opt_duration"};
const char* HC1_PUMP[MAX_LANG]                  =   {"HK1_Pumpe",                                   "hc1_pump"};
const char* HC1_MIXER[MAX_LANG]                 =   {"HK1_Mischerstellung",                         "hc1_mixer"};
const char* HC1_HEAT_CURVE1[MAX_LANG]           =   {"HK1_Heizkennlinie_10_Grad",                   "hc1_heat_curve_10C"};
const char* HC1_HEAT_CURVE2[MAX_LANG]           =   {"HK1_Heizkennlinie_0_Grad",                    "hc1_heat_curve_0C"};
const char* HC1_HEAT_CURVE3[MAX_LANG]           =   {"HK1_Heizkennlinie_-10_Grad",                  "hc1_heat_curve_-10C"};

const char* HC2_OV1_OFFTIME_OPT[MAX_LANG]       =   {"HK2_BW1_Ausschaltoptimierung",                "hc2_ov1_off_time_opt"};
const char* HC2_OV1_ONTIME_OPT[MAX_LANG]        =   {"HK2_BW1_Einschaltoptimierung",                "hc2_ov1_on_time_opt"};
const char* HC2_OV1_AUTOMATIC[MAX_LANG]         =   {"HK2_BW1_Automatik",                           "hc2_ov1_automatic"};
const char* HC2_OV1_WW_PRIO[MAX_LANG]           =   {"HK2_BW1_Warmwasservorrang",                   "hc2_ov1_ww_priority"};
const char* HC2_OV1_SCREED_DRY[MAX_LANG]        =   {"HK2_BW1_Estrichtrocknung",                    "hc2_ov1_screed_drying"};
const char* HC2_OV1_HOLIDAY[MAX_LANG]           =   {"HK2_BW1_Ferien",                              "hc2_ov1_holiday"};
const char* HC2_OV1_FROST[MAX_LANG]             =   {"HK2_BW1_Frostschutz",                         "hc2_ov1_frost_protection"};
const char* HC2_OV1_MANUAL[MAX_LANG]            =   {"HK2_BW1_Manuell",                             "hc2_ov1_manual"};

const char* HC2_OV2_SUMMER[MAX_LANG]            =   {"HK2_BW2_Sommer",                              "hc2_ov2_summer"};
const char* HC2_OV2_DAY[MAX_LANG]               =   {"HK2_BW2_Tag",                                 "hc2_ov2_day"};
const char* HC2_OV2_NO_COM_REMOTE[MAX_LANG]     =   {"HK2_BW2_Keine_Komm_mit_FB",                   "hc2_ov2_no_conn_to_remotectrl"};
const char* HC2_OV2_REMOTE_ERR[MAX_LANG]        =   {"HK2_BW2_FB_fehlerhaft",                       "hc2_ov2_remotectrl_error"};
const char* HC2_OV2_FLOW_SENS_ERR[MAX_LANG]     =   {"HK2_BW2_Fehler_Vorlauffuehler",               "hc2_ov2_failure_flow_sensor"};
const char* HC2_OV2_FLOW_AT_MAX[MAX_LANG]       =   {"HK2_BW2_Maximaler_Vorlauf",                   "hc2_ov2_flow_at_maximum"};
const char* HC2_OV2_EXT_SENS_ERR[MAX_LANG]      =   {"HK2_BW2_Externer_Stoereingang",               "hc2_ov2_external_signal_input"};

const char* HC2_FLOW_SETPOINT[MAX_LANG]         =   {"HK2_Vorlaufsolltemperatur",                   "hc2_flow_setpoint"};
const char* HC2_FLOW_TEMP[MAX_LANG]             =   {"HK2_Vorlaufisttemperatur",                    "hc2_flow_temp"};
const char* HC2_ROOM_SETPOINT[MAX_LANG]         =   {"HK2_Raumsolltemperatur",                      "hc2_room_setpoint"};
const char* HC2_ROOM_TEMP[MAX_LANG]             =   {"HK2_Raumisttemperatur",                       "hc2_room_temp"};
const char* HC2_ON_TIME_OPT[MAX_LANG]           =   {"HK2_Einschaltoptimierung",                    "hc2_on_time_opt_duration"};
const char* HC2_OFF_TIME_OPT[MAX_LANG]          =   {"HK2_Ausschaltoptimierung",                    "hc2_off_time_opt_duration"};
const char* HC2_PUMP[MAX_LANG]                  =   {"HK2_Pumpe",                                   "hc2_pump"};
const char* HC2_MIXER[MAX_LANG]                 =   {"HK2_Mischerstellung",                         "hc2_mixer"};
const char* HC2_HEAT_CURVE1[MAX_LANG]           =   {"HK2_Heizkennlinie_10_Grad",                   "hc2_heat_curve_10C"};
const char* HC2_HEAT_CURVE2[MAX_LANG]           =   {"HK2_Heizkennlinie_0_Grad",                    "hc2_heat_curve_0C"};
const char* HC2_HEAT_CURVE3[MAX_LANG]           =   {"HK2_Heizkennlinie_-10_Grad",                  "hc2_heat_curve_-10C"};

const char* WW_OV1_AUTO[MAX_LANG]               =   {"WW_BW1_Automatik",                            "ww_ov1_auto"};
const char* WW_OV1_DESINFECT[MAX_LANG]          =   {"WW_BW1_Desinfektion",                         "ww_ov1_disinfection"};
const char* WW_OV1_RELOAD[MAX_LANG]             =   {"WW_BW1_Nachladung",                           "ww_ov1_reload"};
const char* WW_OV1_HOLIDAY[MAX_LANG]            =   {"WW_BW1_Ferien",                               "ww_ov1_holiday"};
const char* WW_OV1_ERR_DESINFECT[MAX_LANG]      =   {"WW_BW1_Fehler_Desinfektion",                  "ww_ov1_err_disinfection"};
const char* WW_OV1_ERR_SENSOR[MAX_LANG]         =   {"WW_BW1_Fehler_Fuehler",                       "ww_ov1_err_sensor"};
const char* WW_OV1_WW_STAY_COLD[MAX_LANG]       =   {"WW_BW1_Fehler_WW_bleibt_kalt",                "ww_ov1_ww_stays_cold"};
const char* WW_OV1_ERR_ANODE[MAX_LANG]          =   {"WW_BW1_Fehler_Anode",                         "ww_ov1_err_anode"};

const char* WW_OV2_LOAD[MAX_LANG]               =   {"WW_BW2_Laden",                                "ww_ov2_load"};
const char* WW_OV2_MANUAL[MAX_LANG]             =   {"WW_BW2_Manuell",                              "ww_ov2_manual"};
const char* WW_OV2_RELOAD[MAX_LANG]             =   {"WW_BW2_Nachladen",                            "ww_ov2_reload"};
const char* WW_OV2_OFF_TIME_OPT[MAX_LANG]       =   {"WW_BW2_Ausschaltoptimierung",                 "ww_ov2_off_time_opt_duration"};
const char* WW_OV2_ON_TIME_OPT[MAX_LANG]        =   {"WW_BW2_Einschaltoptimierung",                 "ww_ov2_on_time_opt_duration"};
const char* WW_OV2_DAY[MAX_LANG]                =   {"WW_BW2_Tag",                                  "ww_ov2_day"};
const char* WW_OV2_HOT[MAX_LANG]                =   {"WW_BW2_Warm",                                 "ww_ov2_hot"};
const char* WW_OV2_PRIO[MAX_LANG]               =   {"WW_BW2_Vorrang",                              "ww_ov2_priority"};

const char* WW_SETPOINT[MAX_LANG]               =   {"WW_Solltemperatur",                           "ww_setpoint"};
const char* WW_TEMP[MAX_LANG]                   =   {"WW_Isttemperatur",                            "ww_temp"};
const char* WW_ONTIME_OPT[MAX_LANG]             =   {"WW_Einschaltoptimierung",                     "ww_on_time_opt_duration"};
const char* WW_PUMP_CHARGE[MAX_LANG]            =   {"WW_Pumpentyp_Ladepumpe",                      "ww_pump_type_charge"};
const char* WW_PUMP_CIRC[MAX_LANG]              =   {"WW_Pumpentyp_Zirkulationspumpe",              "ww_pump_type_circulation"};
const char* WW_PUMP_SOLAR[MAX_LANG]             =   {"WW_Pumpentyp_Absenkung_Solar",                "ww_pump_type_groundwater_solar"};

const char* BOILER_SETPOINT[MAX_LANG]           =   {"Kessel_Vorlaufsolltemperatur",                "boiler_setpoint"};
const char* BOILER_TEMP[MAX_LANG]               =   {"Kessel_Vorlaufisttemperatur",                 "boiler_temp"};
const char* BOILER_ON_TEMP[MAX_LANG]            =   {"Brenner_Einschalttemperatur",                 "boiler_switch_on_temp"};
const char* BOILER_OFF_TEMP[MAX_LANG]           =   {"Brenner_Ausschalttemperatur",                 "boiler_switch_off_temp"};

const char* BOILER_ERR_BURNER[MAX_LANG]         =   {"Kessel_Fehler_Brennerstoerung",               "boiler_failure_burner"};
const char* BOILER_ERR_SENSOR[MAX_LANG]         =   {"Kessel_Fehler_Kesselfuehler",                 "boiler_failure_boiler_sensor"};
const char* BOILER_ERR_AUX_SENS[MAX_LANG]       =   {"Kessel_Fehler_Zusatzfuehler",                 "boiler_failure_aux_sensor"};
const char* BOILER_ERR_STAY_COLD[MAX_LANG]      =   {"Kessel_Fehler_Kessel_bleibt_kalt",            "boiler_failure_boiler_stays_cold"};
const char* BOILER_ERR_GAS_SENS[MAX_LANG]       =   {"Kessel_Fehler_Abgasfuehler",                  "boiler_failure_exhaust_gas_sensor"};
const char* BOILER_ERR_EXHAUST[MAX_LANG]        =   {"Kessel_Fehler_Abgas_ueber_Grenzwert",         "boiler_failure_exhaust_gas_over_limit"};
const char* BOILER_ERR_SAFETY[MAX_LANG]         =   {"Kessel_Fehler_Sicherungskette_ausgeloest",    "boiler_failure_safety_chain"};
const char* BOILER_ERR_EXT[MAX_LANG]            =   {"Kessel_Fehler_Externe_Stoerung",              "boiler_failure_external"};

const char* BOILER_STATE_GASTEST[MAX_LANG]      =   {"Kessel_Betrieb_Abgastest",                    "boiler_state_exhaust_gas_test"};
const char* BOILER_STATE_STAGE1[MAX_LANG]       =   {"Kessel_Betrieb_Betrieb_Stufe1",               "boiler_state_stage1"};
const char* BOILER_STATE_PROTECT[MAX_LANG]      =   {"Kessel_Betrieb_Kesselschutz",                 "boiler_state_boiler_protection"};
const char* BOILER_STATE_ACTIVE[MAX_LANG]       =   {"Kessel_Betrieb_Unter_Betrieb",                "boiler_state_active"};
const char* BOILER_STATE_PER_FREE[MAX_LANG]     =   {"Kessel_Betrieb_Leistung_frei",                "boiler_state_performance_free"};
const char* BOILER_STATE_PER_HIGH[MAX_LANG]     =   {"Kessel_Betrieb_Leistung_hoch",                "boiler_state_performance_high"};
const char* BOILER_STATE_STAGE2[MAX_LANG]       =   {"Kessel_Betrieb_BetriebStufe2",                "boiler_state_stage2"};

const char* BOILER_CONTROL[MAX_LANG]            =   {"Brenner_Ansteuerung",                         "burner_control"};
const char* EXHAUST_TEMP[MAX_LANG]              =   {"Abgastemperatur",                             "exhaust_gas_temp"};
const char* BOILER_LIFETIME_1[MAX_LANG]         =   {"Brenner_Laufzeit_Minuten65536",               "burner_runtime_minutes65536"};
const char* BOILER_LIFETIME_2[MAX_LANG]         =   {"Brenner_Laufzeit_Minuten256",                 "burner_runtime_minutes256"};
const char* BOILER_LIFETIME_3[MAX_LANG]         =   {"Brenner_Laufzeit_Minuten",                    "burner_runtime_minutes"};
const char* BOILER_LIFETIME_4[MAX_LANG]         =   {"Brenner_Laufzeit_Summe",                      "burner_runtime_overall"};

const char* OUTSIDE_TEMP[MAX_LANG]              =   {"Aussentemperatur",                            "outside_temp"};
const char* OUTSIDE_TEMP_DAMPED[MAX_LANG]       =   {"Aussentemperatur_gedaempft",                  "outside_temp_damped"};

const char* VERSION_VK[MAX_LANG]                =   {"Versionsnummer_VK",                           "version_vk"};
const char* VERSION_NK[MAX_LANG]                =   {"Versionsnummer_NK",                           "version_nk"};
const char* MODULE_ID[MAX_LANG]                 =   {"Modulkennung",                                "module_id"};

const char* ALARM_EXHAUST[MAX_LANG]             =   {"ERR_Alarmstatus_Abgasfuehler",                "err_alarm_exhaust"};
const char* ALARM_02[MAX_LANG]                  =   {"ERR_Alarmstatus_02",                          "err_alarm_02"};
const char* ALARM_BOILER_FLOW[MAX_LANG]         =   {"ERR_Alarmstatus_Kesselvorlauffuehler",        "err_alarm_boiler_flow_sensor"};
const char* ALARM_08[MAX_LANG]                  =   {"ERR_Alarmstatus_08",                          "err_alarm_08"};
const char* ALARM_BURNER[MAX_LANG]              =   {"ERR_Alarmstatus_Brenner",                     "err_alarm_burner"};
const char* ALARM_20[MAX_LANG]                  =   {"ERR_Alarmstatus_20",                          "err_alarm_20"};
const char* ALARM_HC2_FLOW_SENS[MAX_LANG]       =   {"ERR_Alarmstatus_HK2-Vorlauffuehler",          "err_alarm_HK2-flow_sensor"};
const char* ALARM_80[MAX_LANG]                  =   {"ERR_Alarmstatus_80",                          "err_alarm_80"};
} s_stat_topics;

// ======================================================================================
// topic decription for error messages buffer from KM271
// ======================================================================================
typedef struct {
const char* ERR_BUFF_1[MAX_LANG]                =    {"Fehlerspeicher1",                            "error_buffer1"};
const char* ERR_BUFF_2[MAX_LANG]                =    {"Fehlerspeicher2",                            "error_buffer2"};
const char* ERR_BUFF_3[MAX_LANG]                =    {"Fehlerspeicher3",                            "error_buffer3"};
const char* ERR_BUFF_4[MAX_LANG]                =    {"Fehlerspeicher4",                            "error_buffer4"};
} s_error_topics;

// ======================================================================================
// decription for encode different config value arrays
// ======================================================================================
#if LANG==0 // GERMAN
typedef struct {
const char* OPMODE[3]={"Nacht", "Tag", "Automatik"};
const char* SCREEN[4]={"Automatik", "Kessel", "Warmwasser", "Aussen"};
const char* LANGUAGE[6]={"DE", "FR", "IT", "NL", "EN", "PL"};
const char* REDUCT_MODE[4]={"Abschalt", "Reduziert", "Raumhalt", "Aussenhalt"};
const char* SUMMER[23]={"Sommer","10 °C","11 °C","12 °C","13 °C","14 °C","15 °C","16 °C","17 °C","18 °C","19 °C","20 °C","21 °C","22 °C","23 °C","24 °C","25 °C","26 °C","27 °C","28 °C","29 °C","30 °C","Winter"};
const char* SWITCH_ON_TEMP[11]={"Aus","1","2","3","4","5","6","7","8","9","10"};
const char* HEATING_SYSTEM[4]={"Aus","Heizkoerper","-","Fussboden"};
const char* ON_OFF[2]={"Aus","An"};
const char* BUILDING_TYPE[3]={"Leicht","Mittel","Schwer"};
const char* CIRC_INTERVAL[8]={"Aus","1","2","3","4","5","6","An"};
const char* BURNER_TYPE[3]={"1-stufig","2-stufig","Modulierend"};
const char* EXHAUST_GAS_THRESHOLD[42]={"Aus","50","55","60","65","70","75","80","85","90","95","100","105","110","115","120","125","130","135","140","145","150","155","160","165","170","175","180","185","190","195","200","205","210","215","220","225","230","235","240","245","250"};
const char* HC_PROGRAM[9]={"Eigen","Familie","Frueh","Spaet","Vormittag","Nachmittag","Mittag","Single","Senior"};
} s_cfg_arrays;

#else // ENGLISH
typedef struct {
const char* OPMODE[3]={"night", "day", "auto"};
const char* SCREEN[4]={"auto", "boiler", "DHW", "outdoor"};
const char* LANGUAGE[6]={"DE", "FR", "IT", "NL", "EN", "PL"};
const char* REDUCT_MODE[4]={"off", "fixed", "room", "outdoors"};
const char* SUMMER[23]={"summer","10 °C","11 °C","12 °C","13 °C","14 °C","15 °C","16 °C","17 °C","18 °C","19 °C","20 °C","21 °C","22 °C","23 °C","24 °C","25 °C","26 °C","27 °C","28 °C","29 °C","30 °C","winter"};
const char* SWITCH_ON_TEMP[11]={"off","1","2","3","4","5","6","7","8","9","10"};
const char* HEATING_SYSTEM[4]={"off","radiator","-","underfloor"};
const char* ON_OFF[2]={"off","on"};
const char* BUILDING_TYPE[3]={"light","medium","heavy"};
const char* CIRC_INTERVAL[8]={"off","1","2","3","4","5","6","on"};
const char* BURNER_TYPE[3]={"1-stage","2-stage","modulated"};
const char* EXHAUST_GAS_THRESHOLD[42]={"off","50","55","60","65","70","75","80","85","90","95","100","105","110","115","120","125","130","135","140","145","150","155","160","165","170","175","180","185","190","195","200","205","210","215","220","225","230","235","240","245","250"};
const char* HC_PROGRAM[9]={"custom","family","early","late","AM","PM","noon","single","senior"};
} s_cfg_arrays;
#endif

// ======================================================================================
// decription for encode different config value arrays
// ======================================================================================
#if LANG==0 // GERMAN
//const char* ERROR_BUFFER[93]={[0]="Kein Fehler",[2]="Aussenfuehler defekt",[3]="HK1-Vorlauffuehler defekt",[87]="Ruecklauffuehler defekt",[92]="RESET"};
typedef struct {
const char* idx[28] = {
                        "Kein Fehler",
                        "Aussenfuehler defekt",
                        "HK1-Vorlauffuehler defekt",
                        "HK2-Vorlauffuehler defekt",
                        "Warmwasserfuehler defekt",
                        "Warmwasser bleibt kalt",
                        "Stoerung thermische Desinfektion",
                        "HK1-Fernbedienung defekt",
                        "HK2-Fernbedienung defekt",
                        "Keine Kommunikation mit HK1-Fernbedienung",
                        "Keine Kommunikation mit HK2-Fernbedienung",
                        "Stoerung Brenner 1",
                        "Keine Verbindung mit Kessel 1",
                        "Interner Fehler Nr. 1",
                        "Interner Fehler Nr. 2",
                        "Interner Fehler Nr. 3",
                        "Interner Fehler Nr. 4",
                        "Kesselvorlauffuehler defekt",
                        "Kesselzusatzfuehler defekt",
                        "Kessel bleibt kalt",
                        "Stoerung Brenner",
                        "Stoerung Sicherheitskette",
                        "Externe Stoerung Kessel",
                        "Abgasfuehler defekt",
                        "Abgasgrenze ueberschritten",
                        "Ruecklauffuehler defekt",
                        "RESET",
                        "unbekannter Fehler"
                        };
} s_err_array;
#else // ENGLISH
typedef struct {
const char* idx[28] = {
                        "no error",
                        "failure outdoor sensor",
                        "failure hc1-flow sensor",
                        "failure hc1-flow sensor",
                        "failure warm water sensor",
                        "Warmwasser bleibt kalt",
                        "Fault thermal disinfection",
                        "Fault hc1-remotecontrol",
                        "Fault hc1-remotecontrol",
                        "no communication to hc1-remote control",
                        "no communication to hc1-remote control",
                        "Fault Burner 1",
                        "no connection with boiler 1",
                        "internal Error Nr. 1",
                        "internal Error Nr. 2",
                        "internal Error Nr. 3",
                        "internal Error Nr. 4",
                        "failure Boiler flow sensor",
                        "failure Boiler auxiliary sensor",
                        "Boiler remains cold",
                        "Fault Burner",
                        "Fault safety chain",
                        "External fault boiler",
                        "failure Exhaust gas sensor",
                        "Exhaust limit exceeded",
                        "failure Return flow sensor",
                        "RESET",
                        "unknown error"
                        };
} s_err_array;
#endif