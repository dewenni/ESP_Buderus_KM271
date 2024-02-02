#pragma once
#include <config.h>

#define MAX_LANG 2      // increase if you add more languages


// ======================================================================================
// mqtt messages : texts that are used for mqtt messages from KM271
// ======================================================================================
typedef struct {
const char* SYSTEM[MAX_LANG] =                   {"System",                                                     "System"};
const char* CONTROL[MAX_LANG] =                  {"Bedienung",                                                  "Control"};
const char* DASHBORAD[MAX_LANG] =                {"Dashboard",                                                  "Dashboard"};
const char* BOILER[MAX_LANG] =                   {"Kessel",                                                     "Boiler"};
const char* AUTOMATIC[MAX_LANG] =                {"Automatik",                                                  "Automatic"};  
const char* OPMODES[MAX_LANG] =                  {"Betriebsarten",                                              "Operation Modes"};
const char* PROGRAMS[MAX_LANG] =                 {"Programme",                                                  "Programs"};
const char* CONFIG[MAX_LANG] =                   {"Konfiguration",                                              "Config"};
const char* STATUS[MAX_LANG] =                   {"Status",                                                     "Status"};
const char* SETTINGS[MAX_LANG] =                 {"Einstellungen",                                              "Settings"};
const char* OP_VALUES[MAX_LANG] =                {"Betriebswerte",                                              "Operating-States"};
const char* TEMPERATURES[MAX_LANG] =             {"Temperaturen",                                               "Temperatures"};      
const char* MANUAL[MAX_LANG] =                   {"Handbetrieb",                                                "Manual"}; 
const char* DAY[MAX_LANG] =                      {"Tag",                                                        "Day"};    
const char* NIGHT[MAX_LANG] =                    {"Nacht",                                                      "Night"};
const char* HC1_DAY_NIGHT[MAX_LANG] =            {"HK1 Tag/Nacht",                                              "HC1 Day/Night"};
const char* HC2_DAY_NIGHT[MAX_LANG] =            {"HK2 Tag/Nacht",                                              "HC2 Day/Night"};
const char* SUMMER[MAX_LANG] =                   {"Sommer",                                                     "Summer"};    
const char* WINTER[MAX_LANG] =                   {"Winter",                                                     "Winter"};
const char* HC1_SUMMER_WINTER[MAX_LANG] =        {"HK1 Sommer/Winter",                                          "HC1 Summer/Winter"};
const char* HC2_SUMMER_WINTER[MAX_LANG] =        {"HK2 Sommer/Winter",                                          "HC2 Summer/Winter"};
const char* SETPOINT[MAX_LANG] =                 {"Sollwert",                                                   "Setpoint"};    
const char* ACT_VALUE[MAX_LANG] =                {"Istwert",                                                    "Actual Value"};    
const char* SET_TEMP[MAX_LANG] =                 {"Solltemperatur",                                             "Set Temperature"};
const char* SET_TEMP_C[MAX_LANG] =               {"Solltemperatur °C",                                          "Set Temperature °C"};
const char* ACT_TEMP[MAX_LANG] =                 {"Isttemperatur",                                              "Actual Temperature"}; 
const char* ACT_TEMP_C[MAX_LANG] =               {"Isttemperatur °C",                                           "Actual Temperature °C"}; 
const char* HC1_OPMODE[MAX_LANG] =               {"HK1-Betriebsart",                                            "HC1 Operation Mode"}; 
const char* HC2_OPMODE[MAX_LANG] =               {"HK2-Betriebsart",                                            "HC2 Operation Mode"}; 
const char* WW_OPMODE[MAX_LANG] =                {"WW-Betriebsart",                                             "WW Operation Mode"}; 
const char* ON[MAX_LANG] =                       {"EIN",                                                        "ON"}; 
const char* OFF[MAX_LANG] =                      {"AUS",                                                        "OFF"};
const char* OK[MAX_LANG] =                       {"OK",                                                         "OK"};
const char* ERROR[MAX_LANG] =                    {"FEHLER",                                                     "ERROR"};
const char* ERROR_FLAGS[MAX_LANG] =              {"Fehlermeldungen",                                            "Error Flags"};
const char* BURNER[MAX_LANG] =                   {"Brenner",                                                    "Burner"}; 
const char* HC1_PUMP[MAX_LANG] =                 {"HK1-Umwälzpumpe",                                            "HC1 Flow Pump"};
const char* HC2_PUMP[MAX_LANG] =                 {"HK2-Umwälzpumpe",                                            "HC2 Flow Pump"}; 
const char* WW[MAX_LANG] =                       {"Warmwasser",                                                 "Warm Water"}; 
const char* BURNER_TEMP[MAX_LANG] =              {"Kessel-Vorlauf",                                             "Boiler Temperatures"};
const char* HC1_FLOW[MAX_LANG] =                 {"HK1-Vorlauf",                                                "HC1 Flow Temperatures"};
const char* HC2_FLOW[MAX_LANG] =                 {"HK2-Vorlauf",                                                "HC2 Flow Temperatures"};
const char* WIFI_INFO[MAX_LANG] =                {"WiFi-Information",                                           "WiFi-Informations"};
const char* WIFI_IP[MAX_LANG] =                  {"IP-Adresse",                                                 "IP-Address"};
const char* HC1[MAX_LANG] =                      {"Heizkreis 1",                                                "Heating Circuit 1"};
const char* HC2[MAX_LANG] =                      {"Heizkreis 2",                                                "Heating Circuit 2"};
const char* HC[MAX_LANG] =                       {"Heizkreise",                                                 "Heating Circuits"};
const char* GENERAL[MAX_LANG] =                  {"Allgemeine Werte",                                           "General Values"};
const char* SW_VERSION[MAX_LANG] =               {"Software-Version",                                           "Software-Version"};
const char* LOGAMATIC_VERSION[MAX_LANG] =        {"Logamatic-Version",                                          "Logamatic-Version"};
const char* LOGAMATIC_MODUL[MAX_LANG] =          {"Logamatic-Modul",                                            "Logamatic-Module"};
const char* VERSION_INFO[MAX_LANG] =             {"Versionsinformationen",                                      "Version Informations"};
const char* HC1_PRG[MAX_LANG] =                  {"HK1-Programm",                                               "HC1 Program"}; 
const char* HC2_PRG[MAX_LANG] =                  {"HK2-Programm",                                               "HC2 Program"};
const char* INFO_SUMMER1[MAX_LANG] =             {"Umschalttemperatur zwischen Sommer / Winter",                "Threshold to switch between Summer/Winter"};
const char* INFO_SUMMER2[MAX_LANG] =             {"9:Sommer | 10..30:Schwelle (°C) | 31:Winter",                "9:Summer | 10..30:Threshold (°C) | 31:Winter"};
const char* INFO_FROST[MAX_LANG] =               {"Umschalttemperatur Frostschutz",                             "Threshold for Frostprotection"};
const char* INFO_DESIGNTEMP[MAX_LANG] =          {"Auslegungstemperatur Heizkennlinie",                         "Design Temperature for heating curve"};
const char* INFO_SWITCHOFF[MAX_LANG] =           {"Umschaltschwelle für Absenkung Aussenhalt",                  "Threshold for reduction mode"};
const char* INFO_WWTEMP[MAX_LANG] =              {"Solltemperatur für Warmwasser",                              "Setpoint for Hot-Water"};
const char* INFO_UNIT_C[MAX_LANG] =              {"Einheit: °C",                                                "Unit: °C"};
const char* INFO_WW_PUMP_CIRC1[MAX_LANG] =       {"Anzahl der Zyklen pro Stunde",                               "count of operation cycles per hour"};
const char* INFO_WW_PUMP_CIRC2[MAX_LANG] =       {"0:AUS | 1..6: Zyklen/Stunde | 7:EIN",                        "0:OFF | 1..6: cycles/hour | 7:ON"};
const char* OILMETER[MAX_LANG] =                 {"Ölzähler",                                                   "Oil-Meter"};
const char* INFO_UNIT_L[MAX_LANG] =              {"Einheit: Liter",                                             "Unit: Litre"};
const char* OILMETER_ACT[MAX_LANG] =             {"Ölzählerstand",                                              "Oil-Meter value"};
const char* BUTTON_SET[MAX_LANG] =               {"setzen",                                                     "set"};
const char* BUTTON_NTP[MAX_LANG] =               {"setzen NTP",                                                 "set NTP"};
const char* BUTTON_DTI[MAX_LANG] =               {"setzen manuell",                                             "set manually"};
const char* VOLTAGE[MAX_LANG] =                  {"Spannung",                                                   "Voltage"};
const char* ESP_HEAPSIZE[MAX_LANG] =             {"ESP HeapSize",                                               "ESP HeapSize"};
const char* ESP_FREEHEAP[MAX_LANG] =             {"ESP FreeHeap",                                               "ESP FreeHeap"};
const char* ESP_MAXALLOCHEAP[MAX_LANG] =         {"ESP MaxAllocHeap",                                           "ESP MaxAllocHeap"};
const char* ESP_MINFREEHEAP[MAX_LANG] =          {"ESP MinFreeHeap",                                            "ESP MinFreeHeap"};
const char* ESP_FLASH_USAGE[MAX_LANG] =          {"ESP Flash usage",                                            "ESP Flash usage"};
const char* ESP_HEAP_USAGE[MAX_LANG] =           {"ESP Heap usage",                                             "ESP Heap usage"};
const char* SYSINFO[MAX_LANG] =                  {"Systeminformationen",                                        "System Informations"};
const char* ALARM[MAX_LANG] =                    {"Alarme",                                                     "Alarms"};
const char* ALARMINFO[MAX_LANG] =                {"letzte Alarm Meldungen",                                     "latest Alarm Messages"};
const char* MESSAGE[MAX_LANG] =                  {"Meldung",                                                    "Message"};
const char* ESP_INFO[MAX_LANG] =                 {"ESP-Info",                                                   "ESP-Info"};
const char* OPERATION[MAX_LANG] =                {"Betrieb",                                                    "Operation"};
const char* LIFETIMES[MAX_LANG] =                {"Laufzeiten",                                                 "Runtimes"};
const char* LIMITS[MAX_LANG] =                   {"Grenzwerte",                                                 "Limits"};
const char* DATETIME[MAX_LANG] =                 {"Datum und Uhrzeit",                                          "Date and Time"};
const char* DATE[MAX_LANG] =                     {"Datum",                                                      "Date"};
const char* TIME[MAX_LANG] =                     {"Uhrzeit",                                                    "Time"};
const char* LOGAMATIC[MAX_LANG] =                {"Logamatic",                                                  "Logamatic"};
const char* WIFI[MAX_LANG] =                     {"WiFi",                                                       "WiFi"};
const char* SSID[MAX_LANG] =                     {"SSID",                                                       "SSID"};
const char* PASSWORD[MAX_LANG] =                 {"Passwort",                                                   "Password"};
const char* USER[MAX_LANG] =                     {"Benutzer",                                                   "User"};
const char* HOSTNAME[MAX_LANG] =                 {"Hostname",                                                   "Hostname"};
const char* SERVER[MAX_LANG] =                   {"Server",                                                     "Server"};
const char* TOPIC[MAX_LANG] =                    {"Topic",                                                      "Topic"};
const char* MQTT[MAX_LANG] =                     {"MQTT",                                                       "MQTT"};
const char* PORT[MAX_LANG] =                     {"Port",                                                       "Port"};
const char* NTP[MAX_LANG] =                      {"NTP-Server",                                                 "NTP-Server"};
const char* NTP_TZ[MAX_LANG] =                   {"Time-Zone",                                                  "Time-Zone"};
const char* SPRACHE[MAX_LANG] =                  {"Sprache",                                                    "Language"};
const char* GPIO[MAX_LANG] =                     {"GPIO-Zuweisung",                                             "GPIO-Settings"};
const char* LED_WIFI[MAX_LANG] =                 {"LED-WiFi",                                                   "LED-WiFi"};
const char* LED_HEARTBEAT[MAX_LANG] =            {"LED-Heartbeat",                                              "LED-Heartbeat"};
const char* LED_LOGMODE[MAX_LANG] =              {"LED-Logmode",                                                "LED-Logmode"};
const char* LED_OILCOUNTER[MAX_LANG] =           {"LED-Ölzähler",                                               "LED-Oilcounter"};
const char* TRIG_OILCOUNTER[MAX_LANG] =          {"Impuls-Ölzähler",                                            "Trigger-Oilcounter"};
const char* KM271_TX[MAX_LANG] =                 {"KM271-TX",                                                   "KM271-TX"};
const char* KM271_RX[MAX_LANG] =                 {"KM271-RX",                                                   "KM271-RX"};
const char* WEBUI[MAX_LANG] =                    {"Webserver",                                                  "Webserver"};
const char* SAVE_RESTART[MAX_LANG] =             {"Speichern und Neustart",                                     "save and restart"};
const char* OIL_HARDWARE[MAX_LANG] =             {"Ölzähler Hardware",                                          "Oilmeter Hardware"};
const char* OIL_VIRTUAL[MAX_LANG] =              {"Ölzähler virtuell",                                          "Oilmeter virtual"};
const char* OIL_PAR1_KG_H[MAX_LANG] =            {"Verbrauch Kg/h",                                             "consumption Kg/h"};
const char* OIL_PAR2_KG_L[MAX_LANG] =            {"Öl-Dichte Kg/L",                                             "oil density Kg/L"};
const char* WIFI_AP_INFO_1[MAX_LANG] =           {"\n⚠️ um den ESP in den Accesspoint Mode zu versetzen ⚠️",      "\n⚠️ to set the ESP in Accesspoint Mode ⚠️"};
const char* WIFI_AP_INFO_2[MAX_LANG] =           {"Reset drücken und nach 5s erneut Reset drücken",             "press reset and after 5s press reset again"};
const char* LANGUAGE[MAX_LANG] =                 {"Sprache",                                                    "Language"};
const char* PREDEFINE[MAX_LANG] =                {"Voreinstellung",                                             "Predefine"};
const char* GPIO_UNUSED[MAX_LANG] =              {"⚠️ \"-1\" = unbenutzt ⚠️",                                     "⚠️ \"-1\" = unused ⚠️"};
const char* OTA[MAX_LANG] =                      {"OTA Firmware Update",                                        "OTA Firmware Update"};
const char* FILEMGN[MAX_LANG] =                  {"Dateimanager",                                               "Filemanager"};
const char* TOOLS[MAX_LANG] =                    {"Tools",                                                      "Tools"};
const char* TEMP_OUT[MAX_LANG] =                 {"Außentemperatur",                                            "Temperature outdoor"};
const char* TEMP_OUT_ACT[MAX_LANG] =             {"aktuell °C",                                                 "actually °C"};
const char* TEMP_OUT_DMP[MAX_LANG] =             {"gedämpft °C",                                                "damped °C"};
const char* MQTT_CFG_RET[MAX_LANG] =             {"Config Nachrichten als retain",                              "config mssages as retain"};
const char* ACTIVATE[MAX_LANG] =                 {"Aktivieren",                                                 "activate"};
const char* MAN_IP_SETTINGS[MAX_LANG] =          {"manuelle IP-Einstellungen",                                  "manual IP settings"};
const char* IP_ADR[MAX_LANG] =                   {"IP-Adresse",                                                 "IP-Address"};
const char* IP_SUBNET[MAX_LANG] =                {"Subnetz",                                                    "Subnet"};
const char* IP_GATEWAY[MAX_LANG] =               {"Gateway",                                                    "Gateway"};
const char* IP_DNS[MAX_LANG] =                   {"DNS-Server",                                                 "DNS-Server"};
const char* ACCESSS[MAX_LANG] =                  {"Zugangskontrolle",                                           "Auhtentification"};
} s_webui_texts; 


