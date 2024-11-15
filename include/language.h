#pragma once
#include <config.h>

#define MAX_LANG 2 // increase if you add more languages

struct s_lang {
  const char *CODE[MAX_LANG] = {"de", "en"};
};

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages from KM271
// ======================================================================================
namespace WEB_TXT {
constexpr const char *AUTOMATIC[] = {"Automatik", "Automatic"};
constexpr const char *MANUAL[] = {"Handbetrieb", "Manual"};
constexpr const char *DAY[] = {"Tag", "Day"};
constexpr const char *NIGHT[] = {"Nacht", "Night"};
constexpr const char *SUMMER[] = {"Sommer", "Summer"};
constexpr const char *WINTER[] = {"Winter", "Winter"};
constexpr const char *ON[] = {"EIN", "ON"};
constexpr const char *OFF[] = {"AUS", "OFF"};
constexpr const char *OK[] = {"OK", "OK"};
constexpr const char *ERROR[] = {"FEHLER", "ERROR"};
constexpr const char *MAN_DAY[] = {"Handbetrieb:Tag", "Manual:Day"};
constexpr const char *MAN_NIGHT[] = {"Handbetrieb:Nacht", "Manual:Night"};
constexpr const char *ACTIVE[] = {"aktiv", "active"};
constexpr const char *INACTIVE[] = {"inaktiv", "inactive"};
constexpr const char *FULL_DUPLEX[] = {"Vollduplex", "Full-Duplex"};
constexpr const char *CONNECTED[] = {"verbunden", "connected"};
constexpr const char *NOT_CONNECTED[] = {"nicht verbunden", "not connected"};
}; // namespace WEB_TXT

// ======================================================================================
// mqtt commands : texts that are used as topics for KM271 commands
// ======================================================================================
namespace MQTT_CMD {
constexpr const char *RESTART[MAX_LANG] = {"/cmd/restart", "/cmd/restart"};
constexpr const char *SERVICE[MAX_LANG] = {"/cmd/service", "/cmd/service"};
constexpr const char *SIMDATA[MAX_LANG] = {"/cmd/simdata", "/cmd/simdata"};
constexpr const char *DEBUG[MAX_LANG] = {"/cmd/debug", "/cmd/debug"};
constexpr const char *SET_DEBUG_FLT[MAX_LANG] = {"/cmd/setdebugflt", "/cmd/setdebugflt"};
constexpr const char *GET_DEBUG_FLT[MAX_LANG] = {"/cmd/getdebugflt", "/cmd/getdebugflt"};
constexpr const char *DATETIME[MAX_LANG] = {"/setvalue/setdatetime", "/setvalue/setdatetime"};
constexpr const char *OILCNT[MAX_LANG] = {"/setvalue/oilcounter", "/setvalue/oilcounter"};
}; // namespace MQTT_CMD

// ======================================================================================
// mqtt messages : texts that are used for mqtt messages from KM271
// ======================================================================================
namespace MQTT_MSG {
constexpr const char *DATETIME_CHANGED[MAX_LANG] = {"Datum und Uhrzeit geändert auf", "date and time set to"};
constexpr const char *ON[MAX_LANG] = {"An", "On"};
constexpr const char *OFF[MAX_LANG] = {"Aus", "Off"};
constexpr const char *MON[MAX_LANG] = {"Mo", "Mon"};
constexpr const char *TUE[MAX_LANG] = {"Di", "Tue"};
constexpr const char *WED[MAX_LANG] = {"Mi", "Wed"};
constexpr const char *THU[MAX_LANG] = {"Do", "Thu"};
constexpr const char *FRI[MAX_LANG] = {"Fr", "Fri"};
constexpr const char *SAT[MAX_LANG] = {"Sa", "Sat"};
constexpr const char *SUN[MAX_LANG] = {"So", "Sun"};
constexpr const char *DAYS[MAX_LANG] = {"Tage", "days"};
constexpr const char *HOURS[MAX_LANG] = {"Stunden", "hours"};
}; // namespace MQTT_MSG

