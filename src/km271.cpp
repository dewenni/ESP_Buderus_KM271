//*****************************************************************************
// The Software was created by Michael Meyer, but it was modified and adapted
// 
// Title      : Handles 3964 protocol for KM271
// Author     : Michael Meyer (Codebase)
// Target MCU : ESP32/Arduino
//
//*****************************************************************************

#include <km271.h>
#include <basics.h>

/* V A R I A B L E S ********************************************************/
SemaphoreHandle_t    accessMutex;                                  // To protect access to kmState structure
s_mqtt_messags       mqttMsg;                                      // texts for mqtt messages
s_err_array          errMsgText;                                   // Array of error messages
 
// This structure contains the complete received status of the KM271 (as far it is parsed by now).
// It is updated automatically on any changes by this driver and therefore kept up-to-date.
// Do not access it directly from outside this source to avoid inconsistency of the structure.
// Instead, use km271GetStatus() to get a current copy in a safe manner.
s_km271_status       kmState;                                      // All current KM271 values       

// Status machine handling
e_rxBlockState       KmRxBlockState= KM_TSK_START;                 // The RX block state

// known commands to KM271, to be used with KmStartTx()
uint8_t KmCSTX[]     = {KM_STX};                                   // STX Command
uint8_t KmCDLE[]     = {KM_DLE};                                   // DLE Response
uint8_t KmCNAK[]     = {KM_NAK};                                   // NAK Response
uint8_t KmCLogMode[] = {0xEE, 0x00, 0x00};                         // Switch to Log Mode

// ************************ km271 handling variables ****************************
uint8_t     rxByte;                                                // The received character. We are reading byte by byte only.
e_rxState   kmRxStatus = KM_RX_RESYNC;                             // Status in Rx reception
uint8_t     kmRxBcc  = 0;                                          // BCC value for Rx Block
KmRx_s      kmRxBuf;                                               // Rx block storag
uint8_t     send_cmd;
uint8_t     send_buf[8] = {};
bool        km271LogModeActive = false;


/**
 * *******************************************************************
 * @brief   Sends a single block of data.
 * @details Data is given as "pure" data without protocol bytes.
 *          This function adds the protocol bytes (including BCC calculation) and
 *          sends it to the Ecomatic 2000.
 *          STX and DLE handling needs to be done outside in handleRxBlock().
 * @param   data: Pointer to the data to be send. Single byte data is send right away (i.e,. ptotocol bytes such as STX. DLE, NAK...)
 *                Block data is prepared with doubling DLE and block end indication.
 * @param   len:  Blocksize in number of bytes, without protocol data 
 * @return  none
 * 
 * *******************************************************************/
void sendTxBlock(uint8_t *data, int len) {
  uint8_t       buf[256];                                                   // To store bytes to be sent
  int           ii, txLen;
  uint8_t       bcc=0;
  
  if(!len) return;                                                          // Nothing to do
  if((len == 1) && ((data[0] == KM_STX) || (data[0] == KM_DLE) || (data[0] == KM_NAK))) {   // Shall a single protocol byte be sent? If yes, send it right away.
    Serial2.write(data, 1);
    return;
  }
  // Here, we need to send a whole block of data. So prepare data.
  for(txLen = 0, ii = 0; ii < len; ii++, txLen++) {
    if(ii) bcc ^= data[ii]; else bcc = data[ii];                            // Initialize / calculate bcc
    buf[txLen] = data[ii];
    if(data[ii] == KM_DLE) {                                                // Doubling of DLE needed?
      bcc ^= data[ii];                                                      // Consider second DLE in BCC calculation
      txLen++;                                                              // Make space for second DLE         
      buf[txLen] = data[ii];                                                // Store second DLE
    }
  }
  // Append buffer with DLE, ETX, BCC
  bcc ^= KM_DLE;
  buf[txLen] = KM_DLE;

  txLen++;                                                                  // Make space for ETX         
  bcc ^= KM_ETX;
  buf[txLen] = KM_ETX;

  txLen++;                                                                  // Make space for BCC
  buf[txLen] = bcc;
  txLen++;
  
  Serial2.write(buf, txLen);                                                // Send the complete block  
}

/**
 * *******************************************************************
 * @brief   Interpretation of an received information block
 * @details Checks and handles the information data received.
 *          Handles update of the global s_km271_status and provides event notifications
 *          to other tasks (if requested).
 *          Status data is handles under task lock to ensure consistency of status structure.
 * @param   data: Pointer to the block of data received.
 * @param   len:  Blocksize in number of bytes, without protocol data 
 * @return  none
 * *******************************************************************/
