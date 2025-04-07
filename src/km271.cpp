//*****************************************************************************
// The Software was created by Michael Meyer, but it was modified and adapted
//
// Title      : Handles 3964 protocol for KM271
// Author     : Michael Meyer (Codebase)
// Target MCU : ESP32/Arduino
//
//*****************************************************************************

#include <basics.h>
#include <km271.h>
#include <language.h>
#include <message.h>

/* P R O T O T Y P E S ********************************************************/
void sendTxBlock(uint8_t *data, int len);
void handleRxBlock(uint8_t *data, int len, uint8_t bcc);
void parseInfo(uint8_t *data, int len);
float decode05cTemp(uint8_t data);
int8_t decodeNegValue(uint8_t data);
void decodeTimer(char *timerInfo, unsigned int size, uint8_t dateOnOff, uint8_t time);
bool decodeErrorMsg(char *errorMsg, unsigned int size, uint8_t *data);
uint8_t getErrorTextIndex(uint8_t errorNr);
uint8_t limit(uint8_t lower, uint8_t value, uint8_t upper);
int compareHexValues(const char *filter, uint8_t data[]);
KmSerialStats kmSerialStats = {0, 0, false};

/* V A R I A B L E S ********************************************************/
s_err_array errMsgText; // Array of error messages

// This structure contains the complete received status of the KM271 (as far it is parsed by now).
// It is updated automatically on any changes by this driver and therefore kept up-to-date.
// Do not access it directly from outside this source to avoid inconsistency of the structure.
// Instead, use km271GetStatus() to get a current copy in a safe manner.
s_km271_status kmStatus;        // All Status values
s_km271_config_str kmConfigStr; // All Config values (String)
s_km271_config_num kmConfigNum; // All Config values (Number)
s_km271_alarm_str kmAlarmMsg;   // Alarm Messages

// Status machine handling
e_rxBlockState KmRxBlockState = KM_TSK_START; // The RX block state

// known commands to KM271, to be used with KmStartTx()
static uint8_t KmCSTX[] = {KM_STX};               // STX Command
static uint8_t KmCDLE[] = {KM_DLE};               // DLE Response
static uint8_t KmCNAK[] = {KM_NAK};               // NAK Response
static uint8_t KmCLogMode[] = {0xEE, 0x00, 0x00}; // Switch to Log Mode

// ************************ km271 handling variables ****************************
static uint8_t rxByte;                      // The received character. We are reading byte by byte only.
static e_rxState kmRxStatus = KM_RX_RESYNC; // Status in Rx reception
static uint8_t kmRxBcc = 0;                 // BCC value for Rx Block
static KmRx_s kmRxBuf;                      // Rx block storag
static uint8_t send_buf[8] = {};
static char kmTmpMsg[256];
static bool kmInitDone = false;
static bool km271RefreshActive = false;

/**
 * *******************************************************************
 * @brief   Main Handling of KM271
 * @details receice serial data and call all other function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicKM271() {
  // >>>>>>>>> KM271 Main Handling
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  if (Serial2.readBytes(&rxByte, 1)) { // Wait for RX byte, if timeout, just loop and read again

    kmSerialStats.RxBytes++; // increase received bytes

    // Protocol handling
    kmRxBcc ^= rxByte; // Calculate BCC
    switch (kmRxStatus) {
    case KM_RX_RESYNC:                                   // Unknown state, discard everthing but STX
      if (rxByte == KM_STX) {                            // React on STX only to re-synchronise
        kmRxBuf.buf[0] = KM_STX;                         // Store current STX
        kmRxBuf.len = 1;                                 // Set length
        kmRxStatus = KM_RX_IDLE;                         // Sync done, now continue to receive
        handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte); // Handle RX block
      }
      break;
    case KM_RX_IDLE:                                                        // Start of block or command
      kmRxBuf.buf[0] = rxByte;                                              // Store current byte
      kmRxBuf.len = 1;                                                      // Initialise length
      kmRxBcc = rxByte;                                                     // Reset BCC
      if ((rxByte == KM_STX) || (rxByte == KM_DLE) || (rxByte == KM_NAK)) { // Give STX, DLE, NAK directly to caller
        handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte);                    // Handle RX block
      } else {                                                              // Whole block will follow
        kmRxStatus = KM_RX_ON;                                              // More data to follow, start collecting
      }
      break;
    case KM_RX_ON:              // Block reception ongoing
      if (rxByte == KM_DLE) {   // Handle DLE doubling
        kmRxStatus = KM_RX_DLE; // Discard first received DLE, could be doubling or end of block, check in next state
        break;                  // Quit here without storing
      }
      if (kmRxBuf.len >= KM_RX_BUF_LEN) { // Check allowed block len, if too long, re-sync
        kmRxStatus = KM_RX_RESYNC;        // Enter re-sync
        break;                            // Do not save data beyond array border
      }
      kmRxBuf.buf[kmRxBuf.len] = rxByte; // No DLE -> store regular, current byte
      kmRxBuf.len++;                     // Adjust length in rx buffer
      break;
    case KM_RX_DLE:                         // Entered when one DLE was already received
      if (rxByte == KM_DLE) {               // Double DLE?
        if (kmRxBuf.len >= KM_RX_BUF_LEN) { // Check allowed block len, if too long, re-sync
          kmRxStatus = KM_RX_RESYNC;        // Enter re-sync
          break;                            // Do not save data beyond array border
        }
        kmRxBuf.buf[kmRxBuf.len] = rxByte; // Yes -> store this DLE as valid part of data
        kmRxBuf.len++;                     // Adjust length in rx buffer
        kmRxStatus = KM_RX_ON;             // Continue to receive block
      } else {                             // This should be ETX now
        if (rxByte == KM_ETX) {            // Really? then we are done, just waiting for BCC
          kmRxStatus = KM_RX_BCC;          // Receive BCC and verify it
        } else {
          kmRxStatus = KM_RX_RESYNC; // Something wrong, just try to restart
        }
      }
      break;
    case KM_RX_BCC:                                      // Last stage, BCC verification, "received BCC" ^ "calculated BCC" shall be 0
      if (!kmRxBcc) {                                    // Block is valid
        handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte); // Handle RX block, provide BCC for debug logging, too
      } else {
        sendTxBlock(KmCNAK, sizeof(KmCNAK)); // Send NAK, ask for re-sending the block
      }
      kmRxStatus = KM_RX_IDLE; // Wait for next data or re-sent block
      break;
    } // end-case
  }   // end-if

  // global status logmode active
  kmSerialStats.logModeActive = (KmRxBlockState == KM_TSK_LOGGING);
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
  switch (KmRxBlockState) {
  case KM_TSK_START: // We need to switch to logging mode, first
    switch (data[0]) {
    case KM_STX:                           // First step: wait for STX
      sendTxBlock(KmCSTX, sizeof(KmCSTX)); // Send STX to KM271
      break;
    case KM_DLE:                                   // DLE received, KM ready to receive command
      sendTxBlock(KmCLogMode, sizeof(KmCLogMode)); // Send logging command
      KmRxBlockState = KM_TSK_LG_CMD;              // Switch to check for logging mode state
      break;
    }
    break;
  case KM_TSK_LG_CMD:                // Check if logging mode is accepted by KM
    if (data[0] != KM_DLE) {         // No DLE, not accepted, try again
      KmRxBlockState = KM_TSK_START; // Back to START state
    } else {
      km271RefreshActive = true;
      KmRxBlockState = KM_TSK_LOGGING; // Command accepted, ready to log!
    }
    break;
  case KM_TSK_LOGGING:                       // We have reached logging state
    if (data[0] == KM_STX) {                 // If STX, this is a send request
      if (send_buf[0] != 0) {                // If a send-request is active,
        sendTxBlock(KmCSTX, sizeof(KmCSTX)); // send STX to KM271 to request for send data
      } else {
        sendTxBlock(KmCDLE, sizeof(KmCDLE)); // Confirm handling of block by sending DLE
      }
    } else if (data[0] == KM_DLE) {            // KM271 is ready to receive
      sendTxBlock(send_buf, sizeof(send_buf)); // send buffer
      memset(send_buf, 0, sizeof(send_buf));   // clear buffer
      KmRxBlockState = KM_TSK_START;           // start log-mode again, to get all new values
    } else {                                   // If not STX, it should be valid data block
      parseInfo(data, len);                    // Handle data block with event information
      sendTxBlock(KmCDLE, sizeof(KmCDLE));     // Confirm handling of block by sending DLE
    }
    break;
  }
}

/**
 * *******************************************************************
 * @brief   Sends a single block of data.
 * @details Data is given as "pure" data without protocol bytes.
 *          This function adds the protocol bytes (including BCC calculation) and
 *          sends it to the Ecomatic 2000.
 *          STX and DLE handling needs to be done outside in handleRxBlock().
 * @param   data: Pointer to the data to be send. Single byte data is send right away (i.e,. ptotocol bytes such as STX.
 * DLE, NAK...) Block data is prepared with doubling DLE and block end indication.
 * @param   len:  Blocksize in number of bytes, without protocol data
 * @return  none
 *
 * *******************************************************************/