// ======================================================================================
// topic decription for config values from KM271
// ======================================================================================
namespace KM_CFG_TOPIC {
constexpr const char *HC1_FROST_THRESHOLD[MAX_LANG] = {"HK1_Frost_ab", "hc1_frost_protection_threshold"};
constexpr const char *HC1_SUMMER_THRESHOLD[MAX_LANG] = {"HK1_Sommer_ab", "hc1_summer_mode_threshold"};
constexpr const char *HC2_FROST_THRESHOLD[MAX_LANG] = {"HK2_Frost_ab", "hc2_frost_protection_threshold"};
constexpr const char *HC2_SUMMER_THRESHOLD[MAX_LANG] = {"HK2_Sommer_ab", "hc2_summer_mode_threshold"};
constexpr const char *HC1_NIGHT_TEMP[MAX_LANG] = {"HK1_Nachttemperatur", "hc1_night_temp"};
constexpr const char *HC1_DAY_TEMP[MAX_LANG] = {"HK1_Tagtemperatur", "hc1_day_temp"};
constexpr const char *HC1_OPMODE[MAX_LANG] = {"HK1_Betriebsart", "hc1_operation_mode"};
constexpr const char *HC1_HOLIDAY_TEMP[MAX_LANG] = {"HK1_Urlaubtemperatur", "hc1_holiday_temp"};
constexpr const char *HC1_MAX_TEMP[MAX_LANG] = {"HK1_Max_Temperatur", "hc1_max_temp"};
constexpr const char *HC1_INTERPR[MAX_LANG] = {"HK1_Auslegung", "hc1_interpretation"};
constexpr const char *HC1_SWITCH_ON_TEMP[MAX_LANG] = {"HK1_Aufschalttemperatur", "hc1_switch_on_temperature"};
constexpr const char *HC1_SWITCH_OFF_THRESHOLD[MAX_LANG] = {"HK1_Aussenhalt_ab", "hc1_switch_off_threshold"};
constexpr const char *HC1_REDUCTION_MODE[MAX_LANG] = {"HK1_Absenkungsart", "hc1_reduction_mode"};
constexpr const char *HC1_HEATING_SYSTEM[MAX_LANG] = {"HK1_Heizsystem", "hc1_heating_system"};
constexpr const char *HC1_TEMP_OFFSET[MAX_LANG] = {"HK1_Temperatur_Offset", "hc1_temp_offset"};
constexpr const char *HC1_REMOTECTRL[MAX_LANG] = {"HK1_Fernbedienung", "hc1_remotecontrol"};
constexpr const char *HC2_NIGHT_TEMP[MAX_LANG] = {"HK2_Nachttemperatur", "hc2_night_temp"};
constexpr const char *HC2_DAY_TEMP[MAX_LANG] = {"HK2_Tagtemperatur", "hc2_day_temp"};
constexpr const char *HC2_OPMODE[MAX_LANG] = {"HK2_Betriebsart", "hc2_operation_mode"};
constexpr const char *HC2_HOLIDAY_TEMP[MAX_LANG] = {"HK2_Urlaubtemperatur", "hc2_holiday_temp"};
constexpr const char *HC2_MAX_TEMP[MAX_LANG] = {"HK2_Max_Temperatur", "hc2_max_temp"};
constexpr const char *HC2_INTERPR[MAX_LANG] = {"HK2_Auslegung", "hc2_interpretation"};
constexpr const char *WW_PRIO[MAX_LANG] = {"WW_Vorrang", "ww_priority"};
constexpr const char *HC2_SWITCH_ON_TEMP[MAX_LANG] = {"HK2_Aufschalttemperatur", "hc2_switch_on_temperature"};
constexpr const char *HC2_SWITCH_OFF_THRESHOLD[MAX_LANG] = {"HK2_Aussenhalt_ab", "hc2_switch_off_threshold"};
constexpr const char *HC2_REDUCTION_MODE[MAX_LANG] = {"HK2_Absenkungsart", "hc2_reduction_mode"};
constexpr const char *HC2_HEATING_SYSTEM[MAX_LANG] = {"HK2_Heizsystem", "hc2_heating_system"};
constexpr const char *HC2_TEMP_OFFSET[MAX_LANG] = {"HK2_Temperatur_Offset", "hc2_temp_offset"};
constexpr const char *HC2_REMOTECTRL[MAX_LANG] = {"HK2_Fernbedienung", "hc2_remotecontrol"};
constexpr const char *BUILDING_TYP[MAX_LANG] = {"Gebaeudeart", "building_type"};
constexpr const char *WW_TEMP[MAX_LANG] = {"WW_Temperatur", "ww_temp"};
constexpr const char *WW_OPMODE[MAX_LANG] = {"WW_Betriebsart", "ww_operation_mode"};
constexpr const char *WW_PROCESSING[MAX_LANG] = {"WW_Aufbereitung", "ww_processing"};
constexpr const char *WW_CIRCULATION[MAX_LANG] = {"WW_Zirkulation", "ww_circulation"};
constexpr const char *LANGUAGE[MAX_LANG] = {"Sprache", "language"};
constexpr const char *SCREEN[MAX_LANG] = {"Anzeige", "display"};
constexpr const char *BURNER_TYP[MAX_LANG] = {"Brennerart", "burner_type"};
constexpr const char *MAX_BOILER_TEMP[MAX_LANG] = {"Max_Kesseltemperatur", "max_boiler_temperature"};
constexpr const char *PUMP_LOGIC[MAX_LANG] = {"Pumplogik", "pump_logic_temp"};
constexpr const char *EXHAUST_THRESHOLD[MAX_LANG] = {"Abgastemperaturschwelle", "exhaust_gas_temperature_threshold"};
constexpr const char *BURNER_MIN_MOD[MAX_LANG] = {"Brenner_Min_Modulation", "burner_min_modulation"};
constexpr const char *BURNER_MOD_TIME[MAX_LANG] = {"Brenner_Mod_Laufzeit", "burner_modulation_runtime"};
constexpr const char *HC1_PROGRAM[MAX_LANG] = {"HK1_Programm", "hc1_program"};
constexpr const char *HC1_HOLIDAY_DAYS[MAX_LANG] = {"HK1_Ferien_Tage", "hc1_holiday_days"};
constexpr const char *HC1_TIMER01[MAX_LANG] = {"HK1_Timer01", "hc1_timer01"};
constexpr const char *HC1_TIMER02[MAX_LANG] = {"HK1_Timer02", "hc1_timer02"};
constexpr const char *HC1_TIMER03[MAX_LANG] = {"HK1_Timer03", "hc1_timer03"};
constexpr const char *HC1_TIMER04[MAX_LANG] = {"HK1_Timer04", "hc1_timer04"};
constexpr const char *HC1_TIMER05[MAX_LANG] = {"HK1_Timer05", "hc1_timer05"};
constexpr const char *HC1_TIMER06[MAX_LANG] = {"HK1_Timer06", "hc1_timer06"};
constexpr const char *HC1_TIMER07[MAX_LANG] = {"HK1_Timer07", "hc1_timer07"};
constexpr const char *HC1_TIMER08[MAX_LANG] = {"HK1_Timer08", "hc1_timer08"};
constexpr const char *HC1_TIMER09[MAX_LANG] = {"HK1_Timer09", "hc1_timer09"};
constexpr const char *HC1_TIMER10[MAX_LANG] = {"HK1_Timer10", "hc1_timer10"};
constexpr const char *HC1_TIMER11[MAX_LANG] = {"HK1_Timer11", "hc1_timer11"};
constexpr const char *HC1_TIMER12[MAX_LANG] = {"HK1_Timer12", "hc1_timer12"};
constexpr const char *HC1_TIMER13[MAX_LANG] = {"HK1_Timer13", "hc1_timer13"};
constexpr const char *HC1_TIMER14[MAX_LANG] = {"HK1_Timer14", "hc1_timer14"};
constexpr const char *HC2_PROGRAM[MAX_LANG] = {"HK2_Programm", "hc2_program"};
constexpr const char *HC2_HOLIDAY_DAYS[MAX_LANG] = {"HK2_Ferien_Tage", "hc2_holiday_days"};
constexpr const char *HC2_TIMER01[MAX_LANG] = {"HK2_Timer01", "hc2_timer01"};
constexpr const char *HC2_TIMER02[MAX_LANG] = {"HK2_Timer02", "hc2_timer02"};
constexpr const char *HC2_TIMER03[MAX_LANG] = {"HK2_Timer03", "hc2_timer03"};
constexpr const char *HC2_TIMER04[MAX_LANG] = {"HK2_Timer04", "hc2_timer04"};
constexpr const char *HC2_TIMER05[MAX_LANG] = {"HK2_Timer05", "hc2_timer05"};
constexpr const char *HC2_TIMER06[MAX_LANG] = {"HK2_Timer06", "hc2_timer06"};
constexpr const char *HC2_TIMER07[MAX_LANG] = {"HK2_Timer07", "hc2_timer07"};
constexpr const char *HC2_TIMER08[MAX_LANG] = {"HK2_Timer08", "hc2_timer08"};
constexpr const char *HC2_TIMER09[MAX_LANG] = {"HK2_Timer09", "hc2_timer09"};
constexpr const char *HC2_TIMER10[MAX_LANG] = {"HK2_Timer10", "hc2_timer10"};
constexpr const char *HC2_TIMER11[MAX_LANG] = {"HK2_Timer11", "hc2_timer11"};
constexpr const char *HC2_TIMER12[MAX_LANG] = {"HK2_Timer12", "hc2_timer12"};
constexpr const char *HC2_TIMER13[MAX_LANG] = {"HK2_Timer13", "hc2_timer13"};
constexpr const char *HC2_TIMER14[MAX_LANG] = {"HK2_Timer14", "hc2_timer14"};
constexpr const char *TIME_OFFSET[MAX_LANG] = {"Uhrzeit_Offset", "time_offset"};
constexpr const char *SOLAR_OPMODE[MAX_LANG] = {"Solar_Betriebsart", "solar_operation_mode"};
constexpr const char *SOLAR_ENABLE[MAX_LANG] = {"Solar_Aktivierung", "solar_enabled"};
constexpr const char *SOLAR_TEMP_MAX[MAX_LANG] = {"Solar_Maximaltemperatur", "solar_temp_max"};
constexpr const char *SOLAR_TEMP_MIN[MAX_LANG] = {"Solar_Minimaltemperatur", "solar_temp_min"};

}; // namespace KM_CFG_TOPIC