// ======================================================================================
// mqtt commands : texts that are used as topics for KM271 commands
// ======================================================================================
typedef struct {
const char* RESTART[MAX_LANG] =                     {"/cmd/restart",                        "/cmd/restart"};
const char* SERVICE[MAX_LANG] =                     {"/cmd/service",                        "/cmd/service"};
const char* DATETIME[MAX_LANG] =                    {"/setvalue/setdatetime",               "/setvalue/setdatetime"};
const char* OILCNT[MAX_LANG] =                      {"/setvalue/oilcounter",                "/setvalue/oilcounter"};
const char* HC1_OPMODE[MAX_LANG] =                  {"/setvalue/hk1_betriebsart",           "/setvalue/hc1_opmode"};
const char* HC2_OPMODE[MAX_LANG] =                  {"/setvalue/hk2_betriebsart",           "/setvalue/hc2_opmode"};
const char* HC1_PRG[MAX_LANG] =                     {"/setvalue/hk1_programm",              "/setvalue/hc1_program"};
const char* HC2_PRG[MAX_LANG] =                     {"/setvalue/hk2_programm",              "/setvalue/hc2_program"};
const char* HC1_INTERPRET[MAX_LANG] =               {"/setvalue/hk1_auslegung",             "/setvalue/hc1_interpretation"};
const char* HC2_INTERPRET[MAX_LANG] =               {"/setvalue/hk2_auslegung",             "/setvalue/hc2_interpretation"};
const char* HC1_SWITCH_OFF_THRESHOLD[MAX_LANG] =    {"/setvalue/hk1_aussenhalt_ab",         "/setvalue/hc1_switch_off_threshold"};
const char* HC2_SWITCH_OFF_THRESHOLD[MAX_LANG] =    {"/setvalue/hk2_aussenhalt_ab",         "/setvalue/hc2_switch_off_threshold"};
const char* HC1_DAY_SETPOINT[MAX_LANG] =            {"/setvalue/hk1_tag_soll",              "/setvalue/hc1_day_setpoint"};
const char* HC2_DAY_SETPOINT[MAX_LANG] =            {"/setvalue/hk2_tag_soll",              "/setvalue/hc2_day_setpoint"};
const char* HC1_NIGHT_SETPOINT[MAX_LANG] =          {"/setvalue/hk1_nacht_soll",            "/setvalue/hc1_night_setpoint"};
const char* HC2_NIGHT_SETPOINT[MAX_LANG] =          {"/setvalue/hk2_nacht_soll",            "/setvalue/hc2_night_setpoint"};
const char* HC1_HOLIDAY_SETPOINT[MAX_LANG] =        {"/setvalue/hk1_ferien_soll",           "/setvalue/hc1_holiday_setpoint"};
const char* HC2_HOLIDAY_SETPOINT[MAX_LANG] =        {"/setvalue/hk2_ferien_soll",           "/setvalue/hc2_holiday_setpoint"};
const char* WW_OPMODE[MAX_LANG] =                   {"/setvalue/ww_betriebsart",            "/setvalue/ww_opmode"};
const char* HC1_SUMMER[MAX_LANG] =                  {"/setvalue/hk1_sommer_ab",             "/setvalue/hc1_summer_mode_threshold"};
const char* HC1_FROST[MAX_LANG] =                   {"/setvalue/hk1_frost_ab",              "/setvalue/hc1_frost_mode_threshold"};
const char* HC2_SUMMER[MAX_LANG] =                  {"/setvalue/hk2_sommer_ab",             "/setvalue/hc2_summer_mode_threshold"};
const char* HC2_FROST[MAX_LANG] =                   {"/setvalue/hk2_frost_ab",              "/setvalue/hc2_frost_mode_threshold"};
const char* WW_SETPOINT[MAX_LANG] =                 {"/setvalue/ww_soll",                   "/setvalue/ww_setpoint"};
const char* HC1_HOLIDAYS[MAX_LANG] =                {"/setvalue/hk1_ferien_tage",           "/setvalue/hc1_holidays"};
const char* HC2_HOLIDAYS[MAX_LANG] =                {"/setvalue/hk2_ferien_tage",           "/setvalue/hc2_holidays"};
const char* WW_PUMP_CYCLES[MAX_LANG] =              {"/setvalue/ww_pumpen_zyklus",          "/setvalue/ww_pump_cycles"};
const char* HC1_SWITCH_ON_TEMP[MAX_LANG] =          {"/setvalue/hk1_aufschalttemperatur",   "/setvalue/hc1_switch_on_temperature"};
const char* HC2_SWITCH_ON_TEMP[MAX_LANG] =          {"/setvalue/hk2_aufschalttemperatur",   "/setvalue/hc2_switch_on_temperature"};
const char* HC1_REDUCTION_MODE[MAX_LANG] =          {"/setvalue/hk1_absenkungsart",         "/setvalue/hc1_reduction_mode"};
const char* HC2_REDUCTION_MODE[MAX_LANG] =          {"/setvalue/hk2_absenkungsart",         "/setvalue/hc2_reduction_mode"};
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
const char* HOURS[MAX_LANG]             =    {"Stunden",                         "hours"};

const char* HC1_OPMODE_RECV[MAX_LANG]                   =    {"setvalue: hk1_betriebsart - empfangen",          "setvalue: hc1_opmode - received"};
const char* HC1_OPMODE_INVALID[MAX_LANG]                =    {"setvalue: hk1_betriebsart - ungültig",           "setvalue: hc1_opmode - invalid"};
const char* HC2_OPMODE_RECV[MAX_LANG]                   =    {"setvalue: hk2_betriebsart - empfangen",          "setvalue: hc2_opmode - received"};
const char* HC2_OPMODE_INVALID[MAX_LANG]                =    {"setvalue: hk2_betriebsart - ungültig",           "setvalue: hc2_opmode - invalid"};
const char* HC1_INTERPRET_RECV[MAX_LANG]                =    {"setvalue: hk1_auslegung - empfangen",            "setvalue: hc1_interpretation - received"};
const char* HC1_INTERPRET_INVALID[MAX_LANG]             =    {"setvalue: hk1_auslegung - ungültig",             "setvalue: hc1_interpretation - invalid"};
const char* HC2_INTERPRET_RECV[MAX_LANG]                =    {"setvalue: hk2_auslegung - empfangen",            "setvalue: hc2_interpretation - received"};
const char* HC2_INTERPRET_INVALID[MAX_LANG]             =    {"setvalue: hk2_auslegung - ungültig",             "setvalue: hc2_interpretation - invalid"};
const char* HC1_PROG_RECV[MAX_LANG]                     =    {"setvalue: hk1_programm - empfangen",             "setvalue: hc1_program - received"};
const char* HC1_PROG_INVALID[MAX_LANG]                  =    {"setvalue: hk1_programm - ungültig",              "setvalue: hc1_program - invalid"};
const char* HC2_PROG_RECV[MAX_LANG]                     =    {"setvalue: hk2_programm - empfangen",             "setvalue: hc2_program - received"};
const char* HC2_PROG_INVALID[MAX_LANG]                  =    {"setvalue: hk2_programm - ungültig",              "setvalue: hc2_program - invalid"};
const char* WW_OPMODE_RECV[MAX_LANG]                    =    {"setvalue: ww_betriebsart - empfangen",           "setvalue: ww_opmode - received"};
const char* WW_OPMODE_INVALID[MAX_LANG]                 =    {"setvalue: ww_betriebsart - ungültig",            "setvalue: ww_opmode - invalid"};
const char* HC1_SUMMER_RECV[MAX_LANG]                   =    {"setvalue: hk1_sommer_ab - empfangen",            "setvalue: hc1_summer_mode_threshold - received"};
const char* HC1_SUMMER_INVALID[MAX_LANG]                =    {"setvalue: hk1_sommer_ab - ungültig",             "setvalue: hc1_summer_mode_threshold - invalid"};
const char* HC1_FROST_RECV[MAX_LANG]                    =    {"setvalue: hk1_frost_ab - empfangen",             "setvalue: hc1_frost_mode_threshold - received"};
const char* HC1_FROST_INVALID[MAX_LANG]                 =    {"setvalue: hk1_frost_ab - ungültig",              "setvalue: hc1_frost_mode_threshold - invalid"};
const char* HC2_SUMMER_RECV[MAX_LANG]                   =    {"setvalue: hk2_sommer_ab - empfangen",            "setvalue: hc2_summer_mode_threshold - received"};
const char* HC2_SUMMER_INVALID[MAX_LANG]                =    {"setvalue: hk2_sommer_ab - ungültig",             "setvalue: hc2_summer_mode_threshold - invalid"};
const char* HC2_FROST_RECV[MAX_LANG]                    =    {"setvalue: hk2_frost_ab - empfangen",             "setvalue: hc2_frost_mode_threshold - received"};
const char* HC2_FROST_INVALID[MAX_LANG]                 =    {"setvalue: hk2_frost_ab - ungültig",              "setvalue: hc2_frost_mode_threshold - invalid"};
const char* HC1_SWITCH_OFF_THRESHOLD_RECV[MAX_LANG]     =    {"setvalue: hk1_aussenhalt_ab - empfangen",        "setvalue: hc1_switch_off_threshold - received"};
const char* HC1_SWITCH_OFF_THRESHOLD_INVALID[MAX_LANG]  =    {"setvalue: hk1_aussenhalt_ab - ungültig",         "setvalue: hc1_switch_off_threshold - invalid"};
const char* HC2_SWITCH_OFF_THRESHOLD_RECV[MAX_LANG]     =    {"setvalue: hk2_aussenhalt_ab - empfangen",        "setvalue: hc2_switch_off_threshold - received"};
const char* HC2_SWITCH_OFF_THRESHOLD_INVALID[MAX_LANG]  =    {"setvalue: hk2_aussenhalt_ab - ungültig",         "setvalue: hc2_switch_off_threshold - invalid"};
const char* WW_SETPOINT_RECV[MAX_LANG]                  =    {"setvalue: ww_soll - empfangen",                  "setvalue: ww_setpoint - received"};
const char* WW_SETPOINT_INVALID[MAX_LANG]               =    {"setvalue: ww_soll - ungültig",                   "setvalue: ww_setpoint - invalid"};
const char* HC1_DAY_SETPOINT_RECV[MAX_LANG]             =    {"setvalue: hk1_tag_soll - empfangen",             "setvalue: hc1_day_setpoint - received"};
const char* HC1_DAY_SETPOINT_INVALID[MAX_LANG]          =    {"setvalue: hk1_tag_soll - ungültig",              "setvalue: hc1_day_setpoint - invalid"};
const char* HC1_NIGHT_SETPOINT_RECV[MAX_LANG]           =    {"setvalue: hk1_nacht_soll - empfangen",           "setvalue: hc1_night_setpoint - received"};
const char* HC1_NIGHT_SETPOINT_INVALID[MAX_LANG]        =    {"setvalue: hk1_nacht_soll - ungültig",            "setvalue: hc1_night_setpoint - invalid"};
const char* HC2_DAY_SETPOINT_RECV[MAX_LANG]             =    {"setvalue: hk2_tag_soll - empfangen",             "setvalue: hc2_day_setpoint - received"};
const char* HC2_DAY_SETPOINT_INVALID[MAX_LANG]          =    {"setvalue: hk2_tag_soll - ungültig",              "setvalue: hc2_day_setpoint - invalid"};
const char* HC2_NIGHT_SETPOINT_RECV[MAX_LANG]           =    {"setvalue: hk2_nacht_soll - empfangen",           "setvalue: hc2_night_setpoint - received"};
const char* HC2_NIGHT_SETPOINT_INVALID[MAX_LANG]        =    {"setvalue: hk2_nacht_soll - ungültig",            "setvalue: hc2_night_setpoint - invalid"};
const char* HC1_HOLIDAY_SETPOINT_RECV[MAX_LANG]         =    {"setvalue: hk1_ferien_soll - empfangen",          "setvalue: hc1_holiday_setpoint - received"};
const char* HC1_HOLIDAY_SETPOINT_INVALID[MAX_LANG]      =    {"setvalue: hk1_ferien_soll - ungültig",           "setvalue: hc1_holiday_setpoint - invalid"};
const char* HC2_HOLIDAY_SETPOINT_RECV[MAX_LANG]         =    {"setvalue: hk2_ferien_soll - empfangen",          "setvalue: hc2_holiday_setpoint - received"};
const char* HC2_HOLIDAY_SETPOINT_INVALID[MAX_LANG]      =    {"setvalue: hk2_ferien_soll - ungültig",           "setvalue: hc2_holiday_setpoint - invalid"};
const char* HC1_HOLIDAYS_RECV[MAX_LANG]                 =    {"setvalue: hk1_ferien_tage - empfangen",          "setvalue: hc1_holidays - received"};
const char* HC1_HOLIDAYS_INVALID[MAX_LANG]              =    {"setvalue: hk1_ferien_tage - ungültig",           "setvalue: hc1_holidays - invalid"};
const char* HC2_HOLIDAYS_RECV[MAX_LANG]                 =    {"setvalue: hk2_ferien_tage - empfangen",          "setvalue: hc2_holidays - received"};
const char* HC2_HOLIDAYS_INVALID[MAX_LANG]              =    {"setvalue: hk2_ferien_tage - ungültig",           "setvalue: hc2_holidays - invalid"};
const char* WW_PUMP_CYCLE_RECV[MAX_LANG]                =    {"setvalue: ww_pumpen_zyklus - empfangen",         "setvalue: ww_pump_cycles - received"};
const char* WW_PUMP_CYCLE_INVALID[MAX_LANG]             =    {"setvalue: ww_pumpen_zyklus - ungültig",          "setvalue: ww_pump_cycles - invalid"};
const char* HC1_SWITCH_ON_TEMP_RECV[MAX_LANG]           =    {"setvalue: hk1_aufschalttemperatur - empfangen",  "setvalue: hc1_switch_on_temperature - received"};
const char* HC2_SWITCH_ON_TEMP_RECV[MAX_LANG]           =    {"setvalue: hk2_aufschalttemperatur - empfangen",  "setvalue: hc2_switch_on_temperature - received"};
const char* HC1_SWITCH_ON_TEMP_INVALID[MAX_LANG]        =    {"setvalue: hk1_aufschalttemperatur - ungültig",   "setvalue: hc1_switch_on_temperature - invalid"};
const char* HC2_SWITCH_ON_TEMP_INVALID[MAX_LANG]        =    {"setvalue: hk2_aufschalttemperatur - ungültig",   "setvalue: hc2_switch_on_temperature - invalid"};
const char* HC1_REDUCTION_MODE_RECV[MAX_LANG]           =    {"setvalue: hk1_absenkungsart - empfangen",        "setvalue: hc1_reduction-mode - empfangen"};
const char* HC1_REDUCTION_MODE_INVALID[MAX_LANG]        =    {"setvalue: hk1_absenkungsart - ungültig",         "setvalue: hc1_reduction-mode - invalid"};
const char* HC2_REDUCTION_MODE_RECV[MAX_LANG]           =    {"setvalue: hk2_absenkungsart - empfangen",        "setvalue: hc2_reduction-mode - empfangen"};
const char* HC2_REDUCTION_MODE_INVALID[MAX_LANG]        =    {"setvalue: hk2_absenkungsart - ungültig",         "setvalue: hc2_reduction-mode - invalid"};
} s_mqtt_messags;   