void sendTxBlock(uint8_t *data, int len) {
  uint8_t buf[256]; // To store bytes to be sent
  int ii, txLen;
  uint8_t bcc = 0;

  if (!len)
    return; // Nothing to do
  if ((len == 1) &&
      ((data[0] == KM_STX) || (data[0] == KM_DLE) || (data[0] == KM_NAK))) { // Shall a single protocol byte be sent? If yes, send it right away.
    Serial2.write(data, 1);
    kmSerialStats.TxBytes += 1;
    return;
  }
  // Here, we need to send a whole block of data. So prepare data.
  for (txLen = 0, ii = 0; ii < len; ii++, txLen++) {
    if (ii)
      bcc ^= data[ii];
    else
      bcc = data[ii]; // Initialize / calculate bcc
    buf[txLen] = data[ii];
    if (data[ii] == KM_DLE) { // Doubling of DLE needed?
      bcc ^= data[ii];        // Consider second DLE in BCC calculation
      txLen++;                // Make space for second DLE
      buf[txLen] = data[ii];  // Store second DLE
    }
  }
  // Append buffer with DLE, ETX, BCC
  bcc ^= KM_DLE;
  buf[txLen] = KM_DLE;

  txLen++; // Make space for ETX
  bcc ^= KM_ETX;
  buf[txLen] = KM_ETX;

  txLen++; // Make space for BCC
  buf[txLen] = bcc;
  txLen++;

  Serial2.write(buf, txLen); // Send the complete block
  kmSerialStats.TxBytes += txLen;
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

  char t1[100] = {'\0'};
  char t2[100] = {'\0'};
  char t3[100] = {'\0'};
  char tmpMessage[300] = {'\0'};

  uint kmregister = (data[0] * 256) + data[1];

  /********************************************************
   * publish all incomming messages for debug reasons
   ********************************************************/
  if (kmregister != 0x0400 && kmregister != 0x882e && kmregister != 0x882f) {
    if (config.debug.enable && compareHexValues(config.debug.filter, data)) {
      snprintf(tmpMessage, sizeof(tmpMessage), "%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x", data[0], data[1], data[2], data[3], data[4],
               data[5], data[6], data[7], data[8], data[9], data[10]);
      km271Msg(KM_TYP_DEBUG, tmpMessage, "");
    }
  }

  switch (kmregister) { // Check if we can find known stati

    /*
     **********************************************************************************
     * config values beginning with 0x00
     * # Message address:byte_offset in the message
     * Attributes:
     *   d:x (divide), p:x (add), bf:x (bitfield), a:x (array), ne (generate no event)
     *   mb:x (multi-byte-message, x-bytes, low byte), s (signed value)
     *   t (timer - special handling), eh (error history - special handling)
     ***********************************************************************************
     */

  case 0x0000:
    kmConfigNum.hc1_summer_mode_threshold = data[2 + 1];
    snprintf(kmConfigStr.hc1_summer_mode_threshold, sizeof(kmConfigStr.hc1_summer_mode_threshold), "%s",
             KM_CFG_ARRAY::SUMMER[config.mqtt.lang][limit(0, kmConfigNum.hc1_summer_mode_threshold - 9, 22)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc1_summer_mode_threshold); // "CFG_Sommer_ab"            => "0000:1,p:-9,a"

    if (config.km271.use_hc1) {
      kmConfigNum.hc1_night_temp = decode05cTemp(data[2 + 2]);
      snprintf(kmConfigStr.hc1_night_temp, sizeof(kmConfigStr.hc1_night_temp), "%0.1f °C", kmConfigNum.hc1_night_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_night_temp); // "CFG_HK1_Nachttemperatur"  => "0000:2,d:2"

      kmConfigNum.hc1_day_temp = decode05cTemp(data[2 + 3]);
      snprintf(kmConfigStr.hc1_day_temp, sizeof(kmConfigStr.hc1_day_temp), "%0.1f °C", kmConfigNum.hc1_day_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_day_temp); // "CFG_HK1_Tagtemperatur"     => "0000:3,d:2"

      kmConfigNum.hc1_operation_mode = data[2 + 4];
      snprintf(kmConfigStr.hc1_operation_mode, sizeof(kmConfigStr.hc1_operation_mode), "%s",
               KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.hc1_operation_mode, 2)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang],
               kmConfigStr.hc1_operation_mode); // "CFG_HK1_Betriebsart"       => "0000:4,a:4"

      kmConfigNum.hc1_holiday_temp = decode05cTemp(data[2 + 5]);
      snprintf(kmConfigStr.hc1_holiday_temp, sizeof(kmConfigStr.hc1_holiday_temp), "%0.1f °C", kmConfigNum.hc1_holiday_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_holiday_temp); // "CFG_HK1_Urlaubtemperatur"   => "0000:5,d:2"
    }
    break;

  case 0x000e:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_max_temp = data[2 + 2];
      snprintf(kmConfigStr.hc1_max_temp, sizeof(kmConfigStr.hc1_max_temp), "%i °C", kmConfigNum.hc1_max_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_MAX_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_max_temp); // "CFG_HK1_Max_Temperatur"    => "000e:2"

      kmConfigNum.hc1_interpretation = data[2 + 4];
      snprintf(kmConfigStr.hc1_interpretation, sizeof(kmConfigStr.hc1_interpretation), "%i °C", kmConfigNum.hc1_interpretation);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang],
               kmConfigStr.hc1_interpretation); // CFG_HK1_Auslegung"          => "000e:4"
    }
    break;

  case 0x0015:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_switch_on_temperature = data[2];
      snprintf(kmConfigStr.hc1_switch_on_temperature, sizeof(kmConfigStr.hc1_switch_on_temperature), "%s",
               KM_CFG_ARRAY::SWITCH_ON_TEMP[config.mqtt.lang][limit(0, kmConfigNum.hc1_switch_on_temperature, 10)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_switch_on_temperature); // "CFG_HK1_Aufschalttemperatur"  => "0015:0,a"

      kmConfigNum.hc1_switch_off_threshold = decodeNegValue(data[2 + 2]);
      snprintf(kmConfigStr.hc1_switch_off_threshold, sizeof(kmConfigStr.hc1_switch_off_threshold), "%i °C", kmConfigNum.hc1_switch_off_threshold);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang],
               kmConfigStr.hc1_switch_off_threshold); // CFG_HK1_Aussenhalt_ab"         => "0015:2,s"
    }
    break;

  case 0x001c:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_reduction_mode = data[2 + 1];
      snprintf(kmConfigStr.hc1_reduction_mode, sizeof(kmConfigStr.hc1_reduction_mode), "%s",
               KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][limit(0, kmConfigNum.hc1_reduction_mode, 3)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang],
               kmConfigStr.hc1_reduction_mode); // "CFG_HK1_Absenkungsart"    => "001c:1,a"

      kmConfigNum.hc1_heating_system = data[2 + 2];
      snprintf(kmConfigStr.hc1_heating_system, sizeof(kmConfigStr.hc1_heating_system), "%s",
               KM_CFG_ARRAY::HEATING_SYSTEM[config.mqtt.lang][limit(0, kmConfigNum.hc1_heating_system, 3)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HEATING_SYSTEM[config.mqtt.lang],
               kmConfigStr.hc1_heating_system); // "CFG_HK1_Heizsystem"       => "001c:2,a"
    }
    break;

  case 0x0031:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_temp_offset = decode05cTemp(decodeNegValue(data[2 + 3]));
      snprintf(kmConfigStr.hc1_temp_offset, sizeof(kmConfigStr.hc1_temp_offset), "%0.1f °C", kmConfigNum.hc1_temp_offset);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TEMP_OFFSET[config.mqtt.lang],
               kmConfigStr.hc1_temp_offset); // "CFG_HK1_Temperatur_Offset"    => "0031:3,s,d:2"

      kmConfigNum.hc1_remotecontrol = data[2 + 4];
      snprintf(kmConfigStr.hc1_remotecontrol, sizeof(kmConfigStr.hc1_remotecontrol), "%s",
               KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.hc1_remotecontrol, 1)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_REMOTECTRL[config.mqtt.lang],
               kmConfigStr.hc1_remotecontrol); // "CFG_HK1_Fernbedienung"        => "0031:4,a"
    }

    kmConfigNum.hc1_frost_protection_threshold = decodeNegValue(data[2 + 5]);
    snprintf(kmConfigStr.hc1_frost_protection_threshold, sizeof(kmConfigStr.hc1_frost_protection_threshold), "%i °C",
             kmConfigNum.hc1_frost_protection_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc1_frost_protection_threshold); // "CFG_Frost_ab"                 => "0031:5,s"
    break;

  case 0x0038:

    if (config.km271.use_hc2) {
      kmConfigNum.hc2_summer_mode_threshold = data[2 + 1];
      snprintf(kmConfigStr.hc2_summer_mode_threshold, sizeof(kmConfigStr.hc2_summer_mode_threshold), "%s",
               KM_CFG_ARRAY::SUMMER[config.mqtt.lang][limit(0, kmConfigNum.hc2_summer_mode_threshold - 9, 22)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang],
               kmConfigStr.hc2_summer_mode_threshold); // "CFG_Sommer_ab"            => "0038:1,p:-9,a"

      kmConfigNum.hc2_night_temp = decode05cTemp(data[2 + 2]);
      snprintf(kmConfigStr.hc2_night_temp, sizeof(kmConfigStr.hc2_night_temp), "%0.1f °C", kmConfigNum.hc2_night_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_night_temp); // "CFG_HK2_Nachttemperatur"   => "0038:2,d:2"

      kmConfigNum.hc2_day_temp = decode05cTemp(data[2 + 3]);
      snprintf(kmConfigStr.hc2_day_temp, sizeof(kmConfigStr.hc2_day_temp), "%0.1f °C", kmConfigNum.hc2_day_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_day_temp); // "CFG_HK2_Tagtemperatur"     => "0038:3,d:2"

      kmConfigNum.hc2_operation_mode = data[2 + 4];
      snprintf(kmConfigStr.hc2_operation_mode, sizeof(kmConfigStr.hc2_operation_mode), "%s",
               KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.hc2_operation_mode, 2)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang],
               kmConfigStr.hc2_operation_mode); // "CFG_HK2_Betriebsart"       => "0038:4,a:4"

      kmConfigNum.hc2_holiday_temp = decode05cTemp(data[2 + 5]);
      snprintf(kmConfigStr.hc2_holiday_temp, sizeof(kmConfigStr.hc2_holiday_temp), "%0.1f °C", kmConfigNum.hc2_holiday_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_holiday_temp); // "CFG_HK2_Urlaubtemperatur"  => "0038:5,d:2"
    }
    break;

  case 0x0046:
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_max_temp = data[2 + 2];
      snprintf(kmConfigStr.hc2_max_temp, sizeof(kmConfigStr.hc2_max_temp), "%i °C", kmConfigNum.hc2_max_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_MAX_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_max_temp); // "CFG_HK2_Max_Temperatur"    => "0046:2"

      kmConfigNum.hc2_interpretation = data[2 + 4];
      snprintf(kmConfigStr.hc2_interpretation, sizeof(kmConfigStr.hc2_interpretation), "%i °C", kmConfigNum.hc2_interpretation);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang],
               kmConfigStr.hc2_interpretation); // "CFG_HK2_Auslegung"         => "0046:4"
    }
    break;

  case 0x004d:
    kmConfigNum.ww_priority = data[2 + 1];
    snprintf(kmConfigStr.ww_priority, sizeof(kmConfigStr.ww_priority), "%s",
             KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.ww_priority, 1)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PRIO[config.mqtt.lang],
             kmConfigStr.ww_priority); // "CFG_WW_Vorrang"   => "004d:1,a"

    if (config.km271.use_hc2) {
      kmConfigNum.hc2_switch_on_temperature = data[2];
      snprintf(kmConfigStr.hc2_switch_on_temperature, sizeof(kmConfigStr.hc2_switch_on_temperature), "%s",
               KM_CFG_ARRAY::SWITCH_ON_TEMP[config.mqtt.lang][limit(0, kmConfigNum.hc2_switch_on_temperature, 10)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_switch_on_temperature); // "CFG_HK1_Aufschalttemperatur"  => "004d:0,a"

      kmConfigNum.hc2_switch_off_threshold = decodeNegValue(data[2 + 2]);
      snprintf(kmConfigStr.hc2_switch_off_threshold, sizeof(kmConfigStr.hc2_switch_off_threshold), "%i °C", kmConfigNum.hc2_switch_off_threshold);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang],
               kmConfigStr.hc2_switch_off_threshold); // CFG_HK1_Aussenhalt_ab"         => "004d:2,s"
    }
    break;

  case 0x0054:
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_reduction_mode = data[2 + 1];
      snprintf(kmConfigStr.hc2_reduction_mode, sizeof(kmConfigStr.hc2_reduction_mode), "%s",
               KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][limit(0, kmConfigNum.hc2_reduction_mode, 3)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang],
               kmConfigStr.hc2_reduction_mode); // "CFG_HK1_Absenkungsart"    => "0054:1,a"

      kmConfigNum.hc2_heating_system = data[2 + 2];
      snprintf(kmConfigStr.hc2_heating_system, sizeof(kmConfigStr.hc2_heating_system), "%s",
               KM_CFG_ARRAY::HEATING_SYSTEM[config.mqtt.lang][limit(0, kmConfigNum.hc2_heating_system, 3)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HEATING_SYSTEM[config.mqtt.lang],
               kmConfigStr.hc2_heating_system); // "CFG_HK1_Heizsystem"       => "0054:2,a"
    }
    break;

  case 0x0069:
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_temp_offset = decode05cTemp(decodeNegValue(data[2 + 3]));
      snprintf(kmConfigStr.hc2_temp_offset, sizeof(kmConfigStr.hc2_temp_offset), "%0.1f °C", kmConfigNum.hc2_temp_offset);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TEMP_OFFSET[config.mqtt.lang],
               kmConfigStr.hc2_temp_offset); // "CFG_HK2_Temperatur_Offset"    => "0069:3,s,d:2"

      kmConfigNum.hc2_remotecontrol = data[2 + 4];
      snprintf(kmConfigStr.hc2_remotecontrol, sizeof(kmConfigStr.hc2_remotecontrol), "%s",
               KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.hc2_remotecontrol, 1)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_REMOTECTRL[config.mqtt.lang],
               kmConfigStr.hc2_remotecontrol); // "CFG_HK2_Fernbedienung"        => "0069:4,a"

      kmConfigNum.hc2_frost_protection_threshold = decodeNegValue(data[2 + 5]);
      snprintf(kmConfigStr.hc2_frost_protection_threshold, sizeof(kmConfigStr.hc2_frost_protection_threshold), "%i °C",
               kmConfigNum.hc2_frost_protection_threshold);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang],
               kmConfigStr.hc2_frost_protection_threshold); // "CFG_Frost_ab"                 => "0069:5,s"
    }
    break;

  case 0x0070:
    kmConfigNum.building_type = data[2 + 2];
    snprintf(kmConfigStr.building_type, sizeof(kmConfigStr.building_type), "%s",
             KM_CFG_ARRAY::BUILDING_TYPE[config.mqtt.lang][limit(0, kmConfigNum.building_type, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BUILDING_TYP[config.mqtt.lang],
             kmConfigStr.building_type); // "CFG_Gebaeudeart"   => "0070:2,a"
    break;

  case 0x007e:
    kmConfigNum.ww_temp = data[2 + 3];
    snprintf(kmConfigStr.ww_temp, sizeof(kmConfigStr.ww_temp), "%i °C", kmConfigNum.ww_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang],
             kmConfigStr.ww_temp); // "CFG_WW_Temperatur"  => "007e:3"
    break;

  case 0x0085:
    kmConfigNum.ww_operation_mode = data[2];
    snprintf(kmConfigStr.ww_operation_mode, sizeof(kmConfigStr.ww_operation_mode), "%s",
             KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.ww_operation_mode, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang],
             kmConfigStr.ww_operation_mode); // "CFG_WW_Betriebsart"  => "0085:0,a"

    kmConfigNum.ww_processing = data[2 + 3];
    snprintf(kmConfigStr.ww_processing, sizeof(kmConfigStr.ww_processing), "%s",
             KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.ww_processing, 1)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PROCESSING[config.mqtt.lang],
             kmConfigStr.ww_processing); // "CFG_WW_Aufbereitung"  => "0085:3,a"

    kmConfigNum.ww_circulation = data[2 + 5];
    snprintf(kmConfigStr.ww_circulation, sizeof(kmConfigStr.ww_circulation), "%s",
             KM_CFG_ARRAY::CIRC_INTERVAL[config.mqtt.lang][limit(0, kmConfigNum.ww_circulation, 7)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang],
             kmConfigStr.ww_circulation); // "CFG_WW_Zirkulation"   => "0085:5,a"
    break;

  case 0x0093:
    kmConfigNum.language = data[2];
    snprintf(kmConfigStr.language, sizeof(kmConfigStr.language), "%s", KM_CFG_ARRAY::LANGUAGE[config.mqtt.lang][limit(0, kmConfigNum.language, 5)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::LANGUAGE[config.mqtt.lang], kmConfigStr.language); // "CFG_Sprache"   => "0093:0"

    kmConfigNum.display = data[2 + 1];
    snprintf(kmConfigStr.display, sizeof(kmConfigStr.display), "%s", KM_CFG_ARRAY::SCREEN[config.mqtt.lang][limit(0, kmConfigNum.display, 3)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SCREEN[config.mqtt.lang], kmConfigStr.display); // "CFG_Anzeige"   => "0093:1,a"
    break;

  case 0x009a:
    kmConfigNum.burner_type = data[2 + 1];
    snprintf(kmConfigStr.burner_type, sizeof(kmConfigStr.burner_type), "%s",
             KM_CFG_ARRAY::BURNER_TYPE[config.mqtt.lang][limit(0, kmConfigNum.burner_type - 1, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_TYP[config.mqtt.lang],
             kmConfigStr.burner_type); // "CFG_Brennerart"             => "009a:1,p:-1,a:12"),
    kmConfigNum.max_boiler_temperature = data[2 + 3];
    snprintf(kmConfigStr.max_boiler_temperature, sizeof(kmConfigStr.max_boiler_temperature), "%i °C", kmConfigNum.max_boiler_temperature);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::MAX_BOILER_TEMP[config.mqtt.lang],
             kmConfigStr.max_boiler_temperature); // "CFG_Max_Kesseltemperatur"   => "009a:3"
    break;

  case 0x00a1:
    kmConfigNum.pump_logic_temp = data[2];
    snprintf(kmConfigStr.pump_logic_temp, sizeof(kmConfigStr.pump_logic_temp), "%i °C", kmConfigNum.pump_logic_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::PUMP_LOGIC[config.mqtt.lang],
             kmConfigStr.pump_logic_temp); // "CFG_Pumplogik"                => "00a1:0"

    kmConfigNum.exhaust_gas_temperature_threshold = data[2 + 5];
    snprintf(kmConfigStr.exhaust_gas_temperature_threshold, sizeof(kmConfigStr.exhaust_gas_temperature_threshold), "%s",
             KM_CFG_ARRAY::EXHAUST_GAS_THRESHOLD[config.mqtt.lang][limit(0, kmConfigNum.exhaust_gas_temperature_threshold - 9, 41)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::EXHAUST_THRESHOLD[config.mqtt.lang],
             kmConfigStr.exhaust_gas_temperature_threshold); // "CFG_Abgastemperaturschwelle"  => "00a1:5,p:-9,a"
    break;

  case 0x00a8:
    kmConfigNum.burner_min_modulation = data[2];
    snprintf(kmConfigStr.burner_min_modulation, sizeof(kmConfigStr.burner_min_modulation), "%i %%", kmConfigNum.burner_min_modulation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_MIN_MOD[config.mqtt.lang],
             kmConfigStr.burner_min_modulation); // "CFG_Brenner_Min_Modulation"     => "00a8:0"

    kmConfigNum.burner_modulation_runtime = data[2 + 1];
    snprintf(kmConfigStr.burner_modulation_runtime, sizeof(kmConfigStr.burner_modulation_runtime), "%i s", kmConfigNum.burner_modulation_runtime);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_MOD_TIME[config.mqtt.lang],
             kmConfigStr.burner_modulation_runtime); // "CFG_Brenner_Mod_Laufzeit"       => "00a8:1"
    break;

  case 0x0100:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_program = data[2];
      snprintf(kmConfigStr.hc1_program, sizeof(kmConfigStr.hc1_program), "%s",
               KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][limit(0, kmConfigNum.hc1_program, 8)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang],
               kmConfigStr.hc1_program); // "CFG_HK1_Programm"  => "0100:0"
      kmConfigNum.hc1_holiday_days = data[2 + 3];
      snprintf(kmConfigStr.hc1_holiday_days, sizeof(kmConfigStr.hc1_holiday_days), "%i %s", kmConfigNum.hc1_holiday_days,
               MQTT_MSG::DAYS[config.mqtt.lang]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang],
               kmConfigStr.hc1_holiday_days); // "CFG_HK1_Ferien_Tage"  => "0100:5"
    }
    break;

  case 0x0169:
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_program = data[2];
      snprintf(kmConfigStr.hc2_program, sizeof(kmConfigStr.hc2_program), "%s",
               KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][limit(0, kmConfigNum.hc2_program, 8)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang],
               kmConfigStr.hc2_program); // "CFG_HK2_Programm"  => "0169:0"
      kmConfigNum.hc2_holiday_days = data[2 + 3];
      snprintf(kmConfigStr.hc2_holiday_days, sizeof(kmConfigStr.hc2_holiday_days), "%i %s", kmConfigNum.hc2_holiday_days,
               MQTT_MSG::DAYS[config.mqtt.lang]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang],
               kmConfigStr.hc2_holiday_days); // "CFG_HK2_Ferien_Tage"  => "0169:5"
    }
    break;

  case 0x0107: // HK1_Timer01
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[0]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer01, sizeof(kmConfigStr.hc1_timer01), "SP01: %s | SP02: %s | SP03: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER01[config.mqtt.lang], kmConfigStr.hc1_timer01);
    }
    break;

  case 0x010e: // HK1_Timer02
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[1]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer02, sizeof(kmConfigStr.hc1_timer02), "SP04: %s | SP05: %s | SP05: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER02[config.mqtt.lang], kmConfigStr.hc1_timer02);
    }
    break;

  case 0x0115: // HK1_Timer03
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[2]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer03, sizeof(kmConfigStr.hc1_timer03), "SP07: %s | SP08: %s | SP09: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER03[config.mqtt.lang], kmConfigStr.hc1_timer03);
    }
    break;

  case 0x011c: // HK1_Timer04
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[3]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer04, sizeof(kmConfigStr.hc1_timer04), "SP10: %s | SP11: %s | SP12: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER04[config.mqtt.lang], kmConfigStr.hc1_timer04);
    }
    break;

  case 0x0123: // HK1_Timer05
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[4]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer05, sizeof(kmConfigStr.hc1_timer05), "SP13: %s | SP14: %s | SP15: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER05[config.mqtt.lang], kmConfigStr.hc1_timer05);
    }
    break;

  case 0x012a: // HK1_Timer06
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[5]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer06, sizeof(kmConfigStr.hc1_timer06), "SP16: %s | SP17: %s | SP18: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER06[config.mqtt.lang], kmConfigStr.hc1_timer06);
    }
    break;

  case 0x0131: // HK1_Timer07
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[6]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer07, sizeof(kmConfigStr.hc1_timer07), "SP19: %s | SP20: %s | SP21: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER07[config.mqtt.lang], kmConfigStr.hc1_timer07);
    }
    break;

  case 0x0138: // HK1_Timer08
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[7]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer08, sizeof(kmConfigStr.hc1_timer08), "SP22: %s | SP23: %s | SP24: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER08[config.mqtt.lang], kmConfigStr.hc1_timer08);
    }
    break;

  case 0x013f: // HK1_Timer09
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[8]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer09, sizeof(kmConfigStr.hc1_timer09), "SP25: %s | SP26: %s | SP27: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER09[config.mqtt.lang], kmConfigStr.hc1_timer09);
    }
    break;

  case 0x0146: // HK1_Timer10
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[9]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer10, sizeof(kmConfigStr.hc1_timer10), "SP28: %s | SP29: %s | SP30: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER10[config.mqtt.lang], kmConfigStr.hc1_timer10);
    }
    break;

  case 0x014d: // HK1_Timer11
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[10]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer11, sizeof(kmConfigStr.hc1_timer11), "SP31: %s | SP32: %s | SP33: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER11[config.mqtt.lang], kmConfigStr.hc1_timer11);
    }
    break;

  case 0x0154: // HK1_Timer12
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[11]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer12, sizeof(kmConfigStr.hc1_timer12), "SP34: %s | SP35: %s | SP36: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER12[config.mqtt.lang], kmConfigStr.hc1_timer12);
    }
    break;

  case 0x015b: // HK1_Timer13
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[12]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer13, sizeof(kmConfigStr.hc1_timer13), "SP37: %s | SP38: %s | SP39: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER13[config.mqtt.lang], kmConfigStr.hc1_timer13);
    }
    break;

  case 0x0162: // HK1_Timer14
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_timer[13]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc1_timer14, sizeof(kmConfigStr.hc1_timer14), "SP40: %s | SP41: %s | SP42: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER14[config.mqtt.lang], kmConfigStr.hc1_timer14);
    }
    break;

  case 0x0170: // HK2_Timer01
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[0]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer01, sizeof(kmConfigStr.hc2_timer01), "SP01: %s | SP02: %s | SP03: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER01[config.mqtt.lang], kmConfigStr.hc2_timer01);
    }
    break;

  case 0x0177: // HK2_Timer02
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[1]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer02, sizeof(kmConfigStr.hc2_timer02), "SP04: %s | SP05: %s | SP05: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER02[config.mqtt.lang], kmConfigStr.hc2_timer02);
    }
    break;

  case 0x017e: // HK2_Timer03
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[2]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer03, sizeof(kmConfigStr.hc2_timer03), "SP07: %s | SP08: %s | SP09: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER03[config.mqtt.lang], kmConfigStr.hc2_timer03);
    }
    break;

  case 0x0185: // HK2_Timer04
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[3]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer04, sizeof(kmConfigStr.hc2_timer04), "SP10: %s | SP11: %s | SP12: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER04[config.mqtt.lang], kmConfigStr.hc2_timer04);
    }
    break;

  case 0x018c: // HK2_Timer05
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[4]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer05, sizeof(kmConfigStr.hc2_timer05), "SP13: %s | SP14: %s | SP15: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER05[config.mqtt.lang], kmConfigStr.hc2_timer05);
    }
    break;

  case 0x0193: // HK2_Timer06
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[5]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer06, sizeof(kmConfigStr.hc2_timer06), "SP16: %s | SP17: %s | SP18: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER06[config.mqtt.lang], kmConfigStr.hc2_timer06);
    }
    break;

  case 0x019a: // HK2_Timer07
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[6]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer07, sizeof(kmConfigStr.hc2_timer07), "SP19: %s | SP20: %s | SP21: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER07[config.mqtt.lang], kmConfigStr.hc2_timer07);
    }
    break;

  case 0x01a1: // HK2_Timer08
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[7]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer08, sizeof(kmConfigStr.hc2_timer08), "SP22: %s | SP23: %s | SP24: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER08[config.mqtt.lang], kmConfigStr.hc2_timer08);
    }
    break;

  case 0x01a8: // HK2_Timer09
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[8]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer09, sizeof(kmConfigStr.hc2_timer09), "SP25: %s | SP26: %s | SP27: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER09[config.mqtt.lang], kmConfigStr.hc2_timer09);
    }
    break;

  case 0x01af: // HK2_Timer10
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[9]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer10, sizeof(kmConfigStr.hc2_timer10), "SP28: %s | SP29: %s | SP30: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER10[config.mqtt.lang], kmConfigStr.hc2_timer10);
    }
    break;

  case 0x01b6: // HK2_Timer11
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[10]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer11, sizeof(kmConfigStr.hc2_timer11), "SP31: %s | SP32: %s | SP33: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER11[config.mqtt.lang], kmConfigStr.hc2_timer11);
    }
    break;

  case 0x01bd: // HK2_Timer12
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[11]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer12, sizeof(kmConfigStr.hc2_timer12), "SP34: %s | SP35: %s | SP36: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER12[config.mqtt.lang], kmConfigStr.hc2_timer12);
    }
    break;

  case 0x01c4: // HK2_Timer13
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[12]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer13, sizeof(kmConfigStr.hc2_timer13), "SP37: %s | SP38: %s | SP39: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER13[config.mqtt.lang], kmConfigStr.hc2_timer13);
    }
    break;

  case 0x01cb: // HK2_Timer14
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_timer[13]++;
      decodeTimer(t1, sizeof(t1), data[2], data[3]);
      decodeTimer(t2, sizeof(t2), data[4], data[5]);
      decodeTimer(t3, sizeof(t3), data[6], data[7]);
      snprintf(kmConfigStr.hc2_timer14, sizeof(kmConfigStr.hc2_timer14), "SP40: %s | SP41: %s | SP42: %s", t1, t2, t3);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER14[config.mqtt.lang], kmConfigStr.hc2_timer14);
    }
    break;

  case 0x01d2:
    if (config.km271.use_solar) {
      // Solar Min-Temp => "01d2:6"
      kmConfigNum.solar_min = data[2 + 3];
      snprintf(kmConfigStr.solar_min, sizeof(kmConfigStr.solar_min), "%i °C", kmConfigNum.solar_min);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_TEMP_MIN[config.mqtt.lang], kmConfigStr.solar_min);

      // Solar Max-Temp => "01d2:7"
      kmConfigNum.solar_max = data[2 + 4];
      snprintf(kmConfigStr.solar_max, sizeof(kmConfigStr.solar_max), "%i °C", kmConfigNum.solar_max);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_TEMP_MAX[config.mqtt.lang], kmConfigStr.solar_max);

      // Solar EIN/AUS => "01d2:8"
      kmConfigNum.solar_activation = data[2 + 5];
      snprintf(kmConfigStr.solar_activation, sizeof(kmConfigStr.solar_activation), "%s",
               KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.solar_activation, 1)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_ENABLE[config.mqtt.lang], kmConfigStr.solar_activation);
    }
    break;

  case 0x01d9: // Solar Betriebsart
    if (config.km271.use_solar) {
      // "Heizkreis Solar"  => "01d9:5,a"
      kmConfigNum.solar_operation_mode = data[2 + 2];
      snprintf(kmConfigStr.solar_operation_mode, sizeof(kmConfigStr.solar_operation_mode), "%s",
               KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.solar_operation_mode, 2)]);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_OPMODE[config.mqtt.lang], kmConfigStr.solar_operation_mode);
    }
    break;

  case 0x01e0: // 01e0:1,s Uhrzeit_Offset
    kmConfigNum.time_offset = decode05cTemp(decodeNegValue(data[2 + 1]));
    snprintf(kmConfigStr.time_offset, sizeof(kmConfigStr.time_offset), "%0.1f %s", kmConfigNum.time_offset, MQTT_MSG::HOURS[config.mqtt.lang]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::TIME_OFFSET[config.mqtt.lang],
             kmConfigStr.time_offset); // "CFG_Uhrzeit_Offset"    => "01e0:1,s"
    break;

  case 0x0300: // 0x0300 : Error Buffer 1
    if (config.km271.use_alarmMsg) {
      if (decodeErrorMsg(kmAlarmMsg.alarm1, sizeof(kmAlarmMsg.alarm1), data)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_1[config.mqtt.lang], kmAlarmMsg.alarm1); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_1[config.mqtt.lang],
                 kmAlarmMsg.alarm1); // acknowledged alarm (History)
      }
    }
    break;

  case 0x0307: // 0x0307 : Error Buffer 2
    if (config.km271.use_alarmMsg) {
      if (decodeErrorMsg(kmAlarmMsg.alarm2, sizeof(kmAlarmMsg.alarm2), data)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_2[config.mqtt.lang], kmAlarmMsg.alarm2); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_2[config.mqtt.lang],
                 kmAlarmMsg.alarm2); // acknowledged alarm (History)
      }
    }
    break;

  case 0x030e: // 0x030e : Error Buffer 3
    if (config.km271.use_alarmMsg) {
      if (decodeErrorMsg(kmAlarmMsg.alarm3, sizeof(kmAlarmMsg.alarm3), data)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_3[config.mqtt.lang], kmAlarmMsg.alarm3); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_3[config.mqtt.lang],
                 kmAlarmMsg.alarm3); // acknowledged alarm (History)
      }
    }
    break;

  case 0x0315: // 0x0315 : Error Buffer 4
    if (config.km271.use_alarmMsg) {
      if (decodeErrorMsg(kmAlarmMsg.alarm4, sizeof(kmAlarmMsg.alarm4), data)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_4[config.mqtt.lang], kmAlarmMsg.alarm4); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_4[config.mqtt.lang],
                 kmAlarmMsg.alarm4); // acknowledged alarm (History)
      }
    }
    break;

  case 0x0400:
    // 04_00_07_01_81_8e_00_c1_ff_00_00_00
    // some kind of lifesign - ignore
    break;

    /********************************************************
     * status values Heating Circuit 1
     ********************************************************/

  case 0x8000: // 0x8000 : Bitfield
    if (config.km271.use_hc1) {

      if (kmInitDone) {
        if (bitRead(kmStatus.HC1_OperatingStates_1, 6) != bitRead(data[2], 6)) {
          km271Msg(KM_TYP_MESSAGE, bitRead(data[2], 6) ? KM_INFO_MSG::HC1_FROST_MODE_ON[config.lang] : KM_INFO_MSG::HC1_FROST_MODE_OFF[config.lang],
                   "");
        }
      }

      kmStatus.HC1_OperatingStates_1 = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_OFFTIME_OPT[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 0)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_ONTIME_OPT[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 1)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_AUTOMATIC[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 2)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_WW_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 3)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_SCREED_DRY[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 4)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 5)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_FROST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 6)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 7)));
    }
    break;

  case 0x8001: // 0x8001 : Bitfield
    if (config.km271.use_hc1) {

      if (kmInitDone) {
        if (bitRead(kmStatus.HC1_OperatingStates_2, 0) != bitRead(data[2], 0)) {
          km271Msg(KM_TYP_MESSAGE, bitRead(data[2], 0) ? KM_INFO_MSG::HC1_SUMMER_MODE[config.lang] : KM_INFO_MSG::HC1_WINTER_MODE[config.lang], "");
        }
      }

      kmStatus.HC1_OperatingStates_2 = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_SUMMER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 0)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 1)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_NO_COM_REMOTE[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 2)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_REMOTE_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 3)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_FLOW_SENS_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 4)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_FLOW_AT_MAX[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 5)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_EXT_SENS_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 6)));
    }
    break;

  case 0x8002: // 0x8002 : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingForwardTargetTemp = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingForwardTargetTemp));
    }
    break;

  case 0x8003: // 0x8003 : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingForwardActualTemp = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_FLOW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingForwardActualTemp));
    }
    break;

  case 0x8004: // 0x8004 : Temperature (0.5C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_RoomTargetTemp = decode05cTemp(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ROOM_SETPOINT[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC1_RoomTargetTemp, 1));
    }
    break;

  case 0x8005: // 0x8005 : Temperature (0.5C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_RoomActualTemp = decode05cTemp(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ROOM_TEMP[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC1_RoomActualTemp, 1));
    }
    break;

  case 0x8006: // 0x8006 : Minutes
    if (config.km271.use_hc1) {
      kmStatus.HC1_SwitchOnOptimizationTime = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ON_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_SwitchOnOptimizationTime));
    }
    break;

  case 0x8007: // 0x8007 : Minutes
    if (config.km271.use_hc1) {
      kmStatus.HC1_SwitchOffOptimizationTime = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OFF_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_SwitchOffOptimizationTime));
    }
    break;

  case 0x8008: // 0x8008 : Percent
    if (config.km271.use_hc1) {
      kmStatus.HC1_PumpPower = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_PUMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_PumpPower));
    }
    break;

  case 0x8009: // 0x8009 : Percent
    if (config.km271.use_hc1) {
      kmStatus.HC1_MixingValue = decodeNegValue(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_MIXER[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_MixingValue));
    }
    break;

  case 0x800c: // 0x800c : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingCurvePlus10 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurvePlus10));
    }
    break;

  case 0x800d: // 0x800d : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingCurve0 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurve0));
    }
    break;

  case 0x800e: // 0x800e : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingCurveMinus10 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurveMinus10));
    }
    break;

  /********************************************************
   * status values Heating Circuit 2
   ********************************************************/
  case 0x8112: // 0x8112 : Bitfield
    if (config.km271.use_hc2) {

      if (kmInitDone) {
        if (bitRead(kmStatus.HC2_OperatingStates_1, 6) != bitRead(data[2], 6)) {
          km271Msg(KM_TYP_MESSAGE, bitRead(data[2], 6) ? KM_INFO_MSG::HC2_FROST_MODE_ON[config.lang] : KM_INFO_MSG::HC2_FROST_MODE_OFF[config.lang],
                   "");
        }
      }

      kmStatus.HC2_OperatingStates_1 = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_OFFTIME_OPT[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 0)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_ONTIME_OPT[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 1)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_AUTOMATIC[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 2)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_WW_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 3)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_SCREED_DRY[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 4)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 5)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_FROST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 6)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 7)));
    }
    break;

  case 0x8113: // 0x8113 : Bitfield
    if (config.km271.use_hc2) {

      if (kmInitDone) {
        if (bitRead(kmStatus.HC2_OperatingStates_2, 0) != bitRead(data[2], 0)) {
          km271Msg(KM_TYP_MESSAGE, bitRead(data[2], 0) ? KM_INFO_MSG::HC2_SUMMER_MODE[config.lang] : KM_INFO_MSG::HC2_WINTER_MODE[config.lang], "");
        }
      }

      kmStatus.HC2_OperatingStates_2 = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_SUMMER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 0)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 1)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_NO_COM_REMOTE[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 2)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_REMOTE_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 3)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_FLOW_SENS_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 4)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_FLOW_AT_MAX[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 5)));
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_EXT_SENS_ERR[config.mqtt.lang],
               EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 6)));
    }
    break;

  case 0x8114: // 0x8114 : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingForwardTargetTemp = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingForwardTargetTemp));
    }
    break;

  case 0x8115: // 0x8115 : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingForwardActualTemp = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_FLOW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingForwardActualTemp));
    }
    break;

  case 0x8116: // 0x8116 : Temperature (0.5C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_RoomTargetTemp = decode05cTemp(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ROOM_SETPOINT[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC2_RoomTargetTemp, 1));
    }
    break;

  case 0x8117: // 0x8117 : Temperature (0.5C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_RoomActualTemp = decode05cTemp(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ROOM_TEMP[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC2_RoomActualTemp, 1));
    }
    break;

  case 0x8118: // 0x8118 : Minutes
    if (config.km271.use_hc2) {
      kmStatus.HC2_SwitchOnOptimizationTime = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ON_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_SwitchOnOptimizationTime));
    }
    break;

  case 0x8119: // 0x8119 : Minutes
    if (config.km271.use_hc2) {
      kmStatus.HC2_SwitchOffOptimizationTime = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OFF_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_SwitchOffOptimizationTime));
    }
    break;

  case 0x811a: // 0x811a : Percent
    if (config.km271.use_hc2) {
      kmStatus.HC2_PumpPower = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_PUMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_PumpPower));
    }
    break;

  case 0x811b: // 0x811b : Percent
    if (config.km271.use_hc2) {
      kmStatus.HC2_MixingValue = decodeNegValue(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_MIXER[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_MixingValue));
    }
    break;

  case 0x811e: // 0x811e : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingCurvePlus10 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurvePlus10));
    }
    break;

  case 0x811f: // 0x811f : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingCurve0 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurve0));
    }
    break;

  case 0x8120: // 0x8120 : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingCurveMinus10 = (float)data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurveMinus10));
    }
    break;

  /********************************************************
   * status values general
   ********************************************************/
  case 0x8424: // 0x8424 : Bitfield
    kmStatus.HotWaterOperatingStates_1 = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_AUTO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_DESINFECT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_RELOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_DESINFECT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_SENSOR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_WW_STAY_COLD[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_ANODE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 7)));
    break;

  case 0x8425: // 0x8425 : Bitfield
    kmStatus.HotWaterOperatingStates_2 = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_LOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_RELOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_OFF_TIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_ON_TIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_HOT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 7)));
    break;

  case 0x8426: // 0x8426 : Temperature (1C resolution)
    kmStatus.HotWaterTargetTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterTargetTemp));
    break;

  case 0x8427: // 0x8427 : Temperature (1C resolution)
    kmStatus.HotWaterActualTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterActualTemp));
    break;

  case 0x8428: // 0x8428 : Minutes
    kmStatus.HotWaterOptimizationTime = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_ONTIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterOptimizationTime));
    break;

  case 0x8429: // 0x8429 :  Bitfield
    kmStatus.HotWaterPumpStates = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_CHARGE[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_CIRC[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_SOLAR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 2)));
    break;

  case 0x882a: // 0x882a : Temperature (1C resolution)
    kmStatus.BoilerForwardTargetTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BoilerForwardTargetTemp));
    break;

  case 0x882b: // 0x882b : Temperature (1C resolution)
    kmStatus.BoilerForwardActualTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BoilerForwardActualTemp));
    break;

  case 0x882c: // 0x882c : Temperature (1C resolution)
    kmStatus.BurnerSwitchOnTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ON_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerSwitchOnTemp));
    break;

  case 0x882d: // 0x882d : Temperature (1C resolution)
    kmStatus.BurnerSwitchOffTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_OFF_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerSwitchOffTemp));
    break;

  case 0x882e: // 0x882e : Number (*256)
    kmStatus.BoilerIntegral_1 = data[2];
    // do nothing - useless value
    break;

  case 0x882f: // 0x882f : Number (*1)
    kmStatus.BoilerIntegral_2 = data[2];
    // do nothing - useless value
    break;

  case 0x8830: // 0x8830 : Bitfield
    kmStatus.BoilerErrorStates = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_BURNER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_SENSOR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_AUX_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_STAY_COLD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_GAS_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_EXHAUST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_SAFETY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_EXT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 7)));
    break;

  case 0x8831: // 0x8831 : Bitfield
    kmStatus.BoilerOperatingStates = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_GASTEST[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_STAGE1[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PROTECT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_ACTIVE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PER_FREE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PER_HIGH[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_STAGE2[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 6)));
    break;

  case 0x8832: // 0x8832 : Bitfield
    kmStatus.BurnerStates = data[2];
    // [ "Kessel aus"), "1.Stufe an"), "-"), "-"), "2.Stufe an bzw. Modulation frei" ]
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_CONTROL[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerStates));
    break;

  case 0x8833: // 0x8833 : Temperature (1C resolution)
    kmStatus.ExhaustTemp = (float)data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::EXHAUST_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ExhaustTemp));
    break;

  case 0x8836: // 0x8836 : Minutes (*65536)
    kmStatus.BurnerOperatingDuration_0 = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_0));
    break;

  case 0x8837: // 0x8837 : Minutes (*256)
    kmStatus.BurnerOperatingDuration_1 = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_1));
    break;

  case 0x8838: // 0x8838 : Minutes (*1) + calculated sum of all 3 runtime values
    kmStatus.BurnerOperatingDuration_2 = data[2];
    kmStatus.BurnerOperatingDuration_Sum =
        kmStatus.BurnerOperatingDuration_2 + (kmStatus.BurnerOperatingDuration_1 * 256) + (kmStatus.BurnerOperatingDuration_0 * 65536);

    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_2));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_4[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_Sum));

    if (config.oilmeter.use_virtual_meter && config.oilmeter.oil_density_kg_l != 0) {
      kmStatus.BurnerCalcOilConsumption =
          ((double)kmStatus.BurnerOperatingDuration_Sum / 60 * config.oilmeter.consumption_kg_h / config.oilmeter.oil_density_kg_l) -
          config.oilmeter.virtual_calc_offset;
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_CONSUMPTION[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.BurnerCalcOilConsumption, 1));
    }
    break;

  case 0x893c: // 0x893c : Temperature (1C resolution, possibly negative)
    kmStatus.OutsideTemp = decodeNegValue(data[2]);
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::OUTSIDE_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.OutsideTemp));
    break;

  case 0x893d: // 0x893d : Temperature (1C resolution, possibly negative)
    kmStatus.OutsideDampedTemp = decodeNegValue(data[2]);
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::OUTSIDE_TEMP_DAMPED[config.mqtt.lang], EspStrUtil::intToString(kmStatus.OutsideDampedTemp));
    break;

  case 0x893e: // 0x893e : Number
    kmStatus.ControllerVersionMain = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::VERSION_VK[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ControllerVersionMain));
    break;

  case 0x893f: // 0x893f : Number
    kmStatus.ControllerVersionSub = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::VERSION_NK[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ControllerVersionSub));
    break;

  case 0x8940: // 0x8940 : Number
    kmStatus.Modul = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::MODULE_ID[config.mqtt.lang], EspStrUtil::intToString(kmStatus.Modul));
    // this should be the last message => init done
    kmInitDone = true;
    km271RefreshActive = false;
    break;

  /********************************************************
   * status values solar
   ********************************************************/
  case 0x9142:
    if (config.km271.use_solar) {
      kmStatus.SolarLoad = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_LOAD[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarLoad));
    }
    break;

  case 0x9144:
    if (config.km271.use_solar) {
      kmStatus.SolarWW = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_WW[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarWW));
    }
    break;

  case 0x9146:
    if (config.km271.use_solar) {
      kmStatus.SolarCollector = decodeNegValue(data[2]);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_COLLECTOR[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarCollector));
    }
    break;

  case 0x9147:
    if (config.km271.use_solar) {
      kmStatus.Solar9147 = data[2];
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_9147[config.mqtt.lang], EspStrUtil::intToString(kmStatus.Solar9147));
    }
    break;

  case 0x9148: // 0x9148 : Minutes (*65536)
    if (config.km271.use_solar) {
      kmStatus.SolarOperatingDuration_0 = data[2];
    }
    break;

  case 0x9149: // 0x9149 : Minutes (*256)
    if (config.km271.use_solar) {
      kmStatus.SolarOperatingDuration_1 = data[2];
    }
    break;

  case 0x914a: // 0x914a : Minutes (*1) + calculated sum of all 3 runtime values
    if (config.km271.use_solar) {
      kmStatus.SolarOperatingDuration_2 = data[2];
      kmStatus.SolarOperatingDuration_Sum =
          kmStatus.SolarOperatingDuration_2 + (kmStatus.SolarOperatingDuration_1 * 256) + (kmStatus.SolarOperatingDuration_0 * 65536);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_RUNTIME[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarOperatingDuration_Sum));
    }
    break;

  case 0xaa42: // 0xaa42 : Bitfeld
    kmStatus.ERR_Alarmstatus = data[2];
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_EXHAUST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_02[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_BOILER_FLOW[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_08[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_BURNER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_20[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_HC2_FLOW_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_80[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 7)));
    break;

  // undefined
  default:
    snprintf(tmpMessage, sizeof(tmpMessage), "%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x_%02x", data[0], data[1], data[2], data[3], data[4],
             data[5], data[6], data[7], data[8], data[9], data[10]);
    km271Msg(KM_TYP_UNDEF_MSG, tmpMessage, "");
    break;
  }
}