// constexpr const char=============================================================================
// constexpr const charription for status values from KM271
// constexpr const char=============================================================================
namespace KM_STAT_TOPIC {
constexpr const char *HC1_OV1_OFFTIME_OPT[MAX_LANG] = {"HK1_BW1_Ausschaltoptimierung", "hc1_ov1_off_time_optimization"};
constexpr const char *HC1_OV1_ONTIME_OPT[MAX_LANG] = {"HK1_BW1_Einschaltoptimierung", "hc1_ov1_on_time_optimization"};
constexpr const char *HC1_OV1_AUTOMATIC[MAX_LANG] = {"HK1_BW1_Automatik", "hc1_ov1_automatic"};
constexpr const char *HC1_OV1_WW_PRIO[MAX_LANG] = {"HK1_BW1_Warmwasservorrang", "hc1_ov1_ww_priority"};
constexpr const char *HC1_OV1_SCREED_DRY[MAX_LANG] = {"HK1_BW1_Estrichtrocknung", "hc1_ov1_screed_drying"};
constexpr const char *HC1_OV1_HOLIDAY[MAX_LANG] = {"HK1_BW1_Ferien", "hc1_ov1_holiday"};
constexpr const char *HC1_OV1_FROST[MAX_LANG] = {"HK1_BW1_Frostschutz", "hc1_ov1_frost_protection"};
constexpr const char *HC1_OV1_MANUAL[MAX_LANG] = {"HK1_BW1_Manuell", "hc1_ov1_manual"};

constexpr const char *HC1_OV2_SUMMER[MAX_LANG] = {"HK1_BW2_Sommer", "hc1_ov2_summer"};
constexpr const char *HC1_OV2_DAY[MAX_LANG] = {"HK1_BW2_Tag", "hc1_ov2_day"};
constexpr const char *HC1_OV2_NO_COM_REMOTE[MAX_LANG] = {"HK1_BW2_Keine_Komm_mit_FB", "hc1_ov2_no_conn_to_remotectrl"};
constexpr const char *HC1_OV2_REMOTE_ERR[MAX_LANG] = {"HK1_BW2_FB_fehlerhaft", "hc1_ov2_remotectrl_error"};
constexpr const char *HC1_OV2_FLOW_SENS_ERR[MAX_LANG] = {"HK1_BW2_Fehler_Vorlauffuehler", "hc1_ov2_failure_flow_sensor"};
constexpr const char *HC1_OV2_FLOW_AT_MAX[MAX_LANG] = {"HK1_BW2_Maximaler_Vorlauf", "hc1_ov2_flow_at_maximum"};
constexpr const char *HC1_OV2_EXT_SENS_ERR[MAX_LANG] = {"HK1_BW2_Externer_Stoereingang", "hc1_ov2_external_signal_input"};

constexpr const char *HC1_FLOW_SETPOINT[MAX_LANG] = {"HK1_Vorlaufsolltemperatur", "hc1_flow_setpoint"};
constexpr const char *HC1_FLOW_TEMP[MAX_LANG] = {"HK1_Vorlaufisttemperatur", "hc1_flow_temp"};
constexpr const char *HC1_ROOM_SETPOINT[MAX_LANG] = {"HK1_Raumsolltemperatur", "hc1_room_setpoint"};
constexpr const char *HC1_ROOM_TEMP[MAX_LANG] = {"HK1_Raumisttemperatur", "hc1_room_temp"};
constexpr const char *HC1_ON_TIME_OPT[MAX_LANG] = {"HK1_Einschaltoptimierung", "hc1_on_time_opt_duration"};
constexpr const char *HC1_OFF_TIME_OPT[MAX_LANG] = {"HK1_Ausschaltoptimierung", "hc1_off_time_opt_duration"};
constexpr const char *HC1_PUMP[MAX_LANG] = {"HK1_Pumpe", "hc1_pump"};
constexpr const char *HC1_MIXER[MAX_LANG] = {"HK1_Mischerstellung", "hc1_mixer"};
constexpr const char *HC1_HEAT_CURVE1[MAX_LANG] = {"HK1_Heizkennlinie_10_Grad", "hc1_heat_curve_10C"};
constexpr const char *HC1_HEAT_CURVE2[MAX_LANG] = {"HK1_Heizkennlinie_0_Grad", "hc1_heat_curve_0C"};
constexpr const char *HC1_HEAT_CURVE3[MAX_LANG] = {"HK1_Heizkennlinie_-10_Grad", "hc1_heat_curve_-10C"};

constexpr const char *HC2_OV1_OFFTIME_OPT[MAX_LANG] = {"HK2_BW1_Ausschaltoptimierung", "hc2_ov1_off_time_opt"};
constexpr const char *HC2_OV1_ONTIME_OPT[MAX_LANG] = {"HK2_BW1_Einschaltoptimierung", "hc2_ov1_on_time_opt"};
constexpr const char *HC2_OV1_AUTOMATIC[MAX_LANG] = {"HK2_BW1_Automatik", "hc2_ov1_automatic"};
constexpr const char *HC2_OV1_WW_PRIO[MAX_LANG] = {"HK2_BW1_Warmwasservorrang", "hc2_ov1_ww_priority"};
constexpr const char *HC2_OV1_SCREED_DRY[MAX_LANG] = {"HK2_BW1_Estrichtrocknung", "hc2_ov1_screed_drying"};
constexpr const char *HC2_OV1_HOLIDAY[MAX_LANG] = {"HK2_BW1_Ferien", "hc2_ov1_holiday"};
constexpr const char *HC2_OV1_FROST[MAX_LANG] = {"HK2_BW1_Frostschutz", "hc2_ov1_frost_protection"};
constexpr const char *HC2_OV1_MANUAL[MAX_LANG] = {"HK2_BW1_Manuell", "hc2_ov1_manual"};

constexpr const char *HC2_OV2_SUMMER[MAX_LANG] = {"HK2_BW2_Sommer", "hc2_ov2_summer"};
constexpr const char *HC2_OV2_DAY[MAX_LANG] = {"HK2_BW2_Tag", "hc2_ov2_day"};
constexpr const char *HC2_OV2_NO_COM_REMOTE[MAX_LANG] = {"HK2_BW2_Keine_Komm_mit_FB", "hc2_ov2_no_conn_to_remotectrl"};
constexpr const char *HC2_OV2_REMOTE_ERR[MAX_LANG] = {"HK2_BW2_FB_fehlerhaft", "hc2_ov2_remotectrl_error"};
constexpr const char *HC2_OV2_FLOW_SENS_ERR[MAX_LANG] = {"HK2_BW2_Fehler_Vorlauffuehler", "hc2_ov2_failure_flow_sensor"};
constexpr const char *HC2_OV2_FLOW_AT_MAX[MAX_LANG] = {"HK2_BW2_Maximaler_Vorlauf", "hc2_ov2_flow_at_maximum"};
constexpr const char *HC2_OV2_EXT_SENS_ERR[MAX_LANG] = {"HK2_BW2_Externer_Stoereingang", "hc2_ov2_external_signal_input"};

constexpr const char *HC2_FLOW_SETPOINT[MAX_LANG] = {"HK2_Vorlaufsolltemperatur", "hc2_flow_setpoint"};
constexpr const char *HC2_FLOW_TEMP[MAX_LANG] = {"HK2_Vorlaufisttemperatur", "hc2_flow_temp"};
constexpr const char *HC2_ROOM_SETPOINT[MAX_LANG] = {"HK2_Raumsolltemperatur", "hc2_room_setpoint"};
constexpr const char *HC2_ROOM_TEMP[MAX_LANG] = {"HK2_Raumisttemperatur", "hc2_room_temp"};
constexpr const char *HC2_ON_TIME_OPT[MAX_LANG] = {"HK2_Einschaltoptimierung", "hc2_on_time_opt_duration"};
constexpr const char *HC2_OFF_TIME_OPT[MAX_LANG] = {"HK2_Ausschaltoptimierung", "hc2_off_time_opt_duration"};
constexpr const char *HC2_PUMP[MAX_LANG] = {"HK2_Pumpe", "hc2_pump"};
constexpr const char *HC2_MIXER[MAX_LANG] = {"HK2_Mischerstellung", "hc2_mixer"};
constexpr const char *HC2_HEAT_CURVE1[MAX_LANG] = {"HK2_Heizkennlinie_10_Grad", "hc2_heat_curve_10C"};
constexpr const char *HC2_HEAT_CURVE2[MAX_LANG] = {"HK2_Heizkennlinie_0_Grad", "hc2_heat_curve_0C"};
constexpr const char *HC2_HEAT_CURVE3[MAX_LANG] = {"HK2_Heizkennlinie_-10_Grad", "hc2_heat_curve_-10C"};

constexpr const char *WW_OV1_AUTO[MAX_LANG] = {"WW_BW1_Automatik", "ww_ov1_auto"};
constexpr const char *WW_OV1_DESINFECT[MAX_LANG] = {"WW_BW1_Desinfektion", "ww_ov1_disinfection"};
constexpr const char *WW_OV1_RELOAD[MAX_LANG] = {"WW_BW1_Nachladung", "ww_ov1_reload"};
constexpr const char *WW_OV1_HOLIDAY[MAX_LANG] = {"WW_BW1_Ferien", "ww_ov1_holiday"};
constexpr const char *WW_OV1_ERR_DESINFECT[MAX_LANG] = {"WW_BW1_Fehler_Desinfektion", "ww_ov1_err_disinfection"};
constexpr const char *WW_OV1_ERR_SENSOR[MAX_LANG] = {"WW_BW1_Fehler_Fuehler", "ww_ov1_err_sensor"};
constexpr const char *WW_OV1_WW_STAY_COLD[MAX_LANG] = {"WW_BW1_Fehler_WW_bleibt_kalt", "ww_ov1_ww_stays_cold"};
constexpr const char *WW_OV1_ERR_ANODE[MAX_LANG] = {"WW_BW1_Fehler_Anode", "ww_ov1_err_anode"};

constexpr const char *WW_OV2_LOAD[MAX_LANG] = {"WW_BW2_Laden", "ww_ov2_load"};
constexpr const char *WW_OV2_MANUAL[MAX_LANG] = {"WW_BW2_Manuell", "ww_ov2_manual"};
constexpr const char *WW_OV2_RELOAD[MAX_LANG] = {"WW_BW2_Nachladen", "ww_ov2_reload"};
constexpr const char *WW_OV2_OFF_TIME_OPT[MAX_LANG] = {"WW_BW2_Ausschaltoptimierung", "ww_ov2_off_time_opt_duration"};
constexpr const char *WW_OV2_ON_TIME_OPT[MAX_LANG] = {"WW_BW2_Einschaltoptimierung", "ww_ov2_on_time_opt_duration"};
constexpr const char *WW_OV2_DAY[MAX_LANG] = {"WW_BW2_Tag", "ww_ov2_day"};
constexpr const char *WW_OV2_HOT[MAX_LANG] = {"WW_BW2_Warm", "ww_ov2_hot"};
constexpr const char *WW_OV2_PRIO[MAX_LANG] = {"WW_BW2_Vorrang", "ww_ov2_priority"};

constexpr const char *WW_SETPOINT[MAX_LANG] = {"WW_Solltemperatur", "ww_setpoint"};
constexpr const char *WW_TEMP[MAX_LANG] = {"WW_Isttemperatur", "ww_temp"};
constexpr const char *WW_ONTIME_OPT[MAX_LANG] = {"WW_Einschaltoptimierung", "ww_on_time_opt_duration"};
constexpr const char *WW_PUMP_CHARGE[MAX_LANG] = {"WW_Pumpentyp_Ladepumpe", "ww_pump_type_charge"};
constexpr const char *WW_PUMP_CIRC[MAX_LANG] = {"WW_Pumpentyp_Zirkulationspumpe", "ww_pump_type_circulation"};
constexpr const char *WW_PUMP_SOLAR[MAX_LANG] = {"WW_Pumpentyp_Absenkung_Solar", "ww_pump_type_groundwater_solar"};

constexpr const char *BOILER_SETPOINT[MAX_LANG] = {"Kessel_Vorlaufsolltemperatur", "boiler_setpoint"};
constexpr const char *BOILER_TEMP[MAX_LANG] = {"Kessel_Vorlaufisttemperatur", "boiler_temp"};
constexpr const char *BOILER_ON_TEMP[MAX_LANG] = {"Brenner_Einschalttemperatur", "boiler_switch_on_temp"};
constexpr const char *BOILER_OFF_TEMP[MAX_LANG] = {"Brenner_Ausschalttemperatur", "boiler_switch_off_temp"};

constexpr const char *BOILER_ERR_BURNER[MAX_LANG] = {"Kessel_Fehler_Brennerstoerung", "boiler_failure_burner"};
constexpr const char *BOILER_ERR_SENSOR[MAX_LANG] = {"Kessel_Fehler_Kesselfuehler", "boiler_failure_boiler_sensor"};
constexpr const char *BOILER_ERR_AUX_SENS[MAX_LANG] = {"Kessel_Fehler_Zusatzfuehler", "boiler_failure_aux_sensor"};
constexpr const char *BOILER_ERR_STAY_COLD[MAX_LANG] = {"Kessel_Fehler_Kessel_bleibt_kalt", "boiler_failure_boiler_stays_cold"};
constexpr const char *BOILER_ERR_GAS_SENS[MAX_LANG] = {"Kessel_Fehler_Abgasfuehler", "boiler_failure_exhaust_gas_sensor"};
constexpr const char *BOILER_ERR_EXHAUST[MAX_LANG] = {"Kessel_Fehler_Abgas_max_Grenzwert", "boiler_failure_exhaust_gas_over_limit"};
constexpr const char *BOILER_ERR_SAFETY[MAX_LANG] = {"Kessel_Fehler_Sicherungskette", "boiler_failure_safety_chain"};
constexpr const char *BOILER_ERR_EXT[MAX_LANG] = {"Kessel_Fehler_Externe_Stoerung", "boiler_failure_external"};

constexpr const char *BOILER_STATE_GASTEST[MAX_LANG] = {"Kessel_Betrieb_Abgastest", "boiler_state_exhaust_gas_test"};
constexpr const char *BOILER_STATE_STAGE1[MAX_LANG] = {"Kessel_Betrieb_Betrieb_Stufe1", "boiler_state_stage1"};
constexpr const char *BOILER_STATE_PROTECT[MAX_LANG] = {"Kessel_Betrieb_Kesselschutz", "boiler_state_boiler_protection"};
constexpr const char *BOILER_STATE_ACTIVE[MAX_LANG] = {"Kessel_Betrieb_Unter_Betrieb", "boiler_state_active"};
constexpr const char *BOILER_STATE_PER_FREE[MAX_LANG] = {"Kessel_Betrieb_Leistung_frei", "boiler_state_performance_free"};
constexpr const char *BOILER_STATE_PER_HIGH[MAX_LANG] = {"Kessel_Betrieb_Leistung_hoch", "boiler_state_performance_high"};
constexpr const char *BOILER_STATE_STAGE2[MAX_LANG] = {"Kessel_Betrieb_BetriebStufe2", "boiler_state_stage2"};

constexpr const char *BOILER_CONTROL[MAX_LANG] = {"Brenner_Ansteuerung", "burner_control"};
constexpr const char *EXHAUST_TEMP[MAX_LANG] = {"Abgastemperatur", "exhaust_gas_temp"};
constexpr const char *BOILER_LIFETIME_1[MAX_LANG] = {"Brenner_Laufzeit_Minuten65536", "burner_runtime_minutes65536"};
constexpr const char *BOILER_LIFETIME_2[MAX_LANG] = {"Brenner_Laufzeit_Minuten256", "burner_runtime_minutes256"};
constexpr const char *BOILER_LIFETIME_3[MAX_LANG] = {"Brenner_Laufzeit_Minuten", "burner_runtime_minutes"};
constexpr const char *BOILER_LIFETIME_4[MAX_LANG] = {"Brenner_Laufzeit_Summe", "burner_runtime_overall"};
constexpr const char *BOILER_CONSUMPTION[MAX_LANG] = {"Oelverbrauch_Gesamt_berechnet", "oil_consumption_overall_calc"};

constexpr const char *OUTSIDE_TEMP[MAX_LANG] = {"Aussentemperatur", "outside_temp"};
constexpr const char *OUTSIDE_TEMP_DAMPED[MAX_LANG] = {"Aussentemperatur_gedaempft", "outside_temp_damped"};

constexpr const char *VERSION_VK[MAX_LANG] = {"Versionsnummer_VK", "version_vk"};
constexpr const char *VERSION_NK[MAX_LANG] = {"Versionsnummer_NK", "version_nk"};
constexpr const char *MODULE_ID[MAX_LANG] = {"Modulkennung", "module_id"};

constexpr const char *ALARM_EXHAUST[MAX_LANG] = {"ERR_Alarmstatus_Abgasfuehler", "err_alarm_exhaust"};
constexpr const char *ALARM_02[MAX_LANG] = {"ERR_Alarmstatus_02", "err_alarm_02"};
constexpr const char *ALARM_BOILER_FLOW[MAX_LANG] = {"ERR_Alarmstatus_Kesselvorlauffuehler", "err_alarm_boiler_flow_sensor"};
constexpr const char *ALARM_08[MAX_LANG] = {"ERR_Alarmstatus_08", "err_alarm_08"};
constexpr const char *ALARM_BURNER[MAX_LANG] = {"ERR_Alarmstatus_Brenner", "err_alarm_burner"};
constexpr const char *ALARM_20[MAX_LANG] = {"ERR_Alarmstatus_20", "err_alarm_20"};
constexpr const char *ALARM_HC2_FLOW_SENS[MAX_LANG] = {"ERR_Alarmstatus_HK2-Vorlauffuehler", "err_alarm_HK2-flow_sensor"};
constexpr const char *ALARM_80[MAX_LANG] = {"ERR_Alarmstatus_80", "err_alarm_80"};

constexpr const char *SOLAR_LOAD[MAX_LANG] = {"Solar_Ladung", "solar_load"};
constexpr const char *SOLAR_WW[MAX_LANG] = {"Solar_Warmwasser", "solar_ww"};
constexpr const char *SOLAR_COLLECTOR[MAX_LANG] = {"Solar_Kollektor", "solar_collector"};
constexpr const char *SOLAR_RUNTIME[MAX_LANG] = {"Solar_Laufzeit", "solar_runtime"};
constexpr const char *SOLAR_9147[MAX_LANG] = {"Solar_9147", "solar_9147"};

}; // namespace KM_STAT_TOPIC