void parseInfo(uint8_t *data, int len) {
  s_km271_status  tmpState;
  s_cfg_topics    cfgTopic;
  s_stat_topics   statTopic;
  s_cfg_arrays    cfgArray;
  s_error_topics  errTopic;

  char            t1[100]={'\0'};
  char            t2[100]={'\0'};
  char            t3[100]={'\0'};
  char            tmpMessage[300]={'\0'};
  char            errorMsg[300]={'\0'};

  // Get current state
  xSemaphoreTake(accessMutex, portMAX_DELAY);               // Prevent task switch to ensure the whole structure remains constistent
  memcpy(&tmpState, &kmState, sizeof(s_km271_status));
  xSemaphoreGive(accessMutex);
  uint kmregister = (data[0] * 256) + data[1];

  /********************************************************
  * publish all incomming messages for debug reasons
  ********************************************************/
  #ifdef DEBUG_ON 
    if (kmregister != 0x0400 && kmregister != 0x882e && kmregister != 0x882f){
      sprintf(tmpMessage, "%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);
      mqttPublish(addTopic("/debug_message"), tmpMessage, false); 
    }
  #endif 

  switch(kmregister) {                                     // Check if we can find known stati
 
    /********************************************************
    * status values Heating Circuit 1
    ********************************************************/
    
    case 0x8000: // 0x8000 : Bitfield                                                           
      #ifdef USE_HC1
      tmpState.HC1_OperatingStates_1 = data[2];                 
      mqttPublish(addStatTopic(statTopic.HC1_OV1_OFFTIME_OPT[LANG]),String(bitRead(tmpState.HC1_OperatingStates_1, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_ONTIME_OPT[LANG]), String(bitRead(tmpState.HC1_OperatingStates_1, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_AUTOMATIC[LANG]),  String(bitRead(tmpState.HC1_OperatingStates_1, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_WW_PRIO[LANG]),    String(bitRead(tmpState.HC1_OperatingStates_1, 3)).c_str(), false);        
      mqttPublish(addStatTopic(statTopic.HC1_OV1_SCREED_DRY[LANG]), String(bitRead(tmpState.HC1_OperatingStates_1, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_HOLIDAY[LANG]),    String(bitRead(tmpState.HC1_OperatingStates_1, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_FROST[LANG]),      String(bitRead(tmpState.HC1_OperatingStates_1, 6)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV1_MANUAL[LANG]),     String(bitRead(tmpState.HC1_OperatingStates_1, 7)).c_str(), false);   
      #endif
      break;
      
    case 0x8001: // 0x8001 : Bitfield                                                          
      #ifdef USE_HC1
      tmpState.HC1_OperatingStates_2 = data[2];                 
      mqttPublish(addStatTopic(statTopic.HC1_OV2_SUMMER[LANG]),        String(bitRead(tmpState.HC1_OperatingStates_2, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV2_DAY[LANG]),           String(bitRead(tmpState.HC1_OperatingStates_2, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV2_NO_COM_REMOTE[LANG]), String(bitRead(tmpState.HC1_OperatingStates_2, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV2_REMOTE_ERR[LANG]),    String(bitRead(tmpState.HC1_OperatingStates_2, 3)).c_str(), false); 
      mqttPublish(addStatTopic(statTopic.HC1_OV2_FLOW_SENS_ERR[LANG]), String(bitRead(tmpState.HC1_OperatingStates_2, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV2_FLOW_AT_MAX[LANG]),   String(bitRead(tmpState.HC1_OperatingStates_2, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC1_OV2_EXT_SENS_ERR[LANG]),  String(bitRead(tmpState.HC1_OperatingStates_2, 6)).c_str(), false);        
      #endif
      break;
      
    case 0x8002: // 0x8002 : Temperature (1C resolution)
      #ifdef USE_HC1
      tmpState.HC1_HeatingForwardTargetTemp = (float)data[2];                 
      mqttPublish(addStatTopic(statTopic.HC1_FLOW_SETPOINT[LANG]), String(tmpState.HC1_HeatingForwardTargetTemp).c_str(), false);  
      #endif
      break;
      
    case 0x8003: // 0x8003 : Temperature (1C resolution)
      #ifdef USE_HC1
      tmpState.HC1_HeatingForwardActualTemp = (float)data[2];                 
      mqttPublish(addStatTopic(statTopic.HC1_FLOW_TEMP[LANG]), String(tmpState.HC1_HeatingForwardActualTemp).c_str(), false);
      #endif
      break;
      
    case 0x8004: // 0x8004 : Temperature (0.5C resolution)
      #ifdef USE_HC1
      tmpState.HC1_RoomTargetTemp = decode05cTemp(data[2]);                   
      mqttPublish(addStatTopic(statTopic.HC1_ROOM_SETPOINT[LANG]), String(tmpState.HC1_RoomTargetTemp).c_str(), false);
      #endif
      break;
      
    case 0x8005: // 0x8005 : Temperature (0.5C resolution)
      #ifdef USE_HC1
      tmpState.HC1_RoomActualTemp = decode05cTemp(data[2]);                   
      mqttPublish(addStatTopic(statTopic.HC1_ROOM_TEMP[LANG]), String(tmpState.HC1_RoomActualTemp).c_str(), false);
      #endif
      break;
      
    case 0x8006: // 0x8006 : Minutes
      #ifdef USE_HC1
      tmpState.HC1_SwitchOnOptimizationTime = data[2];                        
      mqttPublish(addStatTopic(statTopic.HC1_ON_TIME_OPT[LANG]), String(tmpState.HC1_SwitchOnOptimizationTime).c_str(), false);
      #endif
      break;
      
    case 0x8007: // 0x8007 : Minutes  
      #ifdef USE_HC1
      tmpState.HC1_SwitchOffOptimizationTime = data[2];                       
      mqttPublish(addStatTopic(statTopic.HC1_OV1_OFFTIME_OPT[LANG]), String(tmpState.HC1_SwitchOffOptimizationTime).c_str(), false);
      #endif
      break;
      
    case 0x8008: // 0x8008 : Percent 
      #ifdef USE_HC1
      tmpState.HC1_PumpPower = data[2];                                       
      mqttPublish(addStatTopic(statTopic.HC1_PUMP[LANG]), String(tmpState.HC1_PumpPower).c_str(), false);
      #endif
      break;
      
    case 0x8009: // 0x8009 : Percent
      #ifdef USE_HC1
      tmpState.HC1_MixingValue = data[2];                                     
      mqttPublish(addStatTopic(statTopic.HC1_MIXER[LANG]), String(tmpState.HC1_MixingValue).c_str(), false);
      #endif
      break;
      
    case 0x800c: // 0x800c : Temperature (1C resolution)
      #ifdef USE_HC1
      tmpState.HC1_HeatingCurvePlus10 = (float)data[2];                       
      mqttPublish(addStatTopic(statTopic.HC1_HEAT_CURVE1[LANG]), String(tmpState.HC1_HeatingCurvePlus10).c_str(), false);
      #endif
      break;
      
    case 0x800d:  // 0x800d : Temperature (1C resolution)
      #ifdef USE_HC1
      tmpState.HC1_HeatingCurve0 = (float)data[2];                           
      mqttPublish(addStatTopic(statTopic.HC1_HEAT_CURVE2[LANG]), String(tmpState.HC1_HeatingCurve0).c_str(), false);
      #endif
      break;
      
    case 0x800e: // 0x800e : Temperature (1C resolution)
      #ifdef USE_HC1
      tmpState.HC1_HeatingCurveMinus10 = (float)data[2];                      
      mqttPublish(addStatTopic(statTopic.HC1_HEAT_CURVE3[LANG]), String(tmpState.HC1_HeatingCurveMinus10).c_str(), false);
      #endif
    break;
    
    
    /********************************************************
    * status values Heating Circuit 2
    ********************************************************/
    case 0x8112: // 0x8112 : Bitfield                                                          
      #ifdef USE_HC2
      tmpState.HC2_OperatingStates_1 = data[2];                 
      mqttPublish(addStatTopic(statTopic.HC2_OV1_OFFTIME_OPT[LANG]),String(bitRead(tmpState.HC2_OperatingStates_1, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_ONTIME_OPT[LANG]), String(bitRead(tmpState.HC2_OperatingStates_1, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_AUTOMATIC[LANG]),  String(bitRead(tmpState.HC2_OperatingStates_1, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_WW_PRIO[LANG]),    String(bitRead(tmpState.HC2_OperatingStates_1, 3)).c_str(), false);        
      mqttPublish(addStatTopic(statTopic.HC2_OV1_SCREED_DRY[LANG]), String(bitRead(tmpState.HC2_OperatingStates_1, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_HOLIDAY[LANG]),    String(bitRead(tmpState.HC2_OperatingStates_1, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_FROST[LANG]),      String(bitRead(tmpState.HC2_OperatingStates_1, 6)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV1_MANUAL[LANG]),     String(bitRead(tmpState.HC2_OperatingStates_1, 7)).c_str(), false);   
      #endif
      break;
      
    case 0x8113: // 0x8113 : Bitfield                                                          
      #ifdef USE_HC2
      tmpState.HC2_OperatingStates_2 = data[2];                
      mqttPublish(addStatTopic(statTopic.HC2_OV2_SUMMER[LANG]),        String(bitRead(tmpState.HC2_OperatingStates_2, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV2_DAY[LANG]),           String(bitRead(tmpState.HC2_OperatingStates_2, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV2_NO_COM_REMOTE[LANG]), String(bitRead(tmpState.HC2_OperatingStates_2, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV2_REMOTE_ERR[LANG]),    String(bitRead(tmpState.HC2_OperatingStates_2, 3)).c_str(), false); 
      mqttPublish(addStatTopic(statTopic.HC2_OV2_FLOW_SENS_ERR[LANG]), String(bitRead(tmpState.HC2_OperatingStates_2, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV2_FLOW_AT_MAX[LANG]),   String(bitRead(tmpState.HC2_OperatingStates_2, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.HC2_OV2_EXT_SENS_ERR[LANG]),  String(bitRead(tmpState.HC2_OperatingStates_2, 6)).c_str(), false);        
      #endif
      break;
      
    case 0x8114: // 0x8114 : Temperature (1C resolution)
      #ifdef USE_HC2
      tmpState.HC2_HeatingForwardTargetTemp = (float)data[2];              
      mqttPublish(addStatTopic(statTopic.HC2_FLOW_SETPOINT[LANG]), String(tmpState.HC2_HeatingForwardTargetTemp).c_str(), false);  
      #endif
      break;
      
    case 0x8115: // 0x8115 : Temperature (1C resolution)
      #ifdef USE_HC2
      tmpState.HC2_HeatingForwardActualTemp = (float)data[2];                
      mqttPublish(addStatTopic(statTopic.HC2_FLOW_TEMP[LANG]), String(tmpState.HC2_HeatingForwardActualTemp).c_str(), false);
      #endif
      break;
      
    case 0x8116: // 0x8116 : Temperature (0.5C resolution)
      #ifdef USE_HC2
      tmpState.HC2_RoomTargetTemp = decode05cTemp(data[2]);              
      mqttPublish(addStatTopic(statTopic.HC2_ROOM_SETPOINT[LANG]), String(tmpState.HC2_RoomTargetTemp).c_str(), false);
      #endif
      break;
      
    case 0x8117: // 0x8117 : Temperature (0.5C resolution)
      #ifdef USE_HC2
      tmpState.HC2_RoomActualTemp = decode05cTemp(data[2]);                  
      mqttPublish(addStatTopic(statTopic.HC2_ROOM_TEMP[LANG]), String(tmpState.HC2_RoomActualTemp).c_str(), false);
      #endif
      break;
      
    case 0x8118: // 0x8118 : Minutes
      #ifdef USE_HC2
      tmpState.HC2_SwitchOnOptimizationTime = data[2];                       
      mqttPublish(addStatTopic(statTopic.HC2_ON_TIME_OPT[LANG]), String(tmpState.HC2_SwitchOnOptimizationTime).c_str(), false);
      #endif
      break;
      
    case 0x8119: // 0x8119 : Minutes 
      #ifdef USE_HC2
      tmpState.HC2_SwitchOffOptimizationTime = data[2];                     
      mqttPublish(addStatTopic(statTopic.HC2_OV1_OFFTIME_OPT[LANG]), String(tmpState.HC2_SwitchOffOptimizationTime).c_str(), false);
      #endif
      break;
      
    case 0x811a: // 0x811a : Percent  
      #ifdef USE_HC2
      tmpState.HC2_PumpPower = data[2];                                     
      mqttPublish(addStatTopic(statTopic.HC2_PUMP[LANG]), String(tmpState.HC2_PumpPower).c_str(), false);
      #endif
      break;
      
    case 0x811b: // 0x811b : Percent
      #ifdef USE_HC2
      tmpState.HC2_MixingValue = data[2];                                 
      mqttPublish(addStatTopic(statTopic.HC2_MIXER[LANG]), String(tmpState.HC2_MixingValue).c_str(), false);
      #endif
      break;
      
    case 0x811e: // 0x811e : Temperature (1C resolution)
      #ifdef USE_HC2
      tmpState.HC2_HeatingCurvePlus10 = (float)data[2];                   
      mqttPublish(addStatTopic(statTopic.HC2_HEAT_CURVE1[LANG]), String(tmpState.HC2_HeatingCurvePlus10).c_str(), false);
      #endif
      break;
      
    case 0x811f: // 0x811f : Temperature (1C resolution)
      #ifdef USE_HC2
      tmpState.HC2_HeatingCurve0 = (float)data[2];                          
      mqttPublish(addStatTopic(statTopic.HC2_HEAT_CURVE2[LANG]), String(tmpState.HC2_HeatingCurve0).c_str(), false);
      #endif
      break;
      
    case 0x8120: // 0x8120 : Temperature (1C resolution) 
      #ifdef USE_HC2
      tmpState.HC2_HeatingCurveMinus10 = (float)data[2];                   
      mqttPublish(addStatTopic(statTopic.HC2_HEAT_CURVE3[LANG]), String(tmpState.HC2_HeatingCurveMinus10).c_str(), false);
      #endif
      break;
    

    /********************************************************
    * status values general
    ********************************************************/
    case 0x8424: // 0x8424 : Bitfield
      tmpState.HotWaterOperatingStates_1 = data[2];                       
      mqttPublish(addStatTopic(statTopic.WW_OV1_AUTO[LANG]),         String(bitRead(tmpState.HotWaterOperatingStates_1, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_DESINFECT[LANG]),    String(bitRead(tmpState.HotWaterOperatingStates_1, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_RELOAD[LANG]),       String(bitRead(tmpState.HotWaterOperatingStates_1, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_HOLIDAY[LANG]),      String(bitRead(tmpState.HotWaterOperatingStates_1, 3)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_ERR_DESINFECT[LANG]),String(bitRead(tmpState.HotWaterOperatingStates_1, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_ERR_SENSOR[LANG]),   String(bitRead(tmpState.HotWaterOperatingStates_1, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV1_WW_STAY_COLD[LANG]), String(bitRead(tmpState.HotWaterOperatingStates_1, 6)).c_str(), false);        
      mqttPublish(addStatTopic(statTopic.WW_OV1_ERR_ANODE[LANG]),    String(bitRead(tmpState.HotWaterOperatingStates_1, 7)).c_str(), false);   
      break;
      
    case 0x8425: // 0x8425 : Bitfield
      tmpState.HotWaterOperatingStates_2 = data[2];                       
      mqttPublish(addStatTopic(statTopic.WW_OV2_LOAD[LANG]),         String(bitRead(tmpState.HotWaterOperatingStates_2, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_MANUAL[LANG]),       String(bitRead(tmpState.HotWaterOperatingStates_2, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_RELOAD[LANG]),       String(bitRead(tmpState.HotWaterOperatingStates_2, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_OFF_TIME_OPT[LANG]), String(bitRead(tmpState.HotWaterOperatingStates_2, 3)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_ON_TIME_OPT[LANG]),  String(bitRead(tmpState.HotWaterOperatingStates_2, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_DAY[LANG]),          String(bitRead(tmpState.HotWaterOperatingStates_2, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_OV2_HOT[LANG]),          String(bitRead(tmpState.HotWaterOperatingStates_2, 6)).c_str(), false);        
      mqttPublish(addStatTopic(statTopic.WW_OV2_PRIO[LANG]),         String(bitRead(tmpState.HotWaterOperatingStates_2, 7)).c_str(), false);   
      break;
      
    case 0x8426: // 0x8426 : Temperature (1C resolution)
      tmpState.HotWaterTargetTemp = (float)data[2];                       
      mqttPublish(addStatTopic(statTopic.WW_SETPOINT[LANG]), String(tmpState.HotWaterTargetTemp).c_str(), false);
      break;
      
    case 0x8427: // 0x8427 : Temperature (1C resolution)
      tmpState.HotWaterActualTemp = (float)data[2];                       
      mqttPublish(addStatTopic(statTopic.WW_TEMP[LANG]), String(tmpState.HotWaterActualTemp).c_str(), false);
      break;
      
    case 0x8428: // 0x8428 : Minutes
      tmpState.HotWaterOptimizationTime = data[2];                        
      mqttPublish(addStatTopic(statTopic.WW_OV2_ON_TIME_OPT[LANG]), String(tmpState.HotWaterOptimizationTime).c_str(), false);
      break;
      
    case 0x8429: // 0x8429 :  Bitfield
      tmpState.HotWaterPumpStates = data[2];                              
      mqttPublish(addStatTopic(statTopic.WW_PUMP_CHARGE[LANG]), String(bitRead(tmpState.HotWaterPumpStates, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_PUMP_CIRC[LANG]), String(bitRead(tmpState.HotWaterPumpStates, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.WW_PUMP_SOLAR[LANG]), String(bitRead(tmpState.HotWaterPumpStates, 2)).c_str(), false);
      break;
      
    case 0x882a: // 0x882a : Temperature (1C resolution)
      tmpState.BoilerForwardTargetTemp  = (float)data[2];                 
      mqttPublish(addStatTopic(statTopic.BOILER_SETPOINT[LANG]), String(tmpState.BoilerForwardTargetTemp).c_str(), false);
      break;
      
    case 0x882b: // 0x882b : Temperature (1C resolution)
      tmpState.BoilerForwardActualTemp  = (float)data[2];                 
      mqttPublish(addStatTopic(statTopic.BOILER_TEMP[LANG]), String(tmpState.BoilerForwardActualTemp).c_str(), false);
      break;
      
    case 0x882c: // 0x882c : Temperature (1C resolution)
      tmpState.BurnerSwitchOnTemp  = (float)data[2];                      
      mqttPublish(addStatTopic(statTopic.BOILER_ON_TEMP[LANG]), String(tmpState.BurnerSwitchOnTemp).c_str(), false);
      break;
      
    case 0x882d: // 0x882d : Temperature (1C resolution)
      tmpState.BurnerSwitchOffTemp  = (float)data[2];                     
      mqttPublish(addStatTopic(statTopic.BOILER_OFF_TEMP[LANG]), String(tmpState.BurnerSwitchOffTemp).c_str(), false);
      break;
      
    case 0x882e:  // 0x882e : Number (*256)
      tmpState.BoilerIntegral_1 = data[2];                                
      // do nothing - useless value
      break;
      
    case 0x882f: // 0x882f : Number (*1)
      tmpState.BoilerIntegral_2  = data[2];                               
      // do nothing - useless value
      break;
      
    case 0x8830: // 0x8830 : Bitfield
      tmpState.BoilerErrorStates  = data[2];                              
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_BURNER[LANG]),      String(bitRead(tmpState.BoilerErrorStates, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_SENSOR[LANG]),      String(bitRead(tmpState.BoilerErrorStates, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_AUX_SENS[LANG]),    String(bitRead(tmpState.BoilerErrorStates, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_STAY_COLD[LANG]),   String(bitRead(tmpState.BoilerErrorStates, 3)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_GAS_SENS[LANG]),    String(bitRead(tmpState.BoilerErrorStates, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_EXHAUST[LANG]),     String(bitRead(tmpState.BoilerErrorStates, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_SAFETY[LANG]),      String(bitRead(tmpState.BoilerErrorStates, 6)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_ERR_EXT[LANG]),         String(bitRead(tmpState.BoilerErrorStates, 7)).c_str(), false);
      break;
      
    case 0x8831: // 0x8831 : Bitfield
      tmpState.BoilerOperatingStates = data[2];                           
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_GASTEST[LANG]), String(bitRead(tmpState.BoilerOperatingStates, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_STAGE1[LANG]),  String(bitRead(tmpState.BoilerOperatingStates, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_PROTECT[LANG]), String(bitRead(tmpState.BoilerOperatingStates, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_ACTIVE[LANG]),  String(bitRead(tmpState.BoilerOperatingStates, 3)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_PER_FREE[LANG]),String(bitRead(tmpState.BoilerOperatingStates, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_PER_HIGH[LANG]),String(bitRead(tmpState.BoilerOperatingStates, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_STATE_STAGE2[LANG]),  String(bitRead(tmpState.BoilerOperatingStates, 6)).c_str(), false);
      break;
      
    case 0x8832: // 0x8832 : Bitfield
      tmpState.BurnerStates = data[2];                                    
      // [ "Kessel aus"), "1.Stufe an"), "-"), "-"), "2.Stufe an bzw. Modulation frei" ]
      mqttPublish(addStatTopic(statTopic.BOILER_CONTROL[LANG]), String(tmpState.BurnerStates).c_str(), false);
      break;
      
    case 0x8833: // 0x8833 : Temperature (1C resolution)
      tmpState.ExhaustTemp = (float)data[2];                              
      mqttPublish(addStatTopic(statTopic.EXHAUST_TEMP[LANG]), String(tmpState.ExhaustTemp).c_str(), false);
      break;
      
    case 0x8836: // 0x8836 : Minutes (*65536)
      tmpState.BurnerOperatingDuration_0 = data[2];                       
      mqttPublish(addStatTopic(statTopic.BOILER_LIFETIME_1[LANG]), String(tmpState.BurnerOperatingDuration_0).c_str(), false);
      break;
      
    case 0x8837:  // 0x8837 : Minutes (*256)
      tmpState.BurnerOperatingDuration_1 = data[2];                      
      mqttPublish(addStatTopic(statTopic.BOILER_LIFETIME_2[LANG]), String(tmpState.BurnerOperatingDuration_1).c_str(), false);
      break;
      
    case 0x8838: // 0x8838 : Minutes (*1) + calculated sum of all 3 runtime values
      tmpState.BurnerOperatingDuration_2 = data[2];                       
      mqttPublish(addStatTopic(statTopic.BOILER_LIFETIME_3[LANG]), String(tmpState.BurnerOperatingDuration_2).c_str(), false);
      mqttPublish(addStatTopic(statTopic.BOILER_LIFETIME_4[LANG]), String((uint64_t)(tmpState.BurnerOperatingDuration_2+(tmpState.BurnerOperatingDuration_1*256)+(tmpState.BurnerOperatingDuration_0*65536))).c_str(), false);
      break;
      
    case 0x893c: // 0x893c : Temperature (1C resolution, possibly negative)
      tmpState.OutsideTemp = decodeNegValue(data[2]);                      
      mqttPublish(addStatTopic(statTopic.OUTSIDE_TEMP[LANG]), String(tmpState.OutsideTemp).c_str(), false);
      break;
      
    case 0x893d: // 0x893d : Temperature (1C resolution, possibly negative)
      tmpState.OutsideDampedTemp = decodeNegValue(data[2]);                
      mqttPublish(addStatTopic(statTopic.OUTSIDE_TEMP_DAMPED[LANG]), String(tmpState.OutsideDampedTemp).c_str(), false);
      break;
      
    case 0x893e: // 0x893e : Number
      tmpState.ControllerVersionMain = data[2];                          
      mqttPublish(addStatTopic(statTopic.VERSION_VK[LANG]), String(tmpState.ControllerVersionMain).c_str(), false);
      break;
      
    case 0x893f: // 0x893f : Number
      tmpState.ControllerVersionSub = data[2];                            
      mqttPublish(addStatTopic(statTopic.VERSION_NK[LANG]), String(tmpState.ControllerVersionSub).c_str(), false);
      break;
      
    case 0x8940: // 0x8940 : Number
      tmpState.Modul = data[2];                                           
      mqttPublish(addStatTopic(statTopic.MODULE_ID[LANG]), String(tmpState.Modul).c_str(), false);
      break;
    
    case 0xaa42: // 0xaa42 : Bitfeld
      tmpState.ERR_Alarmstatus = data[2];                                
      mqttPublish(addStatTopic(statTopic.ALARM_EXHAUST[LANG]),       String(bitRead(tmpState.ERR_Alarmstatus, 0)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_02[LANG]),            String(bitRead(tmpState.ERR_Alarmstatus, 1)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_BOILER_FLOW[LANG]),   String(bitRead(tmpState.ERR_Alarmstatus, 2)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_08[LANG]),            String(bitRead(tmpState.ERR_Alarmstatus, 3)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_BURNER[LANG]),        String(bitRead(tmpState.ERR_Alarmstatus, 4)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_20[LANG]),            String(bitRead(tmpState.ERR_Alarmstatus, 5)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_HC2_FLOW_SENS[LANG]), String(bitRead(tmpState.ERR_Alarmstatus, 6)).c_str(), false);
      mqttPublish(addStatTopic(statTopic.ALARM_80[LANG]),            String(bitRead(tmpState.ERR_Alarmstatus, 7)).c_str(), false);
      break;   

    case 0x0300: // 0x0300 : Error Buffer 1                               
      decodeErrorMsg(errorMsg, data);
      mqttPublish(addAlarmTopic(errTopic.ERR_BUFF_1[LANG]), errorMsg, false);
      break;   

    case 0x0307: // 0x0307 : Error Buffer 2                               
      decodeErrorMsg(errorMsg, data);
      mqttPublish(addAlarmTopic(errTopic.ERR_BUFF_2[LANG]), errorMsg, false);
      break; 

    case 0x030e: // 0x030e : Error Buffer 3                               
      decodeErrorMsg(errorMsg, data);
      mqttPublish(addAlarmTopic(errTopic.ERR_BUFF_3[LANG]), errorMsg, false);
      break; 

    case 0x0315: // 0x0315 : Error Buffer 4                             
      decodeErrorMsg(errorMsg, data);
      mqttPublish(addAlarmTopic(errTopic.ERR_BUFF_4[LANG]), errorMsg, false);
      break; 



    /*
    **********************************************************************************
    * config values beginnig with 0x00 
    * # Message address:byte_offset in the message
    * Attributes:
    *   d:x (divide), p:x (add), bf:x (bitfield), a:x (array), ne (generate no event)
    *   mb:x (multi-byte-message, x-bytes, low byte), s (signed value)
    *   t (timer - special handling), eh (error history - special handling)
    ***********************************************************************************
    */
    

    case 0x0000: 
      mqttPublish(addCfgTopic(cfgTopic.SUMMER_THRESHOLD[LANG]), (cfgArray.SUMMER[data[2+1]-9]), false);                                       // "CFG_Sommer_ab"            => "0000:1,p:-9,a"
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_NIGHT_TEMP[LANG]), String(decode05cTemp(data[2+2]) + String(" °C")).c_str(), false);       // "CFG_HK1_Nachttemperatur"  => "0000:2,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC1_DAY_TEMP[LANG]), String(decode05cTemp(data[2+3]) + String(" °C")).c_str(), false);         // "CFG_HK1_Tagtemperatur"     => "0000:3,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC1_OPMODE[LANG]), cfgArray.OPMODE[data[2+4]], false);                                  // "CFG_HK1_Betriebsart"       => "0000:4,a:4"
      mqttPublish(addCfgTopic(cfgTopic.HC1_HOLIDAY_TEMP[LANG]), String(decode05cTemp(data[2+5]) + String(" °C")).c_str(), false);      // "CFG_HK1_Urlaubtemperatur"   => "0000:5,d:2"
      #endif 
      break;

    case 0x000e: 
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_MAX_TEMP[LANG]), String(data[2+2] + String(" °C")).c_str(), false);        // "CFG_HK1_Max_Temperatur"    => "000e:2"
      mqttPublish(addCfgTopic(cfgTopic.HC1_INTERPR[LANG]), String(data[2+4]).c_str(), false);                             // CFG_HK1_Auslegung"          => "000e:4"
      #endif
      break;
    
    case 0x0015: 
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_SWITCH_ON_TEMP[LANG]), (cfgArray.SWITCH_ON_TEMP[data[2]] + String(" °C")).c_str(), false);  // "CFG_HK1_Aufschalttemperatur"  => "0015:0,a"
      mqttPublish(addCfgTopic(cfgTopic.HC1_SWITCH_OFF_THRESHOLD[LANG]), String(decodeNegValue(data[2+2]) + String(" °C")).c_str(), false);        // CFG_HK1_Aussenhalt_ab"         => "0015:2,s"
      #endif
      break;
    
    case 0x001c: 
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_REDUCTION_MODE[LANG]), cfgArray.REDUCT_MODE[data[2+1]], false);              // "CFG_HK1_Absenkungsart"    => "001c:1,a"
      mqttPublish(addCfgTopic(cfgTopic.HC1_HEATING_SYSTEM[LANG]), cfgArray.HEATING_SYSTEM[data[2+2]], false);           // "CFG_HK1_Heizsystem"       => "001c:2,a"
      #endif
      break;

    case 0x0031: 
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_TEMP_OFFSET[LANG]), String(decode05cTemp(decodeNegValue(data[2+3])) + String(" °C")).c_str(), false);     // "CFG_HK1_Temperatur_Offset"    => "0031:3,s,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC1_REMOTECTRL[LANG]), cfgArray.ON_OFF[data[2+4]], false);                                                   // "CFG_HK1_Fernbedienung"        => "0031:4,a"  
      #endif
      mqttPublish(addCfgTopic(cfgTopic.FROST_THRESHOLD[LANG]), String(decodeNegValue(data[2+5]) + String(" °C")).c_str(), false);                    // "CFG_Frost_ab"                 => "0031:5,s"
      break;

    case 0x0038:                                     
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_NIGHT_TEMP[LANG]), String(decode05cTemp(data[2+2]) + String(" °C")).c_str(), false);         // "CFG_HK2_Nachttemperatur"   => "0038:2,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC2_DAY_TEMP[LANG]), String(decode05cTemp(data[2+3]) + String(" °C")).c_str(), false);           // "CFG_HK2_Tagtemperatur"     => "0038:3,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC2_OPMODE[LANG]), cfgArray.OPMODE[data[2+4]], false);                                           // "CFG_HK2_Betriebsart"       => "0038:4,a:4"
      mqttPublish(addCfgTopic(cfgTopic.HC2_HOLIDAY_TEMP[LANG]), String(decode05cTemp(data[2+5]) + String(" °C")).c_str(), false);       // "CFG_HK2_Urlaubtemperatur"  => "0038:5,d:2"
      #endif 
      break;

    case 0x0046: 
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_MAX_TEMP[LANG]), String(data[2+2] + String(" °C")).c_str(), false);        // "CFG_HK2_Max_Temperatur"    => "0046:2"
      mqttPublish(addCfgTopic(cfgTopic.HC2_INTERPR[LANG]), String(data[2+4]).c_str(), false);                             // "CFG_HK2_Auslegung"         => "0046:4"
      #endif
      break;

    case 0x004d: 
      mqttPublish(addCfgTopic(cfgTopic.WW_PRIO[LANG]), cfgArray.ON_OFF[data[2+1]], false);     // "CFG_WW_Vorrang"   => "004d:1,a"
      
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_SWITCH_ON_TEMP[LANG]), (cfgArray.SWITCH_ON_TEMP[data[2]] + String(" °C")).c_str(), false);  // "CFG_HK1_Aufschalttemperatur"  => "004d:0,a"
      mqttPublish(addCfgTopic(cfgTopic.HC2_SWITCH_OFF_THRESHOLD[LANG]), String(decodeNegValue(data[2+2]) + String(" °C")).c_str(), false);         // CFG_HK1_Aussenhalt_ab"         => "004d:2,s"
      #endif
      
      break;

    case 0x0054: 
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_REDUCTION_MODE[LANG]), cfgArray.REDUCT_MODE[data[2+1]], false);     // "CFG_HK1_Absenkungsart"    => "0054:1,a"
      mqttPublish(addCfgTopic(cfgTopic.HC2_HEATING_SYSTEM[LANG]), cfgArray.HEATING_SYSTEM[data[2+2]], false);           // "CFG_HK1_Heizsystem"       => "0054:2,a"
      #endif
      break;
    
    case 0x0069: 
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_TEMP_OFFSET[LANG]), String(decode05cTemp(decodeNegValue(data[2+3])) + String(" °C")).c_str(), false);   // "CFG_HK2_Temperatur_Offset"    => "0069:3,s,d:2"
      mqttPublish(addCfgTopic(cfgTopic.HC2_REMOTECTRL[LANG]), cfgArray.ON_OFF[data[2+4]], false);                                                   // "CFG_HK2_Fernbedienung"        => "0069:4,a"  
      #endif
      break;

    case 0x0070: 
      mqttPublish(addCfgTopic(cfgTopic.BUILDING_TYP[LANG]), cfgArray.BUILDING_TYPE[data[2+2]], false);     // "CFG_Gebaeudeart"   => "0070:2,a" 
      break;

    case 0x007e: 
      mqttPublish(addCfgTopic(cfgTopic.WW_TEMP[LANG]), String(data[2+3] + String(" °C")).c_str(), false);     // "CFG_WW_Temperatur"  => "007e:3"
      break;

    case 0x0085: 
      mqttPublish(addCfgTopic(cfgTopic.WW_OPMODE[LANG]), cfgArray.OPMODE[data[2]], false);    // "CFG_WW_Betriebsart"  => "0085:0,a"
      mqttPublish(addCfgTopic(cfgTopic.WW_PROCESSING[LANG]), cfgArray.ON_OFF[data[2+3]], false);       // "CFG_WW_Aufbereitung"  => "0085:3,a"
      mqttPublish(addCfgTopic(cfgTopic.WW_CIRCULATION[LANG]), cfgArray.CIRC_INTERVAL[data[2+5]], false);  // "CFG_WW_Zirkulation"   => "0085:5,a"
      break;

    case 0x0093: 
      mqttPublish(addCfgTopic(cfgTopic.LANGUAGE[LANG]), cfgArray.LANGUAGE[data[2]], false);    // "CFG_Sprache"   => "0093:0"
      mqttPublish(addCfgTopic(cfgTopic.SCREEN[LANG]), cfgArray.SCREEN[data[2+1]], false);    // "CFG_Anzeige"   => "0093:1,a"
      break;

    case 0x009a: 
      mqttPublish(addCfgTopic(cfgTopic.BURNER_TYP[LANG]), cfgArray.BURNER_TYPE[data[2+1]-1], false);                     // "CFG_Brennerart"             => "009a:1,p:-1,a:12"),
      mqttPublish(addCfgTopic(cfgTopic.MAX_BOILER_TEMP[LANG]), String(data[2+3] + String(" °C")).c_str(), false);    // "CFG_Max_Kesseltemperatur"   => "009a:3"
      break;

    case 0x00a1: 
      mqttPublish(addCfgTopic(cfgTopic.PUMP_LOGIC[LANG]), String(data[2] + String(" °C")).c_str(), false);                    // "CFG_Pumplogik"                => "00a1:0"
      mqttPublish(addCfgTopic(cfgTopic.EXHAUST_THRESHOLD[LANG]), (cfgArray.EXHAUST_GAS_THRESHOLD[data[2+5]-9]), false);  // "CFG_Abgastemperaturschwelle"  => "00a1:5,p:-9,a"
      break;

    case 0x00a8: 
      mqttPublish(addCfgTopic(cfgTopic.BURNER_MIN_MOD[LANG]), String(data[2]).c_str(), false);   // "CFG_Brenner_Min_Modulation"     => "00a8:0"
      mqttPublish(addCfgTopic(cfgTopic.BURNER_MOD_TIME[LANG]), String(data[2+1]).c_str(), false);   // "CFG_Brenner_Mod_Laufzeit"       => "00a8:1"
      break;

    
    case 0x0100:
      #ifdef USE_HC1
      mqttPublish(addCfgTopic(cfgTopic.HC1_PROGRAM[LANG]), cfgArray.HC_PROGRAM[data[2]], false);     // "CFG_HK1_Programm"  => "0100:0"
      #endif
      break;
   
    case 0x0169:
      #ifdef USE_HC2
      mqttPublish(addCfgTopic(cfgTopic.HC2_PROGRAM[LANG]), cfgArray.HC_PROGRAM[data[2]], false);     // "CFG_HK2_Programm"  => "0169:0"
      #endif
      break;

    case 0x0107: // HK1_Timer01  
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP01: %s | SP02: %s | SP03: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER01[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x010e: // HK1_Timer02
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP04: %s | SP05: %s | SP06: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER02[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0115: // HK1_Timer03
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP07: %s | SP08: %s | SP09: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER03[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x011c: // HK1_Timer04
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP10: %s | SP11: %s | SP12: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER04[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0123: // HK1_Timer05
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP13: %s | SP14: %s | SP15: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER05[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x012a: // HK1_Timer06
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP16: %s | SP17: %s | SP18: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER06[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0131: // HK1_Timer07
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP19: %s | SP20: %s | SP21: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER07[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0138: // HK1_Timer08
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP22: %s | SP23: %s | SP24: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER08[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x013f: // HK1_Timer09
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP25: %s | SP26: %s | SP27: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER09[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0146: // HK1_Timer10
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP28: %s | SP29: %s | SP30: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER10[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x014d: // HK1_Timer11
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP31: %s | SP32: %s | SP33: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER11[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0154: // HK1_Timer12
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP34: %s | SP35: %s | SP36: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER12[LANG]), tmpMessage, false);
      #endif
      break;
 
    case 0x015b: // HK1_Timer13
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP37: %s | SP38: %s | SP39: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER13[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0162: // HK1_Timer14
      #ifdef USE_HC1
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP40: %s | SP41: %s | SP42: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC1_TIMER14[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0170: // HK2_Timer01  
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP01: %s | SP02: %s | SP03: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER01[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0177: // HK2_Timer02
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP04: %s | SP05: %s | SP06: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER02[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x017e: // HK2_Timer03
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP07: %s | SP08: %s | SP09: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER03[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0185: // HK2_Timer04
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP10: %s | SP11: %s | SP12: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER04[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x018c: // HK2_Timer05
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP13: %s | SP14: %s | SP15: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER05[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x0193: // HK2_Timer06
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP16: %s | SP17: %s | SP18: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER06[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x019a: // HK2_Timer07
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP19: %s | SP20: %s | SP21: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER07[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01a1: // HK2_Timer08
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP22: %s | SP23: %s | SP24: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER08[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01a8: // HK2_Timer09
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP25: %s | SP26: %s | SP27: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER09[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01af: // HK2_Timer10
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP28: %s | SP29: %s | SP30: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER10[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01b6: // HK2_Timer11
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP31: %s | SP32: %s | SP33: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER11[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01bd: // HK2_Timer12
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP34: %s | SP35: %s | SP36: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER12[LANG]), tmpMessage, false);
      #endif
      break;
 
    case 0x01c4: // HK2_Timer13
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP37: %s | SP38: %s | SP39: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER13[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01cb: // HK2_Timer14
      #ifdef USE_HC2
      decodeTimer(t1, data[2], data[3]);
      decodeTimer(t2, data[4], data[5]);
      decodeTimer(t3, data[6], data[7]);
      sprintf(tmpMessage,"SP40: %s | SP41: %s | SP42: %s",t1,t2,t3);
      mqttPublish(addCfgTopic(cfgTopic.HC2_TIMER14[LANG]), tmpMessage, false);
      #endif
      break;

    case 0x01e0: // 01e0:1,s Uhrzeit_Offset
        mqttPublish(addCfgTopic(cfgTopic.TIME_OFFSET[LANG]), String(decode05cTemp(decodeNegValue(data[2+1])) + String(" ") + String(mqttMsg.HOURS[LANG])).c_str(), false);   // "CFG_Uhrzeit_Offset"    => "01e0:1,s"
      break;


    case 0x0400: 
        // 04_00_07_01_81_8e_00_c1_ff_00_00_00
        // some kind of lifesign - ignore
      break;

    // undefined
    default:   
      #ifdef DEBUG_ON 
        sprintf(tmpMessage, "%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10]);
        mqttPublish(addTopic("/undefinded_message"), tmpMessage, false); 
      #endif                                                     
      break;
  }
 
  // write new values back if something has changed                           
  if(memcmp(&tmpState, &kmState, sizeof(s_km271_status))) {
    memcpy(&kmState, &tmpState, sizeof(s_km271_status)); 
  }
  xSemaphoreGive(accessMutex); 
}


/**
 * *******************************************************************
 * @brief   Decodes KM271 temperatures with 0.5 C resolution
 * @details The value is provided in 0.5 steps
 * @param   data: the receive dat byte
 * @return  the decoded value as float
 * *******************************************************************/
float decode05cTemp(uint8_t data) {
  return ((float)data) / 2.0f;
}


/**
 * *******************************************************************
 * @brief   Decodes KM271 values with negative range
 * @details Values >128 are negative
 * @param   data: the receive dat byte
 * @return  the decoded value as float
  * ********************************************************************/
float decodeNegValue(uint8_t data) {
  if(data > 128) {
        return (((float)(256-data)) * -1.0f);
  } else {
        return (float)data;
  }
}


/**
 * *******************************************************************
 * @brief   Retrieves the current status and copies it into
 *          the destination given.
 * @details This is under task lock to ensure consistency of status structure.
 * @param   pDestStatus: The destination address the status shall be stored to
 * @return  none
 * *******************************************************************/
void km271GetStatus(s_km271_status *pDestStatus) {
  if(accessMutex) {                                                       // Just in case the mutex is not initialzed when another task tries to use it
    xSemaphoreTake(accessMutex, portMAX_DELAY);                           // Prevent task switch to ensure the whole structure remains constistent
    memcpy(pDestStatus, &kmState, sizeof(s_km271_status));
    xSemaphoreGive(accessMutex);                                          // We may continue normally here
  }
}

/**
 * *******************************************************************
 * @brief   Initializes the KM271 protocoll (based on 3964 protocol)
 * @details 
 * @param   rxPin   The ESP32 RX pin number to be used for KM271 communication
 * @param   txPin   The ESP32 TX pin number to be used for KM271 communication
 * @return  e_ret error code
 * *******************************************************************/
e_ret km271ProtInit(int rxPin, int txPin) {
  Serial2.begin(KM271_BAUDRATE, SERIAL_8N1, rxPin, txPin);                // Set serial port for communication with KM271/Ecomatic 2000

  // Create the mutex to access the kmState structure in a safe manner
  accessMutex = xSemaphoreCreateMutex();

  return RET_OK;  
}

/**
 * *******************************************************************
 * @brief   Main Handling of KM271
 * @details receice serial data and call all other function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicKM271(){
  // >>>>>>>>> KM271 Main Handling >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  if(Serial2.readBytes(&rxByte, 1)) {                                   // Wait for RX byte, if timeout, just loop and read again
    // Protocol handling
    kmRxBcc ^= rxByte;                                                  // Calculate BCC
    switch(kmRxStatus) {
      case KM_RX_RESYNC:                                                // Unknown state, discard everthing but STX
        if(rxByte == KM_STX) {                                          // React on STX only to re-synchronise
          kmRxBuf.buf[0] = KM_STX;                                      // Store current STX
          kmRxBuf.len = 1;                                              // Set length
          kmRxStatus = KM_RX_IDLE;                                      // Sync done, now continue to receive
          handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte);              // Handle RX block
        }
        break;
      case KM_RX_IDLE:                                                  // Start of block or command
        kmRxBuf.buf[0] = rxByte;                                        // Store current byte
        kmRxBuf.len = 1;                                                // Initialise length
        kmRxBcc = rxByte;                                               // Reset BCC
        if((rxByte == KM_STX) || (rxByte == KM_DLE) || (rxByte == KM_NAK)) {    // Give STX, DLE, NAK directly to caller
          handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte);              // Handle RX block
        } else {                                                        // Whole block will follow
          kmRxStatus = KM_RX_ON;                                        // More data to follow, start collecting
        }
        break;                      
      case KM_RX_ON:                                                    // Block reception ongoing
        if(rxByte == KM_DLE) {                                          // Handle DLE doubling
          kmRxStatus = KM_RX_DLE;                                       // Discard first received DLE, could be doubling or end of block, check in next state
          break;                                                        // Quit here without storing
        }
        if(kmRxBuf.len >= KM_RX_BUF_LEN) {                              // Check allowed block len, if too long, re-sync
          kmRxStatus = KM_RX_RESYNC;                                    // Enter re-sync
          break;                                                        // Do not save data beyond array border
        }
        kmRxBuf.buf[kmRxBuf.len] = rxByte;                              // No DLE -> store regular, current byte
        kmRxBuf.len++;                                                  // Adjust length in rx buffer
        break;
      case KM_RX_DLE:                                                   // Entered when one DLE was already received
        if(rxByte == KM_DLE) {                                          // Double DLE?
          if(kmRxBuf.len >= KM_RX_BUF_LEN) {                            // Check allowed block len, if too long, re-sync
            kmRxStatus = KM_RX_RESYNC;                                  // Enter re-sync
            break;                                                      // Do not save data beyond array border
          }
          kmRxBuf.buf[kmRxBuf.len] = rxByte;                            // Yes -> store this DLE as valid part of data
          kmRxBuf.len++;                                                // Adjust length in rx buffer
          kmRxStatus = KM_RX_ON;                                        // Continue to receive block
        } else {                                                        // This should be ETX now
          if(rxByte == KM_ETX) {                                        // Really? then we are done, just waiting for BCC
            kmRxStatus = KM_RX_BCC;                                     // Receive BCC and verify it
          } else {
            kmRxStatus = KM_RX_RESYNC;                                  // Something wrong, just try to restart 
          }
        }
        break;
      case KM_RX_BCC:                                                   // Last stage, BCC verification, "received BCC" ^ "calculated BCC" shall be 0 
        if(!kmRxBcc) {                                                  // Block is valid
          handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte);              // Handle RX block, provide BCC for debug logging, too
        } else {
          sendTxBlock(KmCNAK, sizeof(KmCNAK));                          // Send NAK, ask for re-sending the block
        }
        kmRxStatus = KM_RX_IDLE;                                        // Wait for next data or re-sent block
        break;    
    } // end-case
  }  // end-if
  
  // global status logmode active
  km271LogModeActive = (KmRxBlockState == KM_TSK_LOGGING);
}

/**
 *  *************************************************************************************************************
 * @brief   Handling of a whole RX block received by the RX task.
 * @details A whole block of RX data is processed according to the current
 *          operating state.
 *          The operating state ensures, that we enable the logging feature of the
 *          KM271. During logging, the KM271 constantlöy updates all information
 *          which has changed automatically.
 * @param   data: Pointer to the block of data received.
 * @param   len:  Blocksize in number of bytes, without protocol data 
 * @param   bcc:  The BCC byte for a regular data block. Only valid for data blocks. Used for debugging only
 * @return  none
 * **************************************************************************************************************/
void handleRxBlock(uint8_t *data, int len, uint8_t bcc) {
  switch(KmRxBlockState) {
    case KM_TSK_START:                                                      // We need to switch to logging mode, first
      switch(data[0]) {
        case KM_STX:                                                        // First step: wait for STX
          sendTxBlock(KmCSTX, sizeof(KmCSTX));                              // Send STX to KM271
          break;
        case KM_DLE:                                                        // DLE received, KM ready to receive command
            sendTxBlock(KmCLogMode, sizeof(KmCLogMode));                    // Send logging command
            KmRxBlockState = KM_TSK_LG_CMD;                                 // Switch to check for logging mode state
          break;
      }
      break;
    case KM_TSK_LG_CMD:                                                     // Check if logging mode is accepted by KM
      if(data[0] != KM_DLE) {                                               // No DLE, not accepted, try again
        KmRxBlockState = KM_TSK_START;                                      // Back to START state
      } else {
        KmRxBlockState = KM_TSK_LOGGING;                                    // Command accepted, ready to log!
      }
      break;
    case KM_TSK_LOGGING:                                                    // We have reached logging state
      if(data[0] == KM_STX) {                                               // If STX, this is a send request
        if (send_buf[0] != 0){                                            // If a send-request is active, 
          sendTxBlock(KmCSTX, sizeof(KmCSTX));                              // send STX to KM271 to request for send data
        }
        else {
          sendTxBlock(KmCDLE, sizeof(KmCDLE));                              // Confirm handling of block by sending DLE
        }
      } else if(data[0] == KM_DLE) {                                        // KM271 is ready to receive
          sendTxBlock(send_buf, sizeof(send_buf));                      // send buffer 
          memset(send_buf, 0, sizeof(send_buf));                        // clear buffer
          KmRxBlockState = KM_TSK_START;                                    // start log-mode again, to get all new values
      } else {                                                              // If not STX, it should be valid data block
        parseInfo(data, len);                                               // Handle data block with event information
        sendTxBlock(KmCDLE, sizeof(KmCDLE));                                // Confirm handling of block by sending DLE
      }
      break;
  }
}

/**
 * *******************************************************************
 * @brief   build info structure ans send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendKM271Info(){
  DynamicJsonDocument infoJSON(1024);
  infoJSON[0]["logmode"] = km271LogModeActive;
  infoJSON[0]["send_cmd_busy"] = (send_buf[0]!=0);
  infoJSON[0]["date-time"] = getDateTimeString();
  String sendInfoJSON;
  serializeJson(infoJSON, sendInfoJSON);
  mqttPublish(addTopic("/info"),String(sendInfoJSON).c_str(), false);
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt CONFIG topic
 * @param   suffix that shoould be add to the static part of the topic
 * @return  pointer to result topic string
 * *******************************************************************/
char newCfgTopic[256];
const char * addCfgTopic(const char *suffix){
  strcpy(newCfgTopic, MQTT_TOPIC);
  strcat(newCfgTopic, "/config/");
  strcat(newCfgTopic, suffix);
  return newCfgTopic;
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt STATUS topic
 * @param   suffix that shoould be add to the static part of the topic
 * @return  pointer to result topic string
 * *******************************************************************/
char newStatTopic[256];
const char * addStatTopic(const char *suffix){
  strcpy(newStatTopic, MQTT_TOPIC);
  strcat(newStatTopic, "/status/");
  strcat(newStatTopic, suffix);
  return newStatTopic;
}

/**
 * *******************************************************************
 * @brief   helper function to add subject to mqtt ERROR topic
 * @param   suffix that shoould be add to the static part of the topic
 * @return  pointer to result topic string
 * *******************************************************************/
char newAlarmTopic[256];
const char * addAlarmTopic(const char *suffix){
  strcpy(newAlarmTopic, MQTT_TOPIC);
  strcat(newAlarmTopic, "/alarm/");
  strcat(newAlarmTopic, suffix);
  return newAlarmTopic;
}

/**
 * *******************************************************************
 * @brief   set actual date and time to buderus
 * @param   dti: date and time info structure
 * @return  none
 * *******************************************************************/
void km271SetDateTime(){
  char dateTimeInfo[128]={'\0'};              // Date and time info String
  time_t now;                                 // this is the epoch
  tm dti;                                     // the structure tm holds time information in a more convient way
  time(&now);                                 // read the current time
  localtime_r(&now, &dti);                    // update the structure tm with the current time
  send_buf[0]= 0x01;                          // address
  send_buf[1]= 0x00;                          // address
  send_buf[2]= dti.tm_sec;                    // seconds
  send_buf[3]= dti.tm_min;                    // minutes
  send_buf[4]= dti.tm_hour;                   // hours (bit 0-4)
  if (dti.tm_isdst>0)
    send_buf[4] |= (1 << 6) & 0x40;           // if time ist DST  (bit 6) 
  send_buf[5]= dti.tm_mday;                   // day of month
  send_buf[6]= dti.tm_mon;                    // month
  send_buf[6]|= (dti.tm_wday << 4) & 0x70;    // day of week (0=monday...6=sunday)
  send_buf[7]= dti.tm_year-1900;              // year 
  
  sprintf(dateTimeInfo, "%s: %d.%d.%d - %02i:%02i:%02i - DST:%d", mqttMsg.DATETIME_CHANGED, dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec, (dti.tm_isdst>0));
  mqttPublish(addTopic("/message"), dateTimeInfo, false);
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: parameter as integer
 * @return  none
 * *******************************************************************/
void km271sendCmd(e_km271_sendCmd sendCmd, int8_t cmdPara){

  switch (sendCmd)
  {
  case KM271_SENDCMD_HC1_OPMODE:
    if (cmdPara>=0 && cmdPara<=2){
      send_buf[0]= 0x07;        // Data-Type für HK1 (0x07) 
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= cmdPara;     // 0:Night | 1:Day | 2:AUTO
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_OPMODE_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_OPMODE_INVALID[LANG], false);
    }
    break;
    
    case KM271_SENDCMD_HC2_OPMODE:
    if (cmdPara>=0 && cmdPara<=2){      
      send_buf[0]= 0x08;        // Data-Type für HK2 (0x08) 
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= cmdPara;     // 0:Night | 1:Day | 2:AUTO
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_OPMODE_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_OPMODE_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_DESIGN_TEMP:
    if (cmdPara>=30 && cmdPara<=90){    
      send_buf[0]= 0x07;        // Data-Type für HK1 (0x07)
      send_buf[1]= 0x0E;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= cmdPara;     // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_INTERPRET_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_INTERPRET_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_DESIGN_TEMP:
    if (cmdPara>=30 && cmdPara<=90){    
      send_buf[0]= 0x08;        // Data-Type für HK2 (0x08)
      send_buf[1]= 0x0E;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= cmdPara;     // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_INTERPRET_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_INTERPRET_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_PROGRAMM:
    if (cmdPara>=0 && cmdPara<=8){     
      send_buf[0]= 0x11;        // Data-Type
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= cmdPara;     // Programmnummer 0..8
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= 0x65; 
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_PROG_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_PROG_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_PROGRAMM:
    if (cmdPara>=0 && cmdPara<=8){     
      send_buf[0]= 0x12;        // Data-Type
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= cmdPara;     // Programmnummer 0..8
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= 0x65; 
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_PROG_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_PROG_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_WW_OPMODE:
    if (cmdPara>=0 && cmdPara<=2){     
      send_buf[0]= 0x0C;      // Data-Type für Warmwasser (0x0C)
      send_buf[1]= 0x0E;      // Offset
      send_buf[2]= cmdPara;   // 0:Night | 1:Day | 2:AUTO
      send_buf[3]= 0x65; 
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65; 
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.WW_OPMODE_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.WW_OPMODE_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_SUMMER:
    if (cmdPara>=9 && cmdPara<=31){     
      send_buf[0]= 0x07;      // Data-Type für HK1 (0x07) 
      send_buf[1]= 0x00;      // Offset 
      send_buf[2]= 0x65;
      send_buf[3]= cmdPara;   // 9:Winter | 10°-30° | 31:Summer
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.SUMMER_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.SUMMER_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_FROST:
    if (cmdPara>=-20 && cmdPara<=10){    
      send_buf[0]= 0x07;      // Data-Type für HK1 (0x07) 
      send_buf[1]= 0x31;      // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;      
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= (cmdPara>0) ? cmdPara:cmdPara+256;    // -20° ... +10° (add 256 if value is negative)
      mqttPublish(addTopic("/message"), mqttMsg.FROST_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.FROST_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD:
    if (cmdPara>=-20 && cmdPara<=10){     
      send_buf[0]= 0x07;      // Data-Type für HK1 (0x07)  
      send_buf[1]= 0x15;      // Offset 
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;       
      send_buf[4]= (cmdPara>0) ? cmdPara:cmdPara+256;    // -20° ... +10° (add 256 if value is negative)
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_SWITCH_OFF_THRESHOLD_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_SWITCH_OFF_THRESHOLD_INVALID[LANG], false);
    }
    break;

 case KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD:
    if (cmdPara>=-20 && cmdPara<=10){     
      send_buf[0]= 0x08;      // Data-Type für HK2 (0x08)  
      send_buf[1]= 0x15;      // Offset 
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;       
      send_buf[4]= (cmdPara>0) ? cmdPara:cmdPara+256;    // -20° ... +10° (add 256 if value is negative)
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_SWITCH_OFF_THRESHOLD_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_SWITCH_OFF_THRESHOLD_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_WW_SETPOINT:
    if (cmdPara>=30 && cmdPara<=60){    
      send_buf[0]= 0x0C;        // Data-Type für Warmwater (0x0C) 
      send_buf[1]= 0x07;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;       
      send_buf[4]= 0x65;
      send_buf[5]= cmdPara;     // 30°-60°
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.WW_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.WW_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_HOLIDAYS:
    if (cmdPara>=0 && cmdPara<=99){    
      send_buf[0]= 0x11;        // Data-Type HK1
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= cmdPara;     // Resolution: 1 Day - Range: 0 – 99 Days 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;    
      mqttPublish(addTopic("/message"), mqttMsg.HC1_HOLIDAYS_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_HOLIDAYS_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_HOLIDAYS:
    if (cmdPara>=0 && cmdPara<=99){    
      send_buf[0]= 0x12;        // Data-Type HK2
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= cmdPara;     // Resolution: 1 Day - Range: 0 – 99 Days 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;    
      mqttPublish(addTopic("/message"), mqttMsg.HC2_HOLIDAYS_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_HOLIDAYS_INVALID[LANG], false);
    }
    break;

  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: parameter as float
 * @return  none
 * *******************************************************************/
void km271sendCmdFlt(e_km271_sendCmd sendCmd, float cmdPara){

  switch (sendCmd)
  {
  case KM271_SENDCMD_HC1_DAY_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){      
      send_buf[0]= 0x07;        // Data-Type für HK1 (0x07)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_DAY_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_DAY_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_NIGHT_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){    
      send_buf[0]= 0x07;        // Data-Type für HK1 (0x07)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      send_buf[5]= 0x65;
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC1_NIGHT_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_NIGHT_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_DAY_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){     
      send_buf[0]= 0x08;        // Data-Type für HK2 (0x08)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_DAY_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_DAY_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_NIGHT_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){     
      send_buf[0]= 0x08;        // Data-Type für HK2 (0x08)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      send_buf[5]= 0x65;
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), mqttMsg.HC2_NIGHT_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_NIGHT_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC1_HOLIDAY_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){    
      send_buf[0]= 0x07;        // Data-Type für HK1 (0x07)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;
      send_buf[6]= 0x65;
      send_buf[7]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      mqttPublish(addTopic("/message"), mqttMsg.HC1_HOLIDAY_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC1_HOLIDAY_SETPOINT_INVALID[LANG], false);
    }
    break;

  case KM271_SENDCMD_HC2_HOLIDAY_SETPOINT:
    if (cmdPara>=10 && cmdPara<=30){    
      send_buf[0]= 0x08;        // Data-Type für HK1 (0x08)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;
      send_buf[6]= 0x65;
      send_buf[7]= trunc(2.0 * cmdPara + 0.5);     // Resolution: 0.5 °C - Range: 10 – 30 °C 
      mqttPublish(addTopic("/message"), mqttMsg.HC2_HOLIDAY_SETPOINT_RECV[LANG], false);
    } else {
      mqttPublish(addTopic("/message"), mqttMsg.HC2_HOLIDAY_SETPOINT_INVALID[LANG], false);
    }
    break;

  default:
    break;
  }
}


/**
 * *******************************************************************
 * @brief   returns the status if LogMode is active
 * @param   none
 * @return  bool state of LogModeActive
 * *******************************************************************/
bool km271GetLogMode(){
  return km271LogModeActive;
}

/**
 * *******************************************************************
 * @brief   decode the timer values
 * @param   
 * @return  string with timer information
 * *******************************************************************/
void decodeTimer(char * timerInfo, uint8_t dateOnOff, uint8_t time){
  
/*
  dateOnOff: first part contains day
  0x00  => "Mo",
  0x20  => "Di",
  0x40  => "Mi",
  0x60  => "Do",
  0x80  => "Fr",
  0xa0  => "Sa",
  0xc0  => "So"
  
  second part contains on/off
  example:
  0x21 => Di + ON / 0x20 => Di + OFF
  0x61 => Do + ON / 0x60 => Do + OFF
  --------
  time:
  The time has a resolution of 10min 
  05:30: => 33 (0x21)
  Calculation: 5 x 6 + (30 / 10) = 33
  -------
  return example: "Di 05:30 (An) | Di 22:00 (Aus)"
  */
  
  bool onOff = dateOnOff & 0x0f;    // extract OnOff State
  uint8_t day = dateOnOff & 0xf0;   // extract day
  char timeString[100]={'\0'};
  char dayString[10]={'\0'};
  char onOffString[10]={'\0'};

  if (!(dateOnOff==0xc2 && time == 0x90)){
    switch (day)
    {
      case 0x00: sprintf(dayString, "%s", mqttMsg.MON[LANG]); break;
      case 0x20: sprintf(dayString, "%s", mqttMsg.TUE[LANG]); break;
      case 0x40: sprintf(dayString, "%s", mqttMsg.WED[LANG]); break;
      case 0x60: sprintf(dayString, "%s", mqttMsg.THU[LANG]); break;
      case 0x80: sprintf(dayString, "%s", mqttMsg.FRI[LANG]); break;
      case 0xa0: sprintf(dayString, "%s", mqttMsg.SAT[LANG]); break;
      case 0xc0: sprintf(dayString, "%s", mqttMsg.SUN[LANG]); break;
      default: strncpy(dayString, "--", sizeof(dayString)); break;
    }
    // add switch state
    if (onOff){
      sprintf(onOffString, " (%s) ", mqttMsg.ON[LANG]);
    }
    else{
      sprintf(onOffString, " (%s) ", mqttMsg.OFF[LANG]);
    }
    // convert time data
    sprintf(timeString, "%02d:%02d", (time / 6), (time % 6)*10);
    // copy to output parameter
    sprintf(timerInfo, "%s %s %s",dayString, timeString, onOffString);
  }
  else {
    // not used
    strncpy(timerInfo, "frei", sizeof(timerInfo));
  }
}

/**
 * *******************************************************************
 * @brief   get the index of the error message for the given error number
 * @param   errorNr Error Number from Logamatic
 * @return  index for error message in errMsgText
 * *******************************************************************/
uint8_t getErrorTextIndex(uint8_t errorNr){
   switch (errorNr)
   {
    case   0: return 0; break;
    case   2: return 1; break;
    case   3: return 2; break;
    case   4: return 3; break;
    case   8: return 4; break;
    case   9: return 5; break;
    case  10: return 6; break;
    case  11: return 7; break;
    case  12: return 8; break;
    case  15: return 9; break;
    case  16: return 10; break;
    case  20: return 11; break;
    case  24: return 12; break;
    case  30: return 13; break;
    case  31: return 14; break;
    case  32: return 15; break;
    case  33: return 16; break;
    case  49: return 17; break;
    case  50: return 18; break;
    case  51: return 19; break;
    case  52: return 20; break;
    case  53: return 21; break;
    case  54: return 22; break;
    case  55: return 23; break;
    case  56: return 24; break;
    case  87: return 25; break;
    case  92: return 26; break;
    default: return 27; break;
   }
}

/**
 * *******************************************************************
 * @brief   generate error message 
 * @param   errorNr Error Number from Logamatic
 * @return  index for error message in errMsgText
 * *******************************************************************/
void decodeErrorMsg(char * errorMsg, uint8_t *data){
  // no error
  if(data[2]==0){
    sprintf(errorMsg, "%s", errMsgText.idx[0]);
  }
  else { 
    // error already acknowledged
    if(data[6]!=0xFF) {
        // Aussenfuehler defekt (>> 16:31 -3 Tage | << 20:40 -2 Tage)
        sprintf(errorMsg, "%s (>> %02i:%02i -%i %s | << %02i:%02i -%i %s)", errMsgText.idx[getErrorTextIndex(data[2])], data[3], data[4], (data[5]+data[8]), mqttMsg.DAYS[LANG], data[6], data[7], data[8], mqttMsg.DAYS[LANG]);
    }
    // unacknowledged error 
    else {
        // example: Aussenfuehler defekt (>> 16:31 -3 Tage)
        sprintf(errorMsg, "%s (>> %02i:%02i -%i %s)", errMsgText.idx[getErrorTextIndex(data[2])], data[3], data[4], data[5], mqttMsg.DAYS[LANG]);
    }
  }
  
}