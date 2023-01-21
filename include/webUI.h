#pragma once

// ======================================================
// includes
// ======================================================
#include <config.h>
#include <Arduino.h>
#include <ESPUI.h>

#define LABLE_STYLE_GROUP           "background-color: unset; width: 100%; text-align: center;"
#define LABLE_STYLE_CLEAR           "background-color: unset; width: 60%; text-align: left;"
#define LABLE_STYLE_VALUE           "width: 30%;"
#define LABLE_STYLE_UNIT            "background-color: unset; width: 10%; text-align: left;"
#define LABLE_STYLE_DASH            "background-color: unset; width: 100%; font-size: 40px"
#define LABLE_STYLE_CLEAR_CFG       "background-color: unset; width: 50%; text-align: left;"
#define LABLE_STYLE_VALUE_CFG       "width: 50%;"
#define LABLE_STYLE_DESCRIPTION     "background-color: unset; width: 100%; text-align: center; font-weight: normal;"
#define LABLE_STYLE_CLEAR_WIDE      "background-color: unset; width: 20%; text-align: left;"
#define LABLE_STYLE_VALUE_WIDE      "width: 70%;"

#define CONFIG_TAB_REFRESH_TIME     5000

typedef struct {
uint16_t hc1pumpState;
uint16_t hc2pumpState;
uint16_t burnerState;
uint16_t hc1summerWinter;
uint16_t hc1dayNight;
uint16_t hc2summerWinter;
uint16_t hc2dayNight;
uint16_t ww_opmode;
uint16_t hc1_opmode;
uint16_t hc2_opmode;
uint16_t wwActTemp;
uint16_t wwSetTemp;
uint16_t burnerActTemp;
uint16_t burnerSetTemp;
uint16_t hc1flowActTemp;
uint16_t hc1flowSetTemp;
uint16_t hc2flowActTemp;
uint16_t hc2flowSetTemp;
uint16_t oilmeter;
} s_webui_id_dash;

typedef struct {
uint16_t hc1_ov1_automatic;    
uint16_t hc1_flow_setpoint;    
uint16_t hc1_flow_temp;        
uint16_t hc1_heatcurve1;       
uint16_t hc1_heatcurve2;       
uint16_t hc1_heatcurve3;       
uint16_t hc1_mixer;            
uint16_t hc1_off_time_opt;     
uint16_t hc1_on_time_opt;      
uint16_t hc1_frost;            
uint16_t hc1_holiday;          
uint16_t hc1_ov1_off_time_opt; 
uint16_t hc1_ov1_on_time_opt;  
uint16_t hc1_ov1_screed_dry;   
uint16_t hc1_ov1_ww_prio;      
uint16_t hc1_ov2_day;          
uint16_t hc1_ov2_ext_sens_err; 
uint16_t hc1_ov2_flow_at_max;  
uint16_t hc1_ov2_flow_sens_err;
uint16_t hc1_ov2_no_com_remote;
uint16_t hc1_ov2_remote_err;   
uint16_t hc1_ov2_summer;       
uint16_t hc1_pump;             
uint16_t hc1_room_setpoint;    
uint16_t hc1_room_temp;
uint16_t hc2_ov1_automatic;    
uint16_t hc2_flow_setpoint;    
uint16_t hc2_flow_temp;        
uint16_t hc2_heatcurve1;       
uint16_t hc2_heatcurve2;       
uint16_t hc2_heatcurve3;       
uint16_t hc2_mixer;            
uint16_t hc2_off_time_opt;     
uint16_t hc2_on_time_opt;      
uint16_t hc2_frost;            
uint16_t hc2_holiday;          
uint16_t hc2_ov1_off_time_opt; 
uint16_t hc2_ov1_on_time_opt;  
uint16_t hc2_ov1_screed_dry;   
uint16_t hc2_ov1_ww_prio;      
uint16_t hc2_ov2_day;          
uint16_t hc2_ov2_ext_sens_err; 
uint16_t hc2_ov2_flow_at_max;  
uint16_t hc2_ov2_flow_sens_err;
uint16_t hc2_ov2_no_com_remote;
uint16_t hc2_ov2_remote_err;   
uint16_t hc2_ov2_summer;       
uint16_t hc2_pump;             
uint16_t hc2_room_setpoint;    
uint16_t hc2_room_temp;
uint16_t ww_temp;             
uint16_t ww_setpoint;         
uint16_t ww_pump_solar;       
uint16_t ww_pump_circ;        
uint16_t ww_pump_charge;      
uint16_t ww_ov2_reload;       
uint16_t ww_ov2_prio;         
uint16_t ww_ov2_ontime_opt;   
uint16_t ww_ov2_offtime_opt;  
uint16_t ww_ov2_manual;       
uint16_t ww_ov2_load;         
uint16_t ww_ov2_hot;          
uint16_t ww_ov2_day;          
uint16_t ww_ov1_stay_cold;    
uint16_t ww_ov1_reload;       
uint16_t ww_ov1_holiday;      
uint16_t ww_ov1_err_sens;     
uint16_t ww_ov1_err_desinfect;
uint16_t ww_ov1_err_anode;    
uint16_t ww_ov1_desinfect;    
uint16_t ww_ov1_auto;         
uint16_t ww_ontime_opt;  
uint16_t boiler_ctrl;          
uint16_t boiler_temp;          
uint16_t boiler_setpoint;      
uint16_t boiler_err_aux_sens;  
uint16_t boiler_err_burner;    
uint16_t boiler_err_exhaust;   
uint16_t boiler_err_ext;       
uint16_t boiler_err_gas_sens;  
uint16_t boiler_err_safety;    
uint16_t boiler_err_sens;      
uint16_t boiler_err_stay_cold; 
uint16_t boiler_lifetime1;     
uint16_t boiler_lifetime2;     
uint16_t boiler_lifetime3;     
uint16_t boiler_lifetime4;     
uint16_t boiler_off_temp;      
uint16_t boiler_on_temp;       
uint16_t boiler_state_active;  
uint16_t boiler_state_gastest; 
uint16_t boiler_state_per_free;
uint16_t boiler_state_per_high;
uint16_t boiler_state_protect; 
uint16_t boiler_state_stage1;  
uint16_t boiler_state_stage2;  
uint16_t outside_temp;
uint16_t outside_temp_damped;
uint16_t exhaust_temp;
uint16_t oilconsumption;
} s_webui_id_staus;