// ======================================================================================
// topic decription for error messages buffer from KM271
// ======================================================================================
namespace KM_ERR_TOPIC {
constexpr const char *ERR_BUFF_1[MAX_LANG] = {"Fehlerspeicher1", "error_buffer1"};
constexpr const char *ERR_BUFF_2[MAX_LANG] = {"Fehlerspeicher2", "error_buffer2"};
constexpr const char *ERR_BUFF_3[MAX_LANG] = {"Fehlerspeicher3", "error_buffer3"};
constexpr const char *ERR_BUFF_4[MAX_LANG] = {"Fehlerspeicher4", "error_buffer4"};
}; // namespace KM_ERR_TOPIC

// ======================================================================================
// decription for encode different config value arrays
// 1. GERMAN
// 2. ENGLISH
// ======================================================================================
namespace KM_CFG_ARRAY {
constexpr const char *OPMODE[MAX_LANG][3] = {{"Nacht", "Tag", "Automatik"}, {"night", "day", "auto"}};
constexpr const char *SCREEN[MAX_LANG][4] = {{"Automatik", "Kessel", "Warmwasser", "Aussen"}, {"auto", "boiler", "DHW", "outdoor"}};
constexpr const char *LANGUAGE[MAX_LANG][6] = {{"DE", "FR", "IT", "NL", "EN", "PL"}, {"DE", "FR", "IT", "NL", "EN", "PL"}};
constexpr const char *REDUCT_MODE[MAX_LANG][4] = {{"Abschalt", "Reduziert", "Raumhalt", "Aussenhalt"}, {"off", "fixed", "room", "outdoors"}};
constexpr const char *SUMMER[MAX_LANG][23] = {
    {"Sommer", "10 °C", "11 °C", "12 °C", "13 °C", "14 °C", "15 °C", "16 °C", "17 °C", "18 °C", "19 °C", "20 °C",
     "21 °C",  "22 °C", "23 °C", "24 °C", "25 °C", "26 °C", "27 °C", "28 °C", "29 °C", "30 °C", "Winter"},
    {"summer", "10 °C", "11 °C", "12 °C", "13 °C", "14 °C", "15 °C", "16 °C", "17 °C", "18 °C", "19 °C", "20 °C",
     "21 °C",  "22 °C", "23 °C", "24 °C", "25 °C", "26 °C", "27 °C", "28 °C", "29 °C", "30 °C", "winter"}};
constexpr const char *SWITCH_ON_TEMP[MAX_LANG][11] = {
    {"Aus", "1 °C", "2 °C", "3 °C", "4 °C", "5 °C", "6 °C", "7 °C", "8 °C", "9 °C", "10 °C"},
    {"off", "1 °C", "2 °C", "3 °C", "4 °C", "5 °C", "6 °C", "7 °C", "8 °C", "9 °C", "10 °C"},
};
constexpr const char *HEATING_SYSTEM[MAX_LANG][4] = {{"Aus", "Heizkoerper", "-", "Fussboden"}, {"off", "radiator", "-", "underfloor"}};
constexpr const char *ON_OFF[2][2] = {{"Aus", "An"}, {"off", "on"}};
constexpr const char *BUILDING_TYPE[MAX_LANG][3] = {{"Leicht", "Mittel", "Schwer"}, {"light", "medium", "heavy"}};
constexpr const char *CIRC_INTERVAL[MAX_LANG][8] = {{"Aus", "1", "2", "3", "4", "5", "6", "An"}, {"off", "1", "2", "3", "4", "5", "6", "on"}};
constexpr const char *BURNER_TYPE[MAX_LANG][3] = {{"1-stufig", "2-stufig", "Modulierend"}, {"1-stage", "2-stage", "modulated"}};
constexpr const char *EXHAUST_GAS_THRESHOLD[MAX_LANG][42] = {
    {"Aus", "50",  "55",  "60",  "65",  "70",  "75",  "80",  "85",  "90",  "95",  "100", "105", "110",
     "115", "120", "125", "130", "135", "140", "145", "150", "155", "160", "165", "170", "175", "180",
     "185", "190", "195", "200", "205", "210", "215", "220", "225", "230", "235", "240", "245", "250"},
    {"off", "50",  "55",  "60",  "65",  "70",  "75",  "80",  "85",  "90",  "95",  "100", "105", "110",
     "115", "120", "125", "130", "135", "140", "145", "150", "155", "160", "165", "170", "175", "180",
     "185", "190", "195", "200", "205", "210", "215", "220", "225", "230", "235", "240", "245", "250"}};
constexpr const char *HC_PROGRAM[MAX_LANG][9] = {{"Eigen", "Familie", "Frueh", "Spaet", "Vormittag", "Nachmittag", "Mittag", "Single", "Senior"},
                                                 {"custom", "family", "early", "late", "AM", "PM", "noon", "single", "senior"}};
constexpr const char *BURNER_STATE[MAX_LANG][5] = {{"Kessel AUS", "EIN (Stufe 1)", "-", "-", "EIN (Stufe 2)"},
                                                   {"Burner OFF", "ON (Stage 1)", "-", "-", "ON (Stage 2)"}};
constexpr const char *LOG_FILTER[MAX_LANG][6] = {
    {"Modus: Alarm", "Modus: Alarm + Info", "Modus: Logamatic Werte", "Modus: unbekannte Datagramme", "Modus: debug Datagramme", "Modus: SystemLog"},
    {"Mode: Alarm", "Mode: Alarm + Info", "Mode: Logamatic values", "Mode: unknown datagramms", "Mode: debug datagramms", "Mode: SystemLog"}};
}; // namespace KM_CFG_ARRAY