/**
 * *******************************************************************
 * @brief   Decodes KM271 temperatures with 0.5 C resolution
 * @details The value is provided in 0.5 steps
 * @param   data: the receive dat byte
 * @return  the decoded value as float
 * *******************************************************************/
float decode05cTemp(uint8_t data) { return ((float)data) / 2.0f; }

/**
 * *******************************************************************
 * @brief   Decodes KM271 values with negative range
 * @details Values >128 are negative
 * @param   data: the receive dat byte
 * @return  the decoded value as int8_t
 * ********************************************************************/
int8_t decodeNegValue(uint8_t data) {
  if (data > 128) {
    return (256 - data) * -1;
  } else {
    return data;
  }
}

/**
 * *******************************************************************
 * @brief   get a copy of the "status value structure"
 * @param   pDestS: The destination address the status shall be stored to
 * @return  none
 * *******************************************************************/
e_ret km271GetStatusValueCopy(s_km271_status *pDest) {
  memcpy(pDest, &kmStatus, sizeof(s_km271_status));
  return RET_OK;
}

/**
 * *******************************************************************
 * @brief   get a copy of the "config sttings structure"
 * @param   pDest: The destination address the status shall be stored to
 * @return  none
 * *******************************************************************/
e_ret km271GetConfigStringsCopy(s_km271_config_str *pDest) {
  memcpy(pDest, &kmConfigStr, sizeof(s_km271_config_str));
  return RET_OK;
}

