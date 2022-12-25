//*****************************************************************************
// The Software was created by Michael Meyer, but it was modified and adapted
// 
// File Name  : 'km271_prot.h'
// Title      : Handles 3964 protocol for KM271
// Author     : Michael Meyer
// Created    : 08.01.2022
// Version    : 0.1
// Target MCU : ESP32/Arduino
// Indicator  : km
//
//*****************************************************************************
#pragma once

#include <Arduino.h>

//*****************************************************************************
// Defines
//*****************************************************************************

// Configuration
#define KM271_EN_PROTLOG          0                                       // Enable/disable protocol logging (most of protocol bytes are reported, but DLE doubling is missing in RX!)
#define KM271_EN_PARSELOG         0                                       // Enable/disable parsing logging (only blocks to be parsed are reported)
#define KM271_EN_PARSE_RESULTLOG  1                                       // Enable/disable parsing result logging: Clear text logging.

// Protocol elements. Do not change, otherwise KM271 communication will fail!
#define KM271_BAUDRATE        2400                                        // The baudrate top be used for KM271 communication
#define KM_STX                0x02                                        // Protocol control bytes
#define KM_DLE                0x10
#define KM_ETX                0x03
#define KM_NAK                0x15

#define KM_RX_BUF_LEN         20                                          // Max number of RX bytes 
#define KM_TX_BUF_LEN         20                                          // Max number of TX bytes 

// ********* Used UART-Pins to connect to KM271
#define RXD2   4        // IO4               // ESP32 RX-pin for KM271 communication, align with hardware
#define TXD2   2        // IO2               // ESP32 TX-pin for KM271 communication, align with hardware


// The states to receive a single block of data.
// First a block iof data is received byte by byte by using this state interpreter.
// If a full block of data is received, a second level state interopreter is called to handle the blocks.
typedef enum {
  KM_RX_RESYNC,                                                           // Unknown state, re-sync by wait for STX
  KM_RX_IDLE,                                                             // Idle state for RX interrupt routine
  KM_RX_ON,                                                               // Block reception started
  KM_RX_DLE,                                                              // DLE doubling
  KM_RX_BCC,                                                              // Verify block
} e_rxState;

// The higher level states used in handleRxBlock();
typedef enum {
  KM_TSK_START,                                                           // Switch to logging mode
  KM_TSK_LG_CMD,                                                          // Receive confirmation from KM
  KM_TSK_LOGGING,                                                         // Logging active
} e_rxBlockState; 

typedef struct {                                                          // Rx structure for one rx block
  uint8_t                   len;                                          // Length of data in buffer
  uint8_t                   buf[KM_RX_BUF_LEN];                           // Received bytes without "10 03 bcc"
} KmRx_s;


// This struicure contains all values read from the heating controller.
// This structure is kept up-to-date automatically by the km271.cpp.
// Use km271GetStatus() to get the most recent copy of these values in a thread-safe manner.
typedef struct {
  // Retrieved values
  uint8_t   HC1_OperatingStates_1;                        // 0x8000 : Bitfield
  uint8_t   HC1_OperatingStates_2;                        // 0x8001 : Bitfield
  float     HC1_HeatingForwardTargetTemp;                 // 0x8002 : Temperature (1C resolution)
  float     HC1_HeatingForwardActualTemp;                 // 0x8003 : Temperature (1C resolution)
  float     HC1_RoomTargetTemp;                           // 0x8004 : Temperature (0.5C resolution)
  float     HC1_RoomActualTemp;                           // 0x8005 : Temperature (0.5C resolution)
  uint8_t   HC1_SwitchOnOptimizationTime;                 // 0x8006 : Minutes
  uint8_t   HC1_SwitchOffOptimizationTime;                // 0x8007 : Minutes
  uint8_t   HC1_PumpPower;                                // 0x8008 : Percent
  uint8_t   HC1_MixingValue;                              // 0x8009 : Percent
  float     HC1_HeatingCurvePlus10;                       // 0x800c : Temperature (1C resolution)
  float     HC1_HeatingCurve0;                            // 0x800d : Temperature (1C resolution)
  float     HC1_HeatingCurveMinus10;                      // 0x800e : Temperature (1C resolution)
  uint8_t   HC2_OperatingStates_1;                        // 0x8112 : Bitfield
  uint8_t   HC2_OperatingStates_2;                        // 0x8113 : Bitfield
  float     HC2_HeatingForwardTargetTemp;                 // 0x8114 : Temperature (1C resolution)
  float     HC2_HeatingForwardActualTemp;                 // 0x8115 : Temperature (1C resolution)
  float     HC2_RoomTargetTemp;                           // 0x8116 : Temperature (0.5C resolution)
  float     HC2_RoomActualTemp;                           // 0x8117 : Temperature (0.5C resolution)
  uint8_t   HC2_SwitchOnOptimizationTime;                 // 0x8118 : Minutes
  uint8_t   HC2_SwitchOffOptimizationTime;                // 0x8119 : Minutes
  uint8_t   HC2_PumpPower;                                // 0x811a : Percent
  uint8_t   HC2_MixingValue;                              // 0x811b : Percent
  float     HC2_HeatingCurvePlus10;                       // 0x811e : Temperature (1C resolution)
  float     HC2_HeatingCurve0;                            // 0x811f : Temperature (1C resolution)
  float     HC2_HeatingCurveMinus10;                      // 0x8120 : Temperature (1C resolution) 
  uint8_t   HotWaterOperatingStates_1;                    // 0x8424 : Bitfield
  uint8_t   HotWaterOperatingStates_2;                    // 0x8425 : Bitfield
  float     HotWaterTargetTemp;                           // 0x8426 : Temperature (1C resolution)
  float     HotWaterActualTemp;                           // 0x8427 : Temperature (1C resolution)
  uint8_t   HotWaterOptimizationTime;                     // 0x8428 : Minutes
  uint8_t   HotWaterPumpStates;                           // 0x8429 : Bitfield
  float     BoilerForwardTargetTemp;                      // 0x882a : Temperature (1C resolution)
  float     BoilerForwardActualTemp;                      // 0x882b : Temperature (1C resolution)
  float     BurnerSwitchOnTemp;                           // 0x882c : Temperature (1C resolution)
  float     BurnerSwitchOffTemp;                          // 0x882d : Temperature (1C resolution)
  uint8_t   BoilerIntegral_1;                             // 0x882e : Number (*256)
  uint8_t   BoilerIntegral_2;                             // 0x882f : Number (*1)
  uint8_t   BoilerErrorStates;                            // 0x8830 : Bitfield
  uint8_t   BoilerOperatingStates;                        // 0x8831 : Bitfield
  uint8_t   BurnerStates;                                 // 0x8832 : Bitfield
  float     ExhaustTemp;                                  // 0x8833 : Temperature (1C resolution)
  uint8_t   BurnerOperatingDuration_2;                    // 0x8836 : Minutes (*65536)
  uint8_t   BurnerOperatingDuration_1;                    // 0x8837 : Minutes (*256)
  uint8_t   BurnerOperatingDuration_0;                    // 0x8838 : Minutes (*1)
  float     OutsideTemp;                                  // 0x893c : Temperature (1C resolution, possibly negative)
  float     OutsideDampedTemp;                            // 0x893e : Temperature (1C resolution, possibly negative)
  uint8_t   ControllerVersionMain;                        // 0x893e : Number
  uint8_t   ControllerVersionSub;                         // 0x893f : Number
  uint8_t   Modul;                                        // 0x8940 : Number
  uint8_t   ERR_Alarmstatus;                              // 0xaa42 : Bitfield
} s_km271_status;