// ======================================================================================
// decription for encode error messages
// ======================================================================================
struct s_err_array {
  const char *idx[2][28] = {{// GERMAN
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
                             "unbekannter Fehler"},
                            {// ENGLISH
                             "no error",
                             "failure outdoor sensor",
                             "failure hc1-flow sensor",
                             "failure hc2-flow sensor",
                             "failure warm water sensor",
                             "Warm Water remains col",
                             "Fault thermal disinfection",
                             "Fault hc1-remotecontrol",
                             "Fault hc2-remotecontrol",
                             "no communication to hc1-remote control",
                             "no communication to hc2-remote control",
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
                             "unknown error"}};
};

// ======================================================================================
// status messages from KM271
// ======================================================================================
namespace KM_INFO_MSG {
constexpr const char *CMD[MAX_LANG] = {"Befehl", "command"};
constexpr const char *RECV[MAX_LANG] = {"empfangen", "received"};
constexpr const char *INVALID[MAX_LANG] = {"ungültiger Parameter", "invalid parameter"};
constexpr const char *VALUE[MAX_LANG] = {"Wert", "value"};
constexpr const char *RESTARTED[MAX_LANG] = {"ESP hat neu gestartet!", "ESP has been restarted!"};
constexpr const char *HC1_SUMMER_MODE[MAX_LANG] = {"Heizung hat in Sommerbetrieb gewechselt ☀️", "Heating has changed to summer mode ☀️"};
constexpr const char *HC1_WINTER_MODE[MAX_LANG] = {"Heizung hat in Winterbetrieb gewechselt ❄️", "Heating has changed to winter mode ❄️"};
constexpr const char *HC2_SUMMER_MODE[MAX_LANG] = {"Heizkreis 2 hat in Sommerbetrieb gewechselt ☀️",
                                                   "Heating Circuit 2 has changed to summer mode ☀️"};
constexpr const char *HC2_WINTER_MODE[MAX_LANG] = {"Heizkreis 2 hat in Winterbetrieb gewechselt ❄️",
                                                   "Heating Circuit 2 has changed to winter mode ❄️"};
constexpr const char *HC1_FROST_MODE_ON[MAX_LANG] = {"Heizung hat in Frostmodus gewechselt ❄️", "Heating has changed to frost mode ❄️"};
constexpr const char *HC2_FROST_MODE_ON[MAX_LANG] = {"Heizkreis 2 hat in Frostmodus gewechselt ❄️",
                                                     "Heating Circuit 2 has changed to frost mode ❄️"};
constexpr const char *HC1_FROST_MODE_OFF[MAX_LANG] = {"Heizung hat den Frostmodus verlassen ❄️", "Heating has exited frost mode ❄️"};
constexpr const char *HC2_FROST_MODE_OFF[MAX_LANG] = {"Heizkreis 2 hat den Frostmodus verlassen ❄️",
                                                      "Heating Circuit 2 has exited frost mode ❄️"};
}; // namespace KM_INFO_MSG