/**
 * *******************************************************************
 * @brief   get a copy of the "alarm message structure"
 * @param   pDest: The destination address the status shall be stored to
 * @return  none
 * *******************************************************************/
e_ret km271GetAlarmMsgCopy(s_km271_alarm_str *pDest) {
  memcpy(pDest, &kmAlarmMsg, sizeof(s_km271_alarm_str));
  return RET_OK;
}

/**
 * *******************************************************************
 * @brief   get a copy of the "config value structure"
 * @param   pDest: The destination address the status shall be stored to
 * @return  none
 * *******************************************************************/
e_ret km271GetConfigNumValueCopy(s_km271_config_num *pDest) {
  memcpy(pDest, &kmConfigNum, sizeof(s_km271_config_num));
  return RET_OK;
}

/**
 * *******************************************************************
 * @brief   Retrieves the address of the Status Value Struct
 * @param   none
 * @return  Address of status structure
 * *******************************************************************/
s_km271_status *km271GetStatusValueAdr() { return &kmStatus; }

/**
 * *******************************************************************
 * @brief   Retrieves the address of the Config Value Struct
 * @param   none
 * @return  Address of config value structure
 * *******************************************************************/
s_km271_config_num *km271GetConfigValueAdr() { return &kmConfigNum; }

/**
 * *******************************************************************
 * @brief   Retrieves the address of the Config Strings Struct
 * @param   none
 * @return  Address of config strings structure
 * *******************************************************************/
