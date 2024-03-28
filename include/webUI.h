#pragma once

// ======================================================
// includes
// ======================================================
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <config.h>

#include <gzip_c_css.h>
#include <gzip_html.h>
#include <gzip_js.h>
#include <gzip_m_css.h>

extern AsyncWebServer server;

typedef enum {
  hc1_frost_protection_threshold,
  hc1_summer_mode_threshold,
  hc2_frost_protection_threshold,
  hc2_summer_mode_threshold,
  hc1_night_temp,
  hc1_day_temp,
  hc1_operation_mode,
  hc1_holiday_temp,
  hc1_max_temp,
  hc1_interpretation,
  hc1_switch_on_temperature,
  hc1_switch_off_threshold,
  hc1_reduction_mode,
  hc1_heating_system,
  hc1_temp_offset,
  hc1_remotecontrol,
  hc2_night_temp,
  hc2_day_temp,
  hc2_operation_mode,
  hc2_holiday_temp,
  hc2_max_temp,
  hc2_interpretation,
  ww_priority,
  hc2_switch_on_temperature,
  hc2_switch_off_threshold,
  hc2_reduction_mode,
  hc2_heating_system,
  hc2_temp_offset,
  hc2_remotecontrol,
  building_type,
  ww_temp,
  ww_operation_mode,
  ww_processing,
  ww_circulation,
  language,
  display,
  burner_type,
  max_boiler_temperature,
  pump_logic_temp,
  exhaust_gas_temperature_threshold,
  burner_min_modulation,
  burner_modulation_runtime,
  hc1_program,
  hc2_program,
  time_offset,
  hc1_holiday_days,
  hc2_holiday_days,
  hc1_timer01,
  hc1_timer02,
  hc1_timer03,
  hc1_timer04,
  hc1_timer05,
  hc1_timer06,
  hc1_timer07,
  hc1_timer08,
  hc1_timer09,
  hc1_timer10,
  hc1_timer11,
  hc1_timer12,
  hc1_timer13,
  hc1_timer14,
  hc2_timer01,
  hc2_timer02,
  hc2_timer03,
  hc2_timer04,
  hc2_timer05,
  hc2_timer06,
  hc2_timer07,
  hc2_timer08,
  hc2_timer09,
  hc2_timer10,
  hc2_timer11,
  hc2_timer12,
  hc2_timer13,
  hc2_timer14,
  kmConfig_Hash_SIZE,
} s_kmConfig_Hash_enum;

// ======================================================
// Prototypes
// ======================================================
void webUISetup();
void webUICylic();
void webReadLogBuffer();