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
uint8_t     rxByte;                                // The received character. We are reading byte by byte only.
e_rxState   kmRxStatus = KM_RX_RESYNC;             // Status in Rx reception
uint8_t     kmRxBcc  = 0;                          // BCC value for Rx Block
KmRx_s      kmRxBuf;                               // Rx block storag
bool        send_request;
uint8_t     send_cmd;
uint8_t     send_buf[8] = {};
bool        km271LogModeActive = false;

// ==================================================================================================
// Message arrays for config messages
// ==================================================================================================
String cfgBetriebsart[]={"Nacht", "Tag", "Automatik"};
String cfgAnzeige[]={"Automatik", "Kessel", "Warmwasser", "Aussen"};
String cfgSprache[]={"DE", "FR", "IT", "NL", "EN", "PL"};
String cfgAbsenkungsart[]={"Abschalt", "Reduziert", "Raumhalt", "Aussenhalt"};
String cfgSommerAb[]={"Sommer","10 °C","11 °C","12 °C","13 °C","14 °C","15 °C","16 °C","17 °C","18 °C","19 °C","20 °C","21 °C","22 °C","23 °C","24 °C","25 °C","26 °C","27 °C","28 °C","29 °C","30 °C","Winter"};
String cfgAufschalttemperatur[]={"Aus","1","2","3","4","5","6","7","8","9","10"};
String cfgHeizsystem[]={"Aus","Heizkoerper","-","Fussboden"};
String cfgAnAus[]={"Aus","An"};
String cfgGebaude[]={"Leicht","Mittel","Schwer"};
String cfgZirkulation[]={"Aus","1","2","3","4","5","6","An"};
String cfgBrennerart[]={"1-stufig","2-stufig","Modulierend"};
String cfgAbgasTempSchwelle[]={"Aus","50","55","60","65","70","75","80","85","90","95","100","105","110","115","120","125","130","135","140","145","150","155","160","165","170","175","180","185","190","195","200","205","210","215","220","225","230","235","240","245","250"};
String cfgHk1Programm[]={"Eigen","Familie","Frueh","Spaet","Vormittag","Nachmittag","Mittag","Single","Senior"};
//********************************************************************************************


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
  s_km271_status        tmpState;
  
  // Get current state
  xSemaphoreTake(accessMutex, portMAX_DELAY);                             // Prevent task switch to ensure the whole structure remains constistent
  memcpy(&tmpState, &kmState, sizeof(s_km271_status));
  xSemaphoreGive(accessMutex);
  uint kmregister = (data[0] * 256) + data[1];
  switch(kmregister) {                                     // Check if we can find known stati
    
    /*
    *******************************************************
    * status values
    *******************************************************
    */
    case 0x8000:                                                          
      tmpState.HeatingCircuitOperatingStates_1 = data[2];                 // 0x8000 : Bitfield 
      mqttPublish(addTopic("/status/HK1_BW1_Ausschaltoptimierung"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 0)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Einschaltoptimierung"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 1)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Automatik"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 2)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Warmwasservorrang"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 3)).c_str(), false);        
      mqttPublish(addTopic("/status/HK1_BW1_Estrichtrocknung"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 4)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Ferien"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 5)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Frostschutz"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 6)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW1_Manuell"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 7)).c_str(), false);   
      break;
      
    case 0x8001:                                                          
      tmpState.HeatingCircuitOperatingStates_2 = data[2];                 // 0x8001 : Bitfield
      mqttPublish(addTopic("/status/HK1_BW2_Sommer"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 0)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW2_Tag"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 1)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW2_Keine_Komm_mit_FB"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 2)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW2_FB_fehlerhaft"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 3)).c_str(), false); 
      mqttPublish(addTopic("/status/HK1_BW2_Fehler_Vorlauffuehler"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 4)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW2_Maximaler_Vorlauf"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 5)).c_str(), false);
      mqttPublish(addTopic("/status/HK1_BW2_Externer_Stoereingang"), String(bitRead(tmpState.HeatingCircuitOperatingStates_2, 6)).c_str(), false);        
      break;
      
    case 0x8002:
      tmpState.HeatingForwardTargetTemp = (float)data[2];                 // 0x8002 : Temperature (1C resolution)
      mqttPublish(addTopic("/status/HK1_Vorlaufsolltemperatur"), String(tmpState.HeatingForwardTargetTemp).c_str(), false);  
      break;
      
    case 0x8003:
      tmpState.HeatingForwardActualTemp = (float)data[2];                 // 0x8003 : Temperature (1C resolution)
      mqttPublish(addTopic("/status/HK1_Vorlaufisttemperatur"), String(tmpState.HeatingForwardActualTemp).c_str(), false);
      break;
      
    case 0x8004:
      tmpState.RoomTargetTemp = decode05cTemp(data[2]);                   // 0x8004 : Temperature (0.5C resolution)
      mqttPublish(addTopic("/status/HK1_Raumsolltemperatur"), String(tmpState.RoomTargetTemp).c_str(), false);
      break;
      
    case 0x8005:
      tmpState.RoomActualTemp = decode05cTemp(data[2]);                   // 0x8005 : Temperature (0.5C resolution)
      mqttPublish(addTopic("/status/HK1_Raumisttemperatur"), String(tmpState.RoomActualTemp).c_str(), false);
      break;
      
    case 0x8006:
      tmpState.SwitchOnOptimizationTime = data[2];                        // 0x8006 : Minutes
      mqttPublish(addTopic("/status/HK1_Einschaltoptimierung"), String(tmpState.SwitchOnOptimizationTime).c_str(), false);
      break;
      
    case 0x8007:  
      tmpState.SwitchOffOptimizationTime = data[2];                       // 0x8007 : Minutes
      mqttPublish(addTopic("/status/HK1_Ausschaltoptimierung"), String(tmpState.SwitchOffOptimizationTime).c_str(), false);
      break;
      
    case 0x8008:  
      tmpState.PumpPower = data[2];                                       // 0x8008 : Percent
      mqttPublish(addTopic("/status/HK1_Pumpe"), String(tmpState.PumpPower).c_str(), false);
      break;
      
    case 0x8009:
      tmpState.MixingValue = data[2];                                     // 0x8009 : Percent
      mqttPublish(addTopic("/status/HK1_Mischerstellung"), String(tmpState.MixingValue).c_str(), false);
      break;
      
    case 0x800c:
      tmpState.HeatingCurvePlus10 = (float)data[2];                       // 0x800c : Temperature (1C resolution)
      mqttPublish(addTopic("/status/HK1_Heizkennlinie_10_Grad"), String(tmpState.HeatingCurvePlus10).c_str(), false);
      break;
      
    case 0x800d:
      tmpState.HeatingCurve0 = (float)data[2];                            // 0x800d : Temperature (1C resolution)
      mqttPublish(addTopic("/status/HK1_Heizkennlinie_0_Grad"), String(tmpState.HeatingCurve0).c_str(), false);
      break;
      
    case 0x800e:
      tmpState.HeatingCurveMinus10 = (float)data[2];                      // 0x800e : Temperature (1C resolution)
      mqttPublish(addTopic("/status/HK1_Heizkennlinie_-10_Grad"), String(tmpState.HeatingCurveMinus10).c_str(), false);
      break;
      
    case 0x8424:
      tmpState.HotWaterOperatingStates_1 = data[2];                       // 0x8424 : Bitfield
      mqttPublish(addTopic("/status/WW_BW1_Automatik"), String(bitRead(tmpState.HotWaterOperatingStates_1, 0)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Desinfektion"), String(bitRead(tmpState.HotWaterOperatingStates_1, 1)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Nachladung"), String(bitRead(tmpState.HotWaterOperatingStates_1, 2)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Ferien"), String(bitRead(tmpState.HotWaterOperatingStates_1, 3)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Fehler_Desinfektion"), String(bitRead(tmpState.HotWaterOperatingStates_1, 4)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Fehler_Fuehler"), String(bitRead(tmpState.HotWaterOperatingStates_1, 5)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW1_Fehler_WW_bleibt_kalt"), String(bitRead(tmpState.HotWaterOperatingStates_1, 6)).c_str(), false);        
      mqttPublish(addTopic("/status/WW_BW1_Fehler_Anode"), String(bitRead(tmpState.HotWaterOperatingStates_1, 7)).c_str(), false);   
      break;
      
    case 0x8425:
      tmpState.HotWaterOperatingStates_2 = data[2];                       // 0x8425 : Bitfield
      mqttPublish(addTopic("/status/WW_BW2_Laden"), String(bitRead(tmpState.HeatingCircuitOperatingStates_1, 0)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Manuell"), String(bitRead(tmpState.HotWaterOperatingStates_2, 1)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Nachladen"), String(bitRead(tmpState.HotWaterOperatingStates_2, 2)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Ausschaltoptimierung"), String(bitRead(tmpState.HotWaterOperatingStates_2, 3)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Einschaltoptimierung"), String(bitRead(tmpState.HotWaterOperatingStates_2, 4)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Tag"), String(bitRead(tmpState.HotWaterOperatingStates_2, 5)).c_str(), false);
      mqttPublish(addTopic("/status/WW_BW2_Warm"), String(bitRead(tmpState.HotWaterOperatingStates_2, 6)).c_str(), false);        
      mqttPublish(addTopic("/status/WW_BW2_Vorrang"), String(bitRead(tmpState.HotWaterOperatingStates_2, 7)).c_str(), false);   
      break;
      
    case 0x8426:
      tmpState.HotWaterTargetTemp = (float)data[2];                       // 0x8426 : Temperature (1C resolution)
      mqttPublish(addTopic("/status/WW_Solltemperatur"), String(tmpState.HotWaterTargetTemp).c_str(), false);
      break;
      
    case 0x8427:
      tmpState.HotWaterActualTemp = (float)data[2];                       // 0x8427 : Temperature (1C resolution)
      mqttPublish(addTopic("/status/WW_Isttemperatur"), String(tmpState.HotWaterActualTemp).c_str(), false);
      break;
      
    case 0x8428:
      tmpState.HotWaterOptimizationTime = data[2];                        // 0x8428 : Minutes
      mqttPublish(addTopic("/status/WW_Einschaltoptimierung"), String(tmpState.HotWaterOptimizationTime).c_str(), false);
      break;
      
    case 0x8429:
      tmpState.HotWaterPumpStates = data[2];                              // 0x8429 : Bitfield
      mqttPublish(addTopic("/status/WW_Pumpentyp_Ladepumpe"), String(bitRead(tmpState.HotWaterPumpStates, 0)).c_str(), false);
      mqttPublish(addTopic("/status/WW_Pumpentyp_Zirkulationspumpe"), String(bitRead(tmpState.HotWaterPumpStates, 1)).c_str(), false);
      mqttPublish(addTopic("/status/WW_Pumpentyp_Absenkung_Solar"), String(bitRead(tmpState.HotWaterPumpStates, 2)).c_str(), false);
      break;
      
    case 0x882a:
      tmpState.BoilerForwardTargetTemp  = (float)data[2];                 // 0x882a : Temperature (1C resolution)
      mqttPublish(addTopic("/status/Kessel_Vorlaufsolltemperatur"), String(tmpState.BoilerForwardTargetTemp).c_str(), false);
      break;
      
    case 0x882b:
      tmpState.BoilerForwardActualTemp  = (float)data[2];                 // 0x882b : Temperature (1C resolution)
      mqttPublish(addTopic("/status/Kessel_Vorlaufisttemperatur"), String(tmpState.BoilerForwardActualTemp).c_str(), false);
      break;
      
    case 0x882c:
      tmpState.BurnerSwitchOnTemp  = (float)data[2];                      // 0x882c : Temperature (1C resolution)
      mqttPublish(addTopic("/status/Brenner_Einschalttemperatur"), String(tmpState.BurnerSwitchOnTemp).c_str(), false);
      break;
      
    case 0x882d:
      tmpState.BurnerSwitchOffTemp  = (float)data[2];                     // 0x882d : Temperature (1C resolution)
      mqttPublish(addTopic("/status/Brenner_Ausschalttemperatur"), String(tmpState.BurnerSwitchOffTemp).c_str(), false);
      break;
      
    case 0x882e:
      tmpState.BoilerIntegral_1 = data[2];                                // 0x882e : Number (*256)
      // do nothing - useless value
      break;
      
    case 0x882f:
      tmpState.BoilerIntegral_2  = data[2];                               // 0x882f : Number (*1)
      // do nothing - useless value
      break;
      
    case 0x8830:
      tmpState.BoilerErrorStates  = data[2];                              // 0x8830 : Bitfield
      mqttPublish(addTopic("/status/Kessel_Fehler_Brennerstoerung"), String(bitRead(tmpState.BoilerErrorStates, 0)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Kesselfuehler"), String(bitRead(tmpState.BoilerErrorStates, 1)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Zusatzfuehler"), String(bitRead(tmpState.BoilerErrorStates, 2)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Kessel_bleibt_kalt"), String(bitRead(tmpState.BoilerErrorStates, 3)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Abgasfuehler"), String(bitRead(tmpState.BoilerErrorStates, 4)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Abgas_ueber_Grenzwert"), String(bitRead(tmpState.BoilerErrorStates, 5)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Sicherungskette_ausgeloest"), String(bitRead(tmpState.BoilerErrorStates, 6)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Fehler_Externe_Stoerung"), String(bitRead(tmpState.BoilerErrorStates, 7)).c_str(), false);
      break;
      
    case 0x8831:
      tmpState.BoilerOperatingStates = data[2];                           // 0x8831 : Bitfield
      mqttPublish(addTopic("/status/Kessel_Betrieb_Abgastest"), String(bitRead(tmpState.BoilerOperatingStates, 0)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_Betrieb_Stufe1"), String(bitRead(tmpState.BoilerOperatingStates, 1)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_Kesselschutz"), String(bitRead(tmpState.BoilerOperatingStates, 2)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_Unter_Betrieb"), String(bitRead(tmpState.BoilerOperatingStates, 3)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_Leistung_frei"), String(bitRead(tmpState.BoilerOperatingStates, 4)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_Leistung_hoch"), String(bitRead(tmpState.BoilerOperatingStates, 5)).c_str(), false);
      mqttPublish(addTopic("/status/Kessel_Betrieb_BetriebStufe2"), String(bitRead(tmpState.BoilerOperatingStates, 6)).c_str(), false);
      break;
      
    case 0x8832:
      // [ "Kessel aus"), "1.Stufe an"), "-"), "-"), "2.Stufe an bzw. Modulation frei" ]
      tmpState.BurnerStates = data[2];                                    // 0x8832 : Bitfield
      mqttPublish(addTopic("/status/Brenner_Ansteuerung"), String(tmpState.BurnerStates).c_str(), false);
      break;
      
    case 0x8833:
      tmpState.ExhaustTemp = (float)data[2];                              // 0x8833 : Temperature (1C resolution)
      mqttPublish(addTopic("/status/Abgastemperatur"), String(tmpState.ExhaustTemp).c_str(), false);
      break;
      
    case 0x8836:
      tmpState.BurnerOperatingDuration_2 = data[2];                       // 0x8836 : Minutes (*65536)
      mqttPublish(addTopic("/status/Brenner_Laufzeit_Minuten65536"), String(tmpState.BurnerOperatingDuration_2).c_str(), false);
      break;
      
    case 0x8837:
      tmpState.BurnerOperatingDuration_1 = data[2];                       // 0x8837 : Minutes (*256)
      mqttPublish(addTopic("/status/Brenner_Laufzeit_Minuten256"), String(tmpState.BurnerOperatingDuration_1).c_str(), false);
      break;
      
    case 0x8838:
      tmpState.BurnerOperatingDuration_0 = data[2];                       // 0x8838 : Minutes (*1)
      mqttPublish(addTopic("/status/Brenner_Laufzeit_Minuten"), String(tmpState.BurnerOperatingDuration_0).c_str(), false);
      break;
      
    case 0x893c:
      tmpState.OutsideTemp = decodeNegTemp(data[2]);                      // 0x893c : Temperature (1C resolution, possibly negative)
      mqttPublish(addTopic("/status/Aussentemperatur"), String(tmpState.OutsideTemp).c_str(), false);
      break;
      
    case 0x893d:
      tmpState.OutsideDampedTemp = decodeNegTemp(data[2]);                // 0x893d : Temperature (1C resolution, possibly negative)
      mqttPublish(addTopic("/status/Aussentemperatur_gedaempft"), String(tmpState.OutsideDampedTemp).c_str(), false);
      break;
      
    case 0x893e:
      tmpState.ControllerVersionMain = data[2];                           // 0x893e : Number
      mqttPublish(addTopic("/status/Versionsnummer_VK"), String(tmpState.ControllerVersionMain).c_str(), false);
      break;
      
    case 0x893f:
      tmpState.ControllerVersionSub = data[2];                            // 0x893f : Number
      mqttPublish(addTopic("/status/Versionsnummer_NK"), String(tmpState.ControllerVersionSub).c_str(), false);
      break;
      
    case 0x8940:
      tmpState.Modul = data[2];                                           // 0x8940 : Number
      mqttPublish(addTopic("/status/Modulkennung"), String(tmpState.Modul).c_str(), false);
      break;
    
    case 0xaa42:
      tmpState.ERR_Alarmstatus = data[2];                                // 0xaa42 : Bitfeld
      mqttPublish(addTopic("/status/ERR_Alarmstatus_Abgasfuehler"), String(bitRead(tmpState.ERR_Alarmstatus, 0)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_02"), String(bitRead(tmpState.ERR_Alarmstatus, 1)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_Kesselvorlauffuehler"), String(bitRead(tmpState.ERR_Alarmstatus, 2)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_08"), String(bitRead(tmpState.ERR_Alarmstatus, 3)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_Brenner"), String(bitRead(tmpState.ERR_Alarmstatus, 4)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_20"), String(bitRead(tmpState.ERR_Alarmstatus, 5)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_HK2-Vorlauffuehler"), String(bitRead(tmpState.ERR_Alarmstatus, 6)).c_str(), false);
      mqttPublish(addTopic("/status/ERR_Alarmstatus_80"), String(bitRead(tmpState.ERR_Alarmstatus, 7)).c_str(), false);
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
      mqttPublish(addTopic("/config/Sommer_ab"), (cfgSommerAb[data[2+1]-9]).c_str(), false);                       // "CFG_Sommer_ab"            => "0000:1,p:-9,a"
      mqttPublish(addTopic("/config/HK1_Nachttemperatur"), String(decode05cTemp(data[2+2]) + String(" °C")).c_str(), false);       // "CFG_HK1_Nachttemperatur"  => "0000:2,d:2"
      mqttPublish(addTopic("/config/HK1_Tagtemperatur"), String(decode05cTemp(data[2+3]) + String(" °C")).c_str(), false);         // CFG_HK1_Tagtemperatur"     => "0000:3,d:2"
      mqttPublish(addTopic("/config/HK1_Betriebsart"), cfgBetriebsart[data[2+4]].c_str(), false);                                  // CFG_HK1_Betriebsart"       => "0000:4,a:4"
      mqttPublish(addTopic("/config/HK1_Urlaubtemperatur"), String(decode05cTemp(data[2+5]) + String(" °C")).c_str(), false);      //CFG_HK1_Urlaubtemperatur"   => "0000:5,d:2"
      break;

    case 0x000e: 
      mqttPublish(addTopic("/config/HK1_Max_Temperatur"), String(data[2+2] + String(" °C")).c_str(), false);        // "CFG_HK1_Max_Temperatur"    => "000e:2"
      mqttPublish(addTopic("/config/HK1_Auslegung"), String(data[2+4]).c_str(), false);                             // CFG_HK1_Auslegung"          => "000e:4"
      break;
    
    case 0x0015: 
      mqttPublish(addTopic("/config/HK1_Aufschalttemperatur"), (cfgAufschalttemperatur[data[2]] + String(" °C")).c_str(), false);  // "CFG_HK1_Aufschalttemperatur"  => "0015:0,a"
      mqttPublish(addTopic("/config/HK1_Aussenhalt_ab"), String(decodeNegTemp(data[2+2]) + String(" °C")).c_str(), false);        // CFG_HK1_Aussenhalt_ab"         => "0015:2,s"
      break;
    
    case 0x001c: 
      mqttPublish(addTopic("/config/HK1_Absenkungsart"), cfgAbsenkungsart[data[2+1]].c_str(), false);     // "CFG_HK1_Absenkungsart"    => "001c:1,a"
      mqttPublish(addTopic("/config/HK1_Heizsystem"), cfgHeizsystem[data[2+2]].c_str(), false);           // "CFG_HK1_Heizsystem"       => "001c:2,a"
      break;

    case 0x0031: 
      mqttPublish(addTopic("/config/HK1_Temperatur_Offset"), String(decode05cTemp(decodeNegTemp(data[2+3])) + String(" °C")).c_str(), false);   // "CFG_HK1_Temperatur_Offset"    => "0031:3,s,d:2"
      mqttPublish(addTopic("/config/HK1_Fernbedienung"), cfgAnAus[data[2+4]].c_str(), false);                                                   // "CFG_HK1_Fernbedienung"        => "0031:4,a"  
      mqttPublish(addTopic("/config/Frost_ab"), String(decodeNegTemp(data[2+5]) + String(" °C")).c_str(), false);                               // "CFG_Frost_ab"                 => "0031:5,s"
      break;

    case 0x004d: 
      mqttPublish(addTopic("/config/WW_Vorrang"), cfgAnAus[data[2+1]].c_str(), false);     // "CFG_WW_Vorrang"   => "004d:1,a"
      break;

    case 0x0070: 
      mqttPublish(addTopic("/config/Gebaeudeart"), cfgGebaude[data[2+2]].c_str(), false);     // "CFG_Gebaeudeart"   => "0070:2,a" 
      break;

    case 0x007e: 
      mqttPublish(addTopic("/config/WW_Temperatur"), String(data[2+3] + String(" °C")).c_str(), false);     // "CFG_WW_Temperatur"  => "007e:3"
      break;

    case 0x0085: 
      mqttPublish(addTopic("/config/WW_Betriebsart"), cfgBetriebsart[data[2]].c_str(), false);    // "CFG_WW_Betriebsart"  => "0085:0,a"
      mqttPublish(addTopic("/config/WW_Aufbereitung"), cfgAnAus[data[2+3]].c_str(), false);       // "CFG_WW_Aufbereitung"  => "0085:3,a"
      mqttPublish(addTopic("/config/WW_Zirkulation"), cfgZirkulation[data[2+5]].c_str(), false);  // "CFG_WW_Zirkulation"   => "0085:5,a"
      break;

    case 0x0093: 
      mqttPublish(addTopic("/config/Sprache"), cfgSprache[data[2]].c_str(), false);      // "CFG_Sprache"   => "0093:0"
      mqttPublish(addTopic("/config/Anzeige"), cfgAnzeige[data[2+1]].c_str(), false);    // "CFG_Anzeige"   => "0093:1,a"
      break;

    case 0x009a: 
      mqttPublish(addTopic("/config/Brennerart"), cfgBrennerart[data[2+1]-1].c_str(), false);                     // "CFG_Brennerart"             => "009a:1,p:-1,a:12"),
      mqttPublish(addTopic("/config/Max_Kesseltemperatur"), String(data[2+3] + String(" °C")).c_str(), false);    // "CFG_Max_Kesseltemperatur"   => "009a:3"
      break;

    case 0x00a1: 
      mqttPublish(addTopic("/config/Pumplogik"), String(data[2] + String(" °C")).c_str(), false);                                    // "CFG_Pumplogik"                => "00a1:0"
      mqttPublish(addTopic("/config/Abgastemperaturschwelle"), (cfgAbgasTempSchwelle[data[2+5]-9]).c_str(), false);  // "CFG_Abgastemperaturschwelle"  => "00a1:5,p:-9,a"
      break;

    case 0x00a8: 
      mqttPublish(addTopic("/config/Brenner_Min_Modulation"), String(data[2]).c_str(), false);   // "CFG_Brenner_Min_Modulation"       => "00a8:0"
      mqttPublish(addTopic("/config/Brenner_Mod_Laufzeit"), String(data[2+1]).c_str(), false);     // "CFG_Brenner_Mod_Laufzeit"       => "00a8:1"
      break;

    // undefined
    default:   
      //sendString = String(data[0], HEX) + "_" + String(data[1], HEX) + "_" + String(data[2], HEX) + "_" + String(data[3], HEX) + "_" + String(data[4], HEX) + "_" + String(data[5], HEX) + "_" + String(data[6], HEX) + "_" + String(data[7], HEX) + "_" + String(data[8], HEX) + "_" + String(data[9], HEX) + "_" + String(data[10], HEX) + "_" + String(data[11], HEX);
      //mqttPublish(addTopic("/undefinded_message"), (sendString).c_str(), false);                                                      
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
 * @brief   Decodes KM271 temperatures with negative temperature range
 * @details Values >128 are negative
 * @param   data: the receive dat byte
 * @return  the decoded value as float
  * ********************************************************************/
float decodeNegTemp(uint8_t data) {
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
          sendTxBlock(KmCLogMode, sizeof(KmCLogMode));                      // Send logging command
          KmRxBlockState = KM_TSK_LG_CMD;                                   // Switch to check for logging mode state
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
        if (send_request){                                                  // If a send-request is active, 
          sendTxBlock(KmCSTX, sizeof(KmCSTX));                              // send STX to KM271 to request for send data
        }
        else {
          sendTxBlock(KmCDLE, sizeof(KmCDLE));                              // Confirm handling of block by sending DLE
        }
      } else if(data[0] == KM_DLE) {                                        // KM271 is ready to receive
          sendTxBlock(send_buf, sizeof(send_buf));                          // send buffer 
          send_request = false;                                             // reset send-request
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
  infoJSON[0]["send_cmd_busy"] = send_request;
  infoJSON[0]["date-time"] = getDateTimeString();
  String sendInfoJSON;
  serializeJson(infoJSON, sendInfoJSON);
  mqttPublish(addTopic("/info"),String(sendInfoJSON).c_str(), false);
}

/**
 * *******************************************************************
 * @brief   set actual date and time to buderus
 * @param   dti: date and time info structure
 * @return  none
 * *******************************************************************/
void km271SetDateTime(){
  time_t now;                       // this is the epoch
  tm dti;                           // the structure tm holds time information in a more convient way
  time(&now);                       // read the current time
  localtime_r(&now, &dti);          // update the structure tm with the current time
  send_request = true;
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
  mqttPublish(addTopic("/message"), "date and time set!", false);
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: parameter
 * @return  none
 * *******************************************************************/
void km271sendCmd(e_km271_sendCmd sendCmd, uint8_t cmdPara){

  switch (sendCmd)
  {
  case KM271_SENDCMD_HK1_BA:
    if (cmdPara>=0 && cmdPara<=2){
      send_request = true;
      send_buf[0]= 0x07;        // Daten-Typ für HK1 (0x07) 
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= cmdPara;     // 0:Nacht | 1:Tag | 2:AUTO
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: hk1_betriebsart - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: hk1_betriebsart - invald value", false);
    }
    break;
  
  case KM271_SENDCMD_HK1_AUSLEGUNG:
    if (cmdPara>=30 && cmdPara<=90){
      send_request = true;
      send_buf[0]= 0x07;        // Daten-Typ für HK1 (0x07)
      send_buf[1]= 0x0E;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= cmdPara;     // Auflösung: 1 °C Stellbereich: 30 – 90 °C WE: 75 °C
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: hk1_auslegung - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: hk1_auslegung - invald value", false);
    }
    break;

  case KM271_SENDCMD_HK1_PROGRAMM:
    if (cmdPara>=0 && cmdPara<=9){
      send_request = true;
      send_buf[0]= 0x11;        // Daten-Typ für HK1 (0x07)
      send_buf[1]= 0x00;        // Offset
      send_buf[2]= cmdPara;     // Programmnummer 0..8
      send_buf[3]= 0x65;
      send_buf[4]= 0x65;
      send_buf[5]= 0x65;     
      send_buf[6]= 0x65; 
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: hk1_programm - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: hk1_programm - invald value", false);
    }
    break;

  case KM271_SENDCMD_WW_BA:
    if (cmdPara>=0 && cmdPara<=2){
      send_request = true;
      send_buf[0]= 0x0C;      // Daten-Typ für Warmwasser (0x0C)
      send_buf[1]= 0x0E;      // Offset
      send_buf[2]= cmdPara;   // 0:Nacht | 1:Tag | 2:AUTO
      send_buf[3]= 0x65; 
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65; 
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: ww_betriebsart - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: ww_betriebsart - invald value", false);
    }
    break;

  case KM271_SENDCMD_SOMMER_AB:
    if (cmdPara>=9 && cmdPara<=31){
      send_request = true;
      send_buf[0]= 0x07;      // Daten-Typ für HK1 (0x07) 
      send_buf[1]= 0x00;      // Offset 
      send_buf[2]= 0x65;
      send_buf[3]= cmdPara;   // 9:Winter | 10°-30° | 31:Sommer
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: sommer_ab - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: sommer_ab - invald value", false);
    }
    break;

  case KM271_SENDCMD_FROST_AB:
    if (cmdPara>=-20 && cmdPara<=10){
      send_request = true;
      send_buf[0]= 0x07;      // Daten-Typ für HK1 (0x07) 
      send_buf[1]= 0x31;      // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;      
      send_buf[4]= 0x65;
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= cmdPara;    // -20° ... +10°
      mqttPublish(addTopic("/message"), "setvalue: frost_ab - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: frost_ab - invald value", false);
    }
    break;

  case KM271_SENDCMD_AUSSENHALT:
    if (cmdPara>=-20 && cmdPara<=10){
      send_request = true;
      send_buf[0]= 0x07;      // Daten-Typ für HK1 (0x07)  
      send_buf[1]= 0x15;      // Offset 
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;       
      send_buf[4]= cmdPara;   // -20° ... +10°
      send_buf[5]= 0x65; 
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: aussenhalt_ab - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: aussenhalt_ab - invald value", false);
    }
    break;

  case KM271_SENDCMD_WW_SOLL:
    if (cmdPara>=30 && cmdPara<=60){
      send_request = true;
      send_buf[0]= 0x0C;        // Daten-Typ für Warmwasser (0x0C) 
      send_buf[1]= 0x07;        // Offset
      send_buf[2]= 0x65;
      send_buf[3]= 0x65;       
      send_buf[4]= 0x65;
      send_buf[5]= cmdPara;     // 30°-60°
      send_buf[6]= 0x65;
      send_buf[7]= 0x65;
      mqttPublish(addTopic("/message"), "setvalue: ww_soll - received", false);
    } else {
      mqttPublish(addTopic("/message"), "setvalue: ww_soll - invald value", false);
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