s_km271_config_str *km271GetConfigStringsAdr() { return &kmConfigStr; }

/**
 * *******************************************************************
 * @brief   Retrieves the address of the Alarm messages Struct
 * @param   none
 * @return  Address of alarm messages structure
 * *******************************************************************/
s_km271_alarm_str *km271GetAlarmMsgAdr() { return &kmAlarmMsg; }

/**
 * *******************************************************************
 * @brief   return status of km271 refresh
 * @param   none
 * @return  status of km271 refresh
 * *******************************************************************/
bool km271GetRefreshState() { return km271RefreshActive; }

/**
 * *******************************************************************
 * @brief   Initializes the KM271 protocoll (based on 3964 protocol)
 * @details
 * @param   rxPin   The ESP32 RX pin number to be used for KM271 communication
 * @param   txPin   The ESP32 TX pin number to be used for KM271 communication
 * @return  e_ret error code
 * *******************************************************************/
e_ret km271ProtInit(int rxPin, int txPin) {
  Serial2.begin(KM271_BAUDRATE, SERIAL_8N1, rxPin, txPin); // Set serial port for communication with KM271/Ecomatic 2000

  Serial2.setTimeout(200); // set timeout for communication with KM271

  // initialize structs
  memset((void *)&kmStatus, 0, sizeof(s_km271_status));
  memset((void *)&kmConfigStr, 0, sizeof(s_km271_config_str));
  memset((void *)&kmConfigNum, 0, sizeof(s_km271_config_num));
  return RET_OK;
}