typedef struct {
uint16_t wifiIP;
uint16_t wifiSignal;
uint16_t wifiRssi;
uint16_t sw_version;
uint16_t logamatic_version;
uint16_t logamatic_modul;
uint16_t espFreeHeap;
uint16_t espHeapSize;
uint16_t espMaxAllocHeap;
uint16_t espMinFreeHeap;
uint16_t date;
uint16_t time;
uint16_t date_input;
uint16_t time_input;
uint16_t dti_button;
uint16_t ntp_button;
} s_webui_id_system;

typedef struct {
uint16_t hc1_night_temp;
uint16_t hc1_day_temp;
uint16_t hc1_operation_mode;
uint16_t hc1_holiday_temp;
uint16_t hc1_max_temp;
uint16_t hc1_interpretation;
uint16_t hc1_switch_on_temperature;
uint16_t hc1_switch_off_threshold;
uint16_t hc1_reduction_mode;
uint16_t hc1_heating_system;
uint16_t hc1_temp_offset;
uint16_t hc1_remotecontrol;
uint16_t hc2_night_temp;
uint16_t hc2_day_temp;
uint16_t hc2_operation_mode;
uint16_t hc2_holiday_temp;
uint16_t hc2_max_temp;
uint16_t hc2_interpretation;
uint16_t hc2_switch_on_temperature;
uint16_t hc2_switch_off_threshold;
uint16_t hc2_reduction_mode;
uint16_t hc2_heating_system;
uint16_t hc2_temp_offset;
uint16_t hc2_remotecontrol;
uint16_t ww_priority;
uint16_t ww_temp;
uint16_t ww_operation_mode;
uint16_t ww_processing;
uint16_t ww_circulation;
uint16_t frost_protection_threshold;
uint16_t summer_mode_threshold;
uint16_t max_boiler_temperature;
uint16_t pump_logic_temp;
uint16_t building_type;
uint16_t burner_type;
uint16_t burner_min_modulation;
uint16_t burner_modulation_runtime;
uint16_t exhaust_gas_temperature_threshold;
uint16_t language;
uint16_t display;
uint16_t hc1_program;
uint16_t hc1_timer01;
uint16_t hc1_timer02;
uint16_t hc1_timer03;
uint16_t hc1_timer04;
uint16_t hc1_timer05;
uint16_t hc1_timer06;
uint16_t hc1_timer07;
uint16_t hc1_timer08;
uint16_t hc1_timer09;
uint16_t hc1_timer10;
uint16_t hc1_timer11;
uint16_t hc1_timer12;
uint16_t hc1_timer13;
uint16_t hc1_timer14;
uint16_t hc2_program;
uint16_t hc2_timer01;
uint16_t hc2_timer02;
uint16_t hc2_timer03;
uint16_t hc2_timer04;
uint16_t hc2_timer05;
uint16_t hc2_timer06;
uint16_t hc2_timer07;
uint16_t hc2_timer08;
uint16_t hc2_timer09;
uint16_t hc2_timer10;
uint16_t hc2_timer11;
uint16_t hc2_timer12;
uint16_t hc2_timer13;
uint16_t hc2_timer14;
uint16_t time_offset;
} s_webui_id_config;

typedef struct {
uint16_t setdatetime;
uint16_t oilcounter;
uint16_t hc1_opmode;
uint16_t hc2_opmode;
uint16_t hc1_program;
uint16_t hc2_program;
uint16_t hc1_interpretation;
uint16_t hc2_interpretation;
uint16_t hc1_switch_off_threshold;
uint16_t hc2_switch_off_threshold;
uint16_t hc1_day_setpoint;
uint16_t hc2_day_setpoint;
uint16_t hc1_night_setpoint;
uint16_t hc2_night_setpoint;
uint16_t hc1_holiday_setpoint;
uint16_t hc2_holiday_setpoint;
uint16_t ww_opmode;
uint16_t summer_mode_threshold;
uint16_t frost_mode_threshold;
uint16_t ww_setpoint;
uint16_t hc1_holidays;
uint16_t hc2_holidays;
uint16_t oilmeter_input;
uint16_t oilmeter_button;
uint16_t oilmeter_output;
} s_webui_id_control;

typedef struct {
uint16_t alarm1;
uint16_t alarm2;
uint16_t alarm3;
uint16_t alarm4;
} s_webui_id_alarm;

typedef struct {
uint16_t dashboard;
uint16_t control;
uint16_t hc1;
uint16_t hc2;
uint16_t ww;
uint16_t boiler;
uint16_t general;
uint16_t system;
uint16_t alarm;
} s_webui_id_tab;

typedef struct {
s_webui_id_dash dash;
s_webui_id_control ctrl;
s_webui_id_staus stat;
s_webui_id_system sys;
s_webui_id_config cfg;
s_webui_id_alarm alarm;
s_webui_id_tab tab;
} s_webui_id;

// ======================================================
// Prototypes
// ======================================================
void webUISetup();
void webUICylic();