// ======================================================================================
// topic decription for config values from KM271
// ======================================================================================
typedef struct {
const char* HC1_FROST_THRESHOLD[MAX_LANG]       =    {"HK1_Frost_ab",                "hc1_frost_protection_threshold"};
const char* HC1_SUMMER_THRESHOLD[MAX_LANG]      =    {"HK1_Sommer_ab",               "hc1_summer_mode_threshold"};
const char* HC2_FROST_THRESHOLD[MAX_LANG]       =    {"HK2_Frost_ab",                "hc2_frost_protection_threshold"};
const char* HC2_SUMMER_THRESHOLD[MAX_LANG]      =    {"HK2_Sommer_ab",               "hc2_summer_mode_threshold"};
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
const char* HC1_HOLIDAY_DAYS[MAX_LANG]          =    {"HK1_Ferien_Tage",             "hc1_holiday_days"};
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
const char* HC2_HOLIDAY_DAYS[MAX_LANG]          =    {"HK2_Ferien_Tage",             "hc2_holiday_days"};
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
// topic decription for status values from KM271
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
const char* BOILER_ERR_EXHAUST[MAX_LANG]        =   {"Kessel_Fehler_Abgas_max_Grenzwert",           "boiler_failure_exhaust_gas_over_limit"};
const char* BOILER_ERR_SAFETY[MAX_LANG]         =   {"Kessel_Fehler_Sicherungskette",               "boiler_failure_safety_chain"};
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
const char* BOILER_CONSUMPTION[MAX_LANG]        =   {"Oelverbrauch_Gesamt_berechnet",               "oil_consumption_overall_calc"};

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
// 1. GERMAN
// 2. ENGLISH
// ======================================================================================
typedef struct {
    const char* OPMODE[MAX_LANG][3]={
        {"Nacht", "Tag", "Automatik"},
        {"night", "day", "auto"}
    };
    const char* SCREEN[MAX_LANG][4]={
        {"Automatik", "Kessel", "Warmwasser", "Aussen"},
        {"auto", "boiler", "DHW", "outdoor"}
    };
    const char* LANGUAGE[MAX_LANG][6]={
        {"DE", "FR", "IT", "NL", "EN", "PL"},
        {"DE", "FR", "IT", "NL", "EN", "PL"}
    };
    const char* REDUCT_MODE[MAX_LANG][4]={
        {"Abschalt", "Reduziert", "Raumhalt", "Aussenhalt"},
        {"off", "fixed", "room", "outdoors"}
    };
    const char* SUMMER[MAX_LANG][23]={
        {"Sommer","10 °C","11 °C","12 °C","13 °C","14 °C","15 °C","16 °C","17 °C","18 °C","19 °C","20 °C","21 °C","22 °C","23 °C","24 °C","25 °C","26 °C","27 °C","28 °C","29 °C","30 °C","Winter"},
        {"summer","10 °C","11 °C","12 °C","13 °C","14 °C","15 °C","16 °C","17 °C","18 °C","19 °C","20 °C","21 °C","22 °C","23 °C","24 °C","25 °C","26 °C","27 °C","28 °C","29 °C","30 °C","winter"}
    };
    const char* SWITCH_ON_TEMP[MAX_LANG][11]={
        {"Aus","1 °C","2 °C","3 °C","4 °C","5 °C","6 °C","7 °C","8 °C","9 °C","10 °C"},
        {"off","1 °C","2 °C","3 °C","4 °C","5 °C","6 °C","7 °C","8 °C","9 °C","10 °C"},
    };
    const char* HEATING_SYSTEM[MAX_LANG][4]={
        {"Aus","Heizkoerper","-","Fussboden"},
        {"off","radiator","-","underfloor"}
    };
    const char* ON_OFF[2][2]={
        {"Aus","An"},
        {"off","on"}
    };
    const char* BUILDING_TYPE[MAX_LANG][3]={
        {"Leicht","Mittel","Schwer"},
        {"light","medium","heavy"}
    };
    const char* CIRC_INTERVAL[MAX_LANG][8]={
        {"Aus","1","2","3","4","5","6","An"},
        {"off","1","2","3","4","5","6","on"}
    };
    const char* BURNER_TYPE[MAX_LANG][3]={
        {"1-stufig","2-stufig","Modulierend"},
        {"1-stage","2-stage","modulated"}
    };
    const char* EXHAUST_GAS_THRESHOLD[MAX_LANG][42]={
        {"Aus","50","55","60","65","70","75","80","85","90","95","100","105","110","115","120","125","130","135","140","145","150","155","160","165","170","175","180","185","190","195","200","205","210","215","220","225","230","235","240","245","250"},
        {"off","50","55","60","65","70","75","80","85","90","95","100","105","110","115","120","125","130","135","140","145","150","155","160","165","170","175","180","185","190","195","200","205","210","215","220","225","230","235","240","245","250"}
    };
    const char* HC_PROGRAM[MAX_LANG][9]={
        {"Eigen","Familie","Frueh","Spaet","Vormittag","Nachmittag","Mittag","Single","Senior"},
        {"custom","family","early","late","AM","PM","noon","single","senior"}
    };
    const char* BURNER_STATE[MAX_LANG][5]={
        {"Kessel AUS", "EIN (Stufe 1)", "-", "-", "EIN (Stufe 2)"},
        {"Burner OFF", "ON (Stage 1)", "-", "-", "ON (Stage 2)"}
    };
} s_cfg_arrays;


// ======================================================================================
// decription for encode error messages
// ======================================================================================
typedef struct {
    const char* idx[2][28] = {
        {   // GERMAN
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
        },
        {   // ENGLISH
            "no error",
            "failure outdoor sensor",
            "failure hc1-flow sensor",
            "failure hc1-flow sensor",
            "failure warm water sensor",
            "Warm Water remains col",
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
        }
    };
} s_err_array;