/**
 * *******************************************************************
 * @brief   build info structure ans send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendKM271Info() {
  if (kmInitDone) {
    JsonDocument infoJSON;
    infoJSON["burner"] = kmStatus.BurnerStates;
    infoJSON["pump"] = kmStatus.HC1_PumpPower;
    infoJSON["ww_temp"] = kmStatus.HotWaterActualTemp;
    infoJSON["boiler_temp"] = kmStatus.BoilerForwardActualTemp;
    char sendInfoJSON[255] = {'\0'};
    serializeJson(infoJSON, sendInfoJSON);
    km271Msg(KM_TYP_INFO, sendInfoJSON, "");
  }
}

/**
 * *******************************************************************
 * @brief   build debug structure ans send it via mqtt
 * @param   none
 * @return  none
 * *******************************************************************/
void sendKM271Debug() {
  JsonDocument infoJSON;
  infoJSON["logmode"] = kmSerialStats.logModeActive;
  infoJSON["send_cmd_busy"] = (send_buf[0] != 0);
  infoJSON["sw_version"] = VERSION;
  infoJSON["date-time"] = EspStrUtil::getDateTimeString();
  char sendInfoJSON[255] = {'\0'};
  serializeJson(infoJSON, sendInfoJSON);
  km271Msg(KM_TYP_KM_DEBUG, sendInfoJSON, "");
}

/**
 * *******************************************************************
 * @brief   set actual date and time to buderus
 * @param   dti: date and time info structure
 * @return  none
 * *******************************************************************/
void km271SetDateTimeDTI(tm dti) {
  char dateTimeInfo[128] = {'\0'}; // Date and time info String
  /* ---------------- INFO ---------------------------------
  dti.tm_year + 1900  // years since 1900
  dti.tm_mon + 1      // January = 0 (!)
  dti.tm_mday         // day of month
  dti.tm_hour         // hours since midnight  0-23
  dti.tm_min          // minutes after the hour  0-59
  dti.tm_sec          // seconds after the minute  0-61*
  dti.tm_wday         // days since Sunday 0-6
  dti.tm_isdst        // Daylight Saving Time flag
  --------------------------------------------------------- */
  send_buf[0] = 0x01;        // address
  send_buf[1] = 0x00;        // address
  send_buf[2] = dti.tm_sec;  // seconds
  send_buf[3] = dti.tm_min;  // minutes
  send_buf[4] = dti.tm_hour; // hours (bit 0-4)
  if (dti.tm_isdst > 0)
    send_buf[4] |= (1 << 6) & 0x40;                     // if time ist DST  (bit 6)
  send_buf[5] = dti.tm_mday;                            // day of month
  send_buf[6] = dti.tm_mon + 1;                         // month
  send_buf[6] |= (((dti.tm_wday + 6) % 7) << 4) & 0x70; // day of week (Logamatic: 0=monday...6=sunday / wday: 0=sunday...6=saturday)
  send_buf[7] = dti.tm_year;                            // year year < 100 means 19xx / year > 100 means 20xx

  char wday[4] = {'\0'};
  switch ((dti.tm_wday + 6) % 7) {
  case 0:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::MON[config.mqtt.lang]);
    break;
  case 1:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::TUE[config.mqtt.lang]);
    break;
  case 2:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::WED[config.mqtt.lang]);
    break;
  case 3:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::THU[config.mqtt.lang]);
    break;
  case 4:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::FRI[config.mqtt.lang]);
    break;
  case 5:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::SAT[config.mqtt.lang]);
    break;
  case 6:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::SUN[config.mqtt.lang]);
    break;
  default:
    break;
  }
  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%s: (%s) %02d.%02d.%d - %02i:%02i:%02i", MQTT_MSG::DATETIME_CHANGED[config.mqtt.lang], wday,
           dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec);
  km271Msg(KM_TYP_MESSAGE, dateTimeInfo, "");
}

/**
 * *******************************************************************
 * @brief   set actual date and time to buderus
 * @param   dti: date and time info structure
 * @return  none
 * *******************************************************************/
void km271SetDateTimeNTP() {
  char dateTimeInfo[128] = {'\0'}; // Date and time info String
  time_t now;                      // this is the epoch
  tm dti;                          // the structure tm holds time information in a more convient way
  time(&now);                      // read the current time
  localtime_r(&now, &dti);         // update the structure tm with the current time
  /* ---------------- INFO ---------------------------------
  dti.tm_year + 1900  // years since 1900
  dti.tm_mon + 1      // January = 0 (!)
  dti.tm_mday         // day of month
  dti.tm_hour         // hours since midnight  0-23
  dti.tm_min          // minutes after the hour  0-59
  dti.tm_sec          // seconds after the minute  0-61*
  dti.tm_wday         // days since Sunday 0-6
  dti.tm_isdst        // Daylight Saving Time flag
  --------------------------------------------------------- */
  send_buf[0] = 0x01;        // address
  send_buf[1] = 0x00;        // address
  send_buf[2] = dti.tm_sec;  // seconds
  send_buf[3] = dti.tm_min;  // minutes
  send_buf[4] = dti.tm_hour; // hours (bit 0-4)
  if (dti.tm_isdst > 0)
    send_buf[4] |= (1 << 6) & 0x40;                     // if time ist DST  (bit 6)
  send_buf[5] = dti.tm_mday;                            // day of month
  send_buf[6] = dti.tm_mon + 1;                         // month
  send_buf[6] |= (((dti.tm_wday + 6) % 7) << 4) & 0x70; // day of week (Logamatic: 0=monday...6=sunday / wday: 0=sunday...6=saturday)
  send_buf[7] = dti.tm_year;                            // year

  char wday[4] = {'\0'};
  switch ((dti.tm_wday + 6) % 7) {
  case 0:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::MON[config.mqtt.lang]);
    break;
  case 1:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::TUE[config.mqtt.lang]);
    break;
  case 2:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::WED[config.mqtt.lang]);
    break;
  case 3:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::THU[config.mqtt.lang]);
    break;
  case 4:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::FRI[config.mqtt.lang]);
    break;
  case 5:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::SAT[config.mqtt.lang]);
    break;
  case 6:
    snprintf(wday, sizeof(wday), "%s", MQTT_MSG::SUN[config.mqtt.lang]);
    break;
  default:
    break;
  }

  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%s: (%s) %02d.%02d.%d - %02i:%02i:%02i - DST:%d", MQTT_MSG::DATETIME_CHANGED[config.mqtt.lang], wday,
           dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec, (dti.tm_isdst > 0));
  km271Msg(KM_TYP_MESSAGE, dateTimeInfo, "");
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: parameter as integer
 * @return  none
 * *******************************************************************/
