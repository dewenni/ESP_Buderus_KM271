#pragma once

/* I N C L U D E S ****************************************************/
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <config.h>
#include <gzip_c_css.h>
#include <gzip_js.h>
#include <gzip_login_html.h>
#include <gzip_m_css.h>
#include <gzip_main_html.h>
#include <language.h>

/* D E C L A R A T I O N S ****************************************************/
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

extern s_webui_texts webText;
extern s_cfg_arrays cfgArrayTexts;

/* P R O T O T Y P E S ********************************************************/
void webUISetup();
void webUICylic();
void webReadLogBuffer();

void sendWebUpdate(const char *message, const char *event);
void setLanguage(const char *language);
void updateWebText(const char *elementID, const char *text, bool isInput);
void updateWebTextInt(const char *elementID, long value, bool isInput);
void updateWebTextFloat(const char *elementID, double value, bool isInput, int decimals);
void updateWebState(const char *elementID, bool state);
void updateWebValueStr(const char *elementID, const char *value);
void updateWebValueInt(const char *elementID, long value);
void updateWebValueFloat(const char *elementID, double value, int decimals);
void showElementClass(const char *className, bool show);
void hideElementId(const char *elementID, bool hide);
void updateWebDialog(const char *elementID, const char *state);
void updateWebSetIcon(const char *elementID, const char *icon);
void updateWebJSON(const char *JSON);
void updateWebHref(const char *elementID, const char *href);
void updateWebBusy(const char *elementID, bool busy);