// Return values used by the KM271 protocol functions
typedef enum {
  RET_OK = 0,
  RET_ERR,
} e_ret;

// send commands to KM271
typedef enum {
  KM271_SENDCMD_HC1_OPMODE,               // HK1 Betriebsart
  KM271_SENDCMD_HC1_DESIGN_TEMP,          // HK1 Auslegung
  KM271_SENDCMD_HC1_PROGRAMM,             // HK1 Programm
  KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, // HK1 Aussehnhalt ab
  KM271_SENDCMD_HC1_DAY_SETPOINT,         // HK1 Tag-Soll
  KM271_SENDCMD_HC1_NIGHT_SETPOINT,       // HK1 Nacht-Soll
  KM271_SENDCMD_HC1_HOLIDAY_SETPOINT,     // HK1 Ferien-Soll
  KM271_SENDCMD_HC2_OPMODE,               // HK2 Betriebsart
  KM271_SENDCMD_HC2_DESIGN_TEMP,          // HK2 Auslegung
  KM271_SENDCMD_HC2_PROGRAMM,             // HK2 Programm
  KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, // HK2 Aussehnhalt ab
  KM271_SENDCMD_HC2_DAY_SETPOINT,         // HK2 Tag-Soll
  KM271_SENDCMD_HC2_NIGHT_SETPOINT,       // HK2 Nacht-Soll
  KM271_SENDCMD_HC2_HOLIDAY_SETPOINT,     // HK2 Ferien-Soll
  KM271_SENDCMD_WW_OPMODE,                // Warmwasser Vetriebsart
  KM271_SENDCMD_SUMMER,                   // Sommer ab
  KM271_SENDCMD_FROST,                    // Frost ab
  KM271_SENDCMD_WW_SETPOINT,              // Warmwasser soll
  KM271_SENDCMD_HC1_HOLIDAYS,             // HK1 Ferien Tage
  KM271_SENDCMD_HC2_HOLIDAYS,             // HK2 Ferien Tage
} e_km271_sendCmd;


//*****************************************************************************
// Function prototypes
//*****************************************************************************
e_ret km271ProtInit(int rxPin, int txPin);                        // Initializes the KM271 communication. To be called once by setup().
void  km271GetStatus(s_km271_status *pDestStatus);                // Retrieves the current status
void sendTxBlock(uint8_t *data, int len);
void handleRxBlock(uint8_t *data, int len, uint8_t bcc);
void parseInfo(uint8_t *data, int len);
float decode05cTemp(uint8_t data);
float decodeNegValue(uint8_t data);
void cyclicKM271();
void sendKM271Info();
void km271sendCmd(e_km271_sendCmd sendCmd, int8_t cmdPara);
void km271sendCmdFlt(e_km271_sendCmd sendCmd, float cmdPara);
bool km271GetLogMode();
void km271SetDateTime();
void decodeTimer(char * timerInfo, uint8_t dateOnOff, uint8_t time);
const char * addCfgTopic(const char *suffix);
const char * addStatTopic(const char *suffix);
const char * addAlarmTopic(const char *suffix);
uint8_t getErrorTextIndex(uint8_t errorNr);
void decodeErrorMsg(char * errorMsg, uint8_t *data);