void km271sendCmd(e_km271_sendCmd sendCmd, int8_t cmdPara) {

  switch (sendCmd) {
  case KM271_SENDCMD_HC1_OPMODE:
    if (cmdPara >= 0 && cmdPara <= 2) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_OPMODE:
    if (cmdPara >= 0 && cmdPara <= 2) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_DESIGN_TEMP:
    if (cmdPara >= 30 && cmdPara <= 90) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x0E; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = cmdPara; // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_DESIGN_TEMP:
    if (cmdPara >= 30 && cmdPara <= 90) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x0E; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = cmdPara; // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_PROGRAMM:
    if (cmdPara >= 0 && cmdPara <= 8) {
      send_buf[0] = 0x11;    // Data-Type HK1 Schaltuhr
      send_buf[1] = 0x00;    // Offset
      send_buf[2] = cmdPara; // Programmnummer 0..8
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_HOLIDAYS:
    if (cmdPara >= 0 && cmdPara <= 99) {
      send_buf[0] = 0x11; // Data-Type HK1 Schaltuhr
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = cmdPara; // Holiday days -  Resolution: 1 Day - Range: 0 – 99 Days
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_PROGRAMM:
    if (cmdPara >= 0 && cmdPara <= 8) {
      send_buf[0] = 0x12;    // Data-Type
      send_buf[1] = 0x00;    // Offset
      send_buf[2] = cmdPara; // Programmnummer 0..8
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_HOLIDAYS:
    if (cmdPara >= 0 && cmdPara <= 99) {
      send_buf[0] = 0x12; // Data-Type HK2
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = cmdPara; // Resolution: 1 Day - Range: 0 – 99 Days
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_WW_OPMODE:
    if (cmdPara >= 0 && cmdPara <= 2) {
      send_buf[0] = 0x0C;    // Data-Type für Warmwasser (0x0C)
      send_buf[1] = 0x0E;    // Offset
      send_buf[2] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_SUMMER:
    if (cmdPara >= 9 && cmdPara <= 31) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = cmdPara; // 9:Winter | 10°-30° | 31:Summer
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_SUMMER:
    if (cmdPara >= 9 && cmdPara <= 31) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = cmdPara; // 9:Winter | 10°-30° | 31:Summer
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_FROST:
    if (cmdPara >= -20 && cmdPara <= 10) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x31; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_FROST:
    if (cmdPara >= -20 && cmdPara <= 10) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x31; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD:
    if (cmdPara >= -20 && cmdPara <= 10) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x15; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD:
    if (cmdPara >= -20 && cmdPara <= 10) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x15; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang],
               KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang], KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_WW_SETPOINT:
    if (cmdPara >= 30 && cmdPara <= 60) {
      send_buf[0] = 0x0C; // Data-Type für Warmwater (0x0C)
      send_buf[1] = 0x07; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = cmdPara; // 30°-60°
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_WW_PUMP_CYCLES:
    if (cmdPara >= 0 && cmdPara <= 7) {
      send_buf[0] = 0x0C; // Data-Type für Warmwater (0x0C)
      send_buf[1] = 0x0E; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = cmdPara; // 0:OFF - 1..6 - 7:ON
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_SWITCH_ON_TEMP:
    if (cmdPara >= 0 && cmdPara <= 10) {
      send_buf[0] = 0x07;    // Data-Type für HK1 (0x07)
      send_buf[1] = 0x15;    // Offset
      send_buf[2] = cmdPara; // Resolution: 1 °C - Range: 0 – 10 °C
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_SWITCH_ON_TEMP:
    if (cmdPara >= 0 && cmdPara <= 10) {
      send_buf[0] = 0x08;    // Data-Type für HK2 (0x08)
      send_buf[1] = 0x15;    // Offset
      send_buf[2] = cmdPara; // Resolution: 1 °C - Range: 0 – 10 °C
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_REDUCTION_MODE:
    if (cmdPara >= 0 && cmdPara <= 3) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x1c; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = cmdPara; // {"off", "fixed", "room", "outdoors"}
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_REDUCTION_MODE:
    if (cmdPara >= 0 && cmdPara <= 3) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x1c; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = cmdPara; // {"off", "fixed", "room", "outdoors"}
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %d", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
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
void km271sendCmdFlt(e_km271_sendCmd sendCmd, float cmdPara) {

  switch (sendCmd) {
  case KM271_SENDCMD_HC1_DAY_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_NIGHT_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_DAY_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_NIGHT_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = 0x65;
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC1_HOLIDAY_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  case KM271_SENDCMD_HC2_HOLIDAY_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      send_buf[0] = 0x08; // Data-Type für HK1 (0x08)
      send_buf[1] = 0x00; // Offset
      send_buf[2] = 0x65;
      send_buf[3] = 0x65;
      send_buf[4] = 0x65;
      send_buf[5] = 0x65;
      send_buf[6] = 0x65;
      send_buf[7] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::VALUE[config.mqtt.lang], cmdPara);
    } else {
      snprintf(kmTmpMsg, sizeof(kmTmpMsg), "%s: %s - %s: %.1f", KM_INFO_MSG::CMD[config.mqtt.lang], KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang],
               KM_INFO_MSG::INVALID[config.mqtt.lang], cmdPara);
    }
    km271Msg(KM_TYP_MESSAGE, kmTmpMsg, "");
    break;

  default:
    break;
  }
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: array of 8 hex parameter (pre checked)
 * @return  none
 * *******************************************************************/
void km271sendServiceCmd(uint8_t cmdPara[8]) {
  // service command with 8 pre checked hex parameters
  for (uint8_t i = 0; i < 8; i++) {
    send_buf[i] = cmdPara[i];
  }
}

/**
 * *******************************************************************
 * @brief   returns the status if LogMode is active
 * @param   none
 * @return  bool state of LogModeActive
 * *******************************************************************/
bool km271GetLogMode() { return kmSerialStats.logModeActive; }
unsigned long km271GetTxBytes() { return kmSerialStats.TxBytes; }
unsigned long km271GetRxBytes() { return kmSerialStats.RxBytes; }

/**
 * *******************************************************************
 * @brief   decode the timer values
 * @param
 * @return  string with timer information
 * *******************************************************************/
void decodeTimer(char *timerInfo, unsigned int size, uint8_t dateOnOff, uint8_t time) {

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

  bool onOff = dateOnOff & 0x0f;  // extract OnOff State
  uint8_t day = dateOnOff & 0xf0; // extract day
  char timeString[100] = {'\0'};
  char dayString[10] = {'\0'};
  char onOffString[10] = {'\0'};

  if (!(dateOnOff == 0xc2 && time == 0x90)) {
    switch (day) {
    case 0x00:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::MON[config.mqtt.lang]);
      break;
    case 0x20:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::TUE[config.mqtt.lang]);
      break;
    case 0x40:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::WED[config.mqtt.lang]);
      break;
    case 0x60:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::THU[config.mqtt.lang]);
      break;
    case 0x80:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::FRI[config.mqtt.lang]);
      break;
    case 0xa0:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::SAT[config.mqtt.lang]);
      break;
    case 0xc0:
      snprintf(dayString, sizeof(dayString), "%s", MQTT_MSG::SUN[config.mqtt.lang]);
      break;
    default:
      strncpy(dayString, "--", sizeof(dayString));
      break;
    }
    // add switch state
    if (onOff) {
      snprintf(onOffString, sizeof(onOffString), " (%s) ", MQTT_MSG::ON[config.mqtt.lang]);
    } else {
      snprintf(onOffString, sizeof(onOffString), " (%s) ", MQTT_MSG::OFF[config.mqtt.lang]);
    }
    // convert time data
    snprintf(timeString, sizeof(timeString), "%02d:%02d", (time / 6), (time % 6) * 10);
    // copy to output parameter
    snprintf(timerInfo, size, "%s %s %s", dayString, timeString, onOffString);
  } else {
    // not used
    strcpy(timerInfo, "--");
  }
}

/**
 * *******************************************************************
 * @brief   get the index of the error message for the given error number
 * @param   errorNr Error Number from Logamatic
 * @return  index for error message in errMsgText
 * *******************************************************************/
uint8_t getErrorTextIndex(uint8_t errorNr) {
  switch (errorNr) {
  case 0:
    return 0;
    break;
  case 2:
    return 1;
    break;
  case 3:
    return 2;
    break;
  case 4:
    return 3;
    break;
  case 8:
    return 4;
    break;
  case 9:
    return 5;
    break;
  case 10:
    return 6;
    break;
  case 11:
    return 7;
    break;
  case 12:
    return 8;
    break;
  case 15:
    return 9;
    break;
  case 16:
    return 10;
    break;
  case 20:
    return 11;
    break;
  case 24:
    return 12;
    break;
  case 30:
    return 13;
    break;
  case 31:
    return 14;
    break;
  case 32:
    return 15;
    break;
  case 33:
    return 16;
    break;
  case 49:
    return 17;
    break;
  case 50:
    return 18;
    break;
  case 51:
    return 19;
    break;
  case 52:
    return 20;
    break;
  case 53:
    return 21;
    break;
  case 54:
    return 22;
    break;
  case 55:
    return 23;
    break;
  case 56:
    return 24;
    break;
  case 87:
    return 25;
    break;
  case 92:
    return 26;
    break;
  default:
    return 27;
    break;
  }
}

/**
 * *******************************************************************
 * @brief   generate error message
 * @param   errorNr Error Number from Logamatic
 * @return  true = unackloaded alarm / false = acknowled alarm
 * *******************************************************************/
bool decodeErrorMsg(char *errorMsg, unsigned int size, uint8_t *data) {
  // no error
  if (data[2] == 0) {
    snprintf(errorMsg, size, "%s", errMsgText.idx[config.mqtt.lang][0]);
    return false;
  } else {
    // error already acknowledged
    if (data[6] != 0xFF) {
      // Aussenfuehler defekt (>> 16:31 -3 Tage | << 20:40 -2 Tage)
      snprintf(errorMsg, size, "%s (>> %02i:%02i -%i %s | << %02i:%02i -%i %s)", errMsgText.idx[config.mqtt.lang][getErrorTextIndex(data[2])],
               data[3], data[4], (data[5] + data[8]), MQTT_MSG::DAYS[config.mqtt.lang], data[6], data[7], data[8], MQTT_MSG::DAYS[config.mqtt.lang]);
      return false;
    }
    // unacknowledged error
    else {
      // example: Aussenfuehler defekt (>> 16:31 -3 Tage)
      snprintf(errorMsg, size, "%s (>> %02i:%02i -%i %s)", errMsgText.idx[config.mqtt.lang][getErrorTextIndex(data[2])], data[3], data[4], data[5],
               MQTT_MSG::DAYS[config.mqtt.lang]);
      return true;
    }
  }
}

/**
 * *******************************************************************
 * @brief   limit function
 * @param   lower lower limit
 * @param   value value that should be limited
 * @param   upper upper limit
 * @return  limited value
 * *******************************************************************/
uint8_t limit(uint8_t lower, uint8_t value, uint8_t upper) {
  if (value > upper)
    return upper;
  else if (value < lower)
    return lower;
  else
    return value;
}

/**
 * *******************************************************************
 * @brief   convert hex string to integer
 * @param   hex value as string
 * @return  int value
 * *******************************************************************/
int hexToInt(char hex[]) { return (int)strtol(hex, NULL, 16); }

/**
 * *******************************************************************
 * @brief   compare data with filter function
 * @param   filter string
 * @param   data data to compare
 * @param   dataSize size of data
 * @return  comparison result true/false
 * *******************************************************************/
int compareHexValues(const char *filter, uint8_t data[]) {
  // Kopie filter-Strings, um strtok zu verwenden
  char filterCopy[strlen(filter) + 1];
  strcpy(filterCopy, filter);

  // Tokenize den filter-String
  char *token = strtok(filterCopy, "_");

  // Iteriere durch die Tokens und vergleiche mit data-Array
  int i = 0; // Index für das data-Array
  while (token != NULL) {
    int hexValue;
    if (strcmp(token, "XX") == 0) {
      // Wildcard: Beliebiger Hexwert wird akzeptiert
      hexValue = data[i];
    } else {
      // Konvertiere Hex-String in Integer
      hexValue = hexToInt(token);
    }
    // Vergleiche mit dem aktuellen Wert im data-Array
    if (hexValue != data[i]) {
      return 0; // Hexwert stimmt nicht überein
    }
    // Nächstes Token und data-Array-Index
    token = strtok(NULL, "_");
    i++;
  }
  // Alle Hexwerte wurden gefunden
  return 1;
}

/**
 * *******************************************************************
 * @brief   send all km271 status values
 * @param   noen
 * @return  none
 * *******************************************************************/
void sendAllKmStatValues() {

  if (config.km271.use_hc1) {
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_OFFTIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_ONTIME_OPT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_AUTOMATIC[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_WW_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_SCREED_DRY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_FROST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV1_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_1, 7)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_SUMMER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_NO_COM_REMOTE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_REMOTE_ERR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_FLOW_SENS_ERR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_FLOW_AT_MAX[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OV2_EXT_SENS_ERR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC1_OperatingStates_2, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingForwardTargetTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_FLOW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingForwardActualTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ROOM_SETPOINT[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC1_RoomTargetTemp, 1));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ROOM_TEMP[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC1_RoomActualTemp, 1));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_ON_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_SwitchOnOptimizationTime));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_OFF_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_SwitchOffOptimizationTime));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_PUMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_PumpPower));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_MIXER[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_MixingValue));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurvePlus10));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurve0));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_HEAT_CURVE3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingCurveMinus10));
  }

  if (config.km271.use_hc2) {
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_OFFTIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_ONTIME_OPT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_AUTOMATIC[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_WW_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_SCREED_DRY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_FROST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV1_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_1, 7)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_SUMMER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_NO_COM_REMOTE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_REMOTE_ERR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_FLOW_SENS_ERR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_FLOW_AT_MAX[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OV2_EXT_SENS_ERR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HC2_OperatingStates_2, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingForwardTargetTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_FLOW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingForwardActualTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ROOM_SETPOINT[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC2_RoomTargetTemp, 1));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ROOM_TEMP[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.HC2_RoomActualTemp, 1));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_ON_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_SwitchOnOptimizationTime));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_OFF_TIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_SwitchOffOptimizationTime));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_PUMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_PumpPower));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_MIXER[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_MixingValue));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurvePlus10));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurve0));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_HEAT_CURVE3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingCurveMinus10));
  }

  if (config.km271.use_ww) {
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_AUTO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_DESINFECT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_RELOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_HOLIDAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_DESINFECT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_SENSOR[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_WW_STAY_COLD[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV1_ERR_ANODE[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_1, 7)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_LOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_MANUAL[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_RELOAD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 2)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_OFF_TIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 3)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_ON_TIME_OPT[config.mqtt.lang],
             EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 4)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_DAY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 5)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_HOT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 6)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_OV2_PRIO[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterOperatingStates_2, 7)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterTargetTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterActualTemp));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_ONTIME_OPT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterOptimizationTime));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_CHARGE[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 0)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_CIRC[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 1)));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_PUMP_SOLAR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.HotWaterPumpStates, 2)));
  }

  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BoilerForwardTargetTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BoilerForwardActualTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ON_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerSwitchOnTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_OFF_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerSwitchOffTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_BURNER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 0)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_SENSOR[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 1)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_AUX_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 2)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_STAY_COLD[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 3)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_GAS_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 4)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_EXHAUST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 5)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_SAFETY[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 6)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_ERR_EXT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerErrorStates, 7)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_GASTEST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 0)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_STAGE1[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 1)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PROTECT[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 2)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_ACTIVE[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 3)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PER_FREE[config.mqtt.lang],
           EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 4)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_PER_HIGH[config.mqtt.lang],
           EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 5)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_STATE_STAGE2[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.BoilerOperatingStates, 6)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_CONTROL[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerStates));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::EXHAUST_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ExhaustTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_1[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_0));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_2[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_1));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_3[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_2));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_LIFETIME_4[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BurnerOperatingDuration_Sum));

  if (config.oilmeter.use_virtual_meter && config.oilmeter.oil_density_kg_l != 0) {
    kmStatus.BurnerCalcOilConsumption =
        (double)kmStatus.BurnerOperatingDuration_Sum / 60 * config.oilmeter.consumption_kg_h / config.oilmeter.oil_density_kg_l;
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_CONSUMPTION[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.BurnerCalcOilConsumption, 1));
  }

  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::OUTSIDE_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.OutsideTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::OUTSIDE_TEMP_DAMPED[config.mqtt.lang], EspStrUtil::intToString(kmStatus.OutsideDampedTemp));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::VERSION_VK[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ControllerVersionMain));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::VERSION_NK[config.mqtt.lang], EspStrUtil::intToString(kmStatus.ControllerVersionSub));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::MODULE_ID[config.mqtt.lang], EspStrUtil::intToString(kmStatus.Modul));

  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_EXHAUST[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 0)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_02[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 1)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_BOILER_FLOW[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 2)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_08[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 3)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_BURNER[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 4)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_20[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 5)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_HC2_FLOW_SENS[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 6)));
  km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::ALARM_80[config.mqtt.lang], EspStrUtil::intToString(bitRead(kmStatus.ERR_Alarmstatus, 7)));

  if (config.km271.use_solar) {
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_LOAD[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarLoad));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_WW[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarWW));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_COLLECTOR[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarCollector));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_9147[config.mqtt.lang], EspStrUtil::intToString(kmStatus.Solar9147));
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_RUNTIME[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarOperatingDuration_Sum));
  }
}

void sendAllKmCfgValues() {

  if (config.km271.use_hc1) {
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang], kmConfigStr.hc1_summer_mode_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang], kmConfigStr.hc1_night_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang], kmConfigStr.hc1_day_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang], kmConfigStr.hc1_operation_mode);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang], kmConfigStr.hc1_holiday_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_MAX_TEMP[config.mqtt.lang], kmConfigStr.hc1_max_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang], kmConfigStr.hc1_interpretation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang], kmConfigStr.hc1_switch_on_temperature);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang], kmConfigStr.hc1_switch_off_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang], kmConfigStr.hc1_reduction_mode);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HEATING_SYSTEM[config.mqtt.lang], kmConfigStr.hc1_heating_system);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TEMP_OFFSET[config.mqtt.lang], kmConfigStr.hc1_temp_offset);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_REMOTECTRL[config.mqtt.lang], kmConfigStr.hc1_remotecontrol);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang], kmConfigStr.hc1_frost_protection_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang], kmConfigStr.hc1_program);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang], kmConfigStr.hc1_holiday_days);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER01[config.mqtt.lang], kmConfigStr.hc1_timer01);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER02[config.mqtt.lang], kmConfigStr.hc1_timer02);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER03[config.mqtt.lang], kmConfigStr.hc1_timer03);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER04[config.mqtt.lang], kmConfigStr.hc1_timer04);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER05[config.mqtt.lang], kmConfigStr.hc1_timer05);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER06[config.mqtt.lang], kmConfigStr.hc1_timer06);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER07[config.mqtt.lang], kmConfigStr.hc1_timer07);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER08[config.mqtt.lang], kmConfigStr.hc1_timer08);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER09[config.mqtt.lang], kmConfigStr.hc1_timer09);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER10[config.mqtt.lang], kmConfigStr.hc1_timer10);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER11[config.mqtt.lang], kmConfigStr.hc1_timer11);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER12[config.mqtt.lang], kmConfigStr.hc1_timer12);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER13[config.mqtt.lang], kmConfigStr.hc1_timer13);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_TIMER14[config.mqtt.lang], kmConfigStr.hc1_timer14);
  }

  if (config.km271.use_hc2) {
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang], kmConfigStr.hc2_summer_mode_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang], kmConfigStr.hc2_night_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang], kmConfigStr.hc2_day_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang], kmConfigStr.hc2_operation_mode);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang], kmConfigStr.hc2_holiday_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_MAX_TEMP[config.mqtt.lang], kmConfigStr.hc2_max_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang], kmConfigStr.hc2_interpretation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang], kmConfigStr.hc2_switch_on_temperature);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang], kmConfigStr.hc2_switch_off_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang], kmConfigStr.hc2_reduction_mode);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HEATING_SYSTEM[config.mqtt.lang], kmConfigStr.hc2_heating_system);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TEMP_OFFSET[config.mqtt.lang], kmConfigStr.hc2_temp_offset);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_REMOTECTRL[config.mqtt.lang], kmConfigStr.hc2_remotecontrol);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang], kmConfigStr.hc2_frost_protection_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang], kmConfigStr.hc2_program);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang], kmConfigStr.hc2_holiday_days);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER01[config.mqtt.lang], kmConfigStr.hc2_timer01);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER02[config.mqtt.lang], kmConfigStr.hc2_timer02);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER03[config.mqtt.lang], kmConfigStr.hc2_timer03);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER04[config.mqtt.lang], kmConfigStr.hc2_timer04);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER05[config.mqtt.lang], kmConfigStr.hc2_timer05);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER06[config.mqtt.lang], kmConfigStr.hc2_timer06);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER07[config.mqtt.lang], kmConfigStr.hc2_timer07);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER08[config.mqtt.lang], kmConfigStr.hc2_timer08);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER09[config.mqtt.lang], kmConfigStr.hc2_timer09);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER10[config.mqtt.lang], kmConfigStr.hc2_timer10);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER11[config.mqtt.lang], kmConfigStr.hc2_timer11);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER12[config.mqtt.lang], kmConfigStr.hc2_timer12);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER13[config.mqtt.lang], kmConfigStr.hc2_timer13);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_TIMER14[config.mqtt.lang], kmConfigStr.hc2_timer14);
  }

  if (config.km271.use_ww) {
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PRIO[config.mqtt.lang], kmConfigStr.ww_priority);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang], kmConfigStr.ww_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang], kmConfigStr.ww_operation_mode);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PROCESSING[config.mqtt.lang], kmConfigStr.ww_processing);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang], kmConfigStr.ww_circulation);
  }

  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BUILDING_TYP[config.mqtt.lang], kmConfigStr.building_type);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::LANGUAGE[config.mqtt.lang], kmConfigStr.language);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SCREEN[config.mqtt.lang], kmConfigStr.display);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_TYP[config.mqtt.lang], kmConfigStr.burner_type);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::MAX_BOILER_TEMP[config.mqtt.lang], kmConfigStr.max_boiler_temperature);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::PUMP_LOGIC[config.mqtt.lang], kmConfigStr.pump_logic_temp);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::EXHAUST_THRESHOLD[config.mqtt.lang], kmConfigStr.exhaust_gas_temperature_threshold);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_MIN_MOD[config.mqtt.lang], kmConfigStr.burner_min_modulation);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::BURNER_MOD_TIME[config.mqtt.lang], kmConfigStr.burner_modulation_runtime);
  km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::TIME_OFFSET[config.mqtt.lang], kmConfigStr.time_offset);

  if (config.km271.use_solar) {
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_TEMP_MIN[config.mqtt.lang], kmConfigStr.solar_min);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_TEMP_MAX[config.mqtt.lang], kmConfigStr.solar_max);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_ENABLE[config.mqtt.lang], kmConfigStr.solar_activation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::SOLAR_OPMODE[config.mqtt.lang], kmConfigStr.solar_operation_mode);
  }

  if (config.km271.use_alarmMsg) {
    km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_1[config.mqtt.lang],
             kmAlarmMsg.alarm1); // acknowledged alarm (History)
    km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_2[config.mqtt.lang],
             kmAlarmMsg.alarm2); // acknowledged alarm (History)
    km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_3[config.mqtt.lang],
             kmAlarmMsg.alarm3); // acknowledged alarm (History)
    km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_4[config.mqtt.lang],
             kmAlarmMsg.alarm4); // acknowledged alarm (History)
  }
}