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
#include <km271WriteQueue.h>
#include <language.h>
#include <message.h>

/* P R O T O T Y P E S ********************************************************/
void sendTxBlock(uint8_t *data, int len);
static void applyAcknowledgedConfig(const uint8_t data[8]);
static void updateTimerBlock(uint8_t circuit, uint8_t block, const uint8_t *bytes);
void handleRxBlock(uint8_t *data, int len, uint8_t bcc);
void parseInfo(uint8_t *data, int len);
float decode05cTemp(uint8_t data);
int8_t decodeNegValue(uint8_t data);
void decodeTimer(char *timerInfo, unsigned int size, uint8_t dateOnOff, uint8_t time);
bool decodeErrorMsg(char *errorMsg, unsigned int size, const uint8_t *data, uint8_t language);
uint8_t getErrorTextIndex(uint8_t errorNr);
uint8_t limit(uint8_t lower, uint8_t value, uint8_t upper);
int compareHexValues(const char *filter, uint8_t data[]);
KmSerialStats kmSerialStats = {0, 0, 0, false};

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
static uint8_t alarmTelegrams[4][9] = {}; // Raw alarms for language-independent WebUI formatting
static uint8_t alarmsReceived = 0;
static uint8_t alarmsCurrent = 0;

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
static Km271WriteQueue writeQueue;
static Km271WriteQueue::Telegram activeWrite = {};
static bool writeActive = false;
static uint8_t writeRetries = 0;
static size_t requestAcknowledged = 0;
static uint32_t handshakeStarted = 0;
static constexpr uint32_t KM271_ACK_TIMEOUT = 2000;
static constexpr uint8_t KM271_WRITE_RETRIES = 2;
static char kmTmpMsg[256];
static bool kmInitDone = false;
// Bits 0..11: six temperature pairs; bits 12/13: burner / HC1 WW priority.
static uint16_t chartValuesReceived = 0;
static bool km271RefreshActive = false;

// WebSocket callers run on AsyncTCP; MQTT and the serial state machine run
// on loop(). Only queue operations/session admission are inside this lock.
static portMUX_TYPE sendBufMux = portMUX_INITIALIZER_UNLOCKED;
static bool writesAccepted = false;
static KmTimerSnapshot timerSnapshots[2];
static int8_t timerWriteCircuit = -1;
static uint32_t timerSession = 0;

const KmTimerSnapshot &km271GetTimers(uint8_t circuit) { return timerSnapshots[circuit == 1 ? 1 : 0]; }
uint32_t km271TimerSession() { return timerSession; }
bool km271TimersBusy() { return timerWriteCircuit >= 0; }

static bool timerAddress(const uint8_t *data) {
  return (data[0] == 0x11 || data[0] == 0x12) && data[1] >= 7 &&
         data[1] <= 98 && data[1] % 7 == 0;
}

const char *km271WriteTimers(uint8_t circuit, uint32_t session, uint32_t revision,
                           uint32_t request, const Km271Timers::Change &change) {
  if (circuit > 1 || !(circuit == 0 ? config.km271.use_hc1 : config.km271.use_hc2)) return "timer_disabled";
  if (!writesAccepted) return "timer_offline";
  if (timerWriteCircuit >= 0 || writeQueue.size()) return "timer_busy";
  KmTimerSnapshot &snapshot = timerSnapshots[circuit];
  if (snapshot.valid != Km271Timers::Complete) return "timer_loading";
  if (session != timerSession || revision != snapshot.revision) return "timer_stale";
  uint8_t output[Km271Timers::Bytes];
  const char *error = Km271Timers::apply(snapshot.raw, change, output);
  if (error) return error;
  uint8_t telegrams[Km271Timers::Blocks][8];
  uint8_t count = 0;
  // FHEM order: the later block first, preserving every untouched neighbour.
  for (int block = Km271Timers::Blocks - 1; block >= 0; --block) {
    if (!memcmp(snapshot.raw + block * 6, output + block * 6, 6)) continue;
    telegrams[count][0] = 0x11 + circuit;
    telegrams[count][1] = 7 * (block + 1);
    memcpy(telegrams[count++] + 2, output + block * 6, 6);
  }
  if (count) {
    portENTER_CRITICAL(&sendBufMux);
    const bool queued = writesAccepted && writeQueue.size() == 0 && writeQueue.enqueue(telegrams, count);
    if (queued) timerWriteCircuit = circuit;
    portEXIT_CRITICAL(&sendBufMux);
    if (!queued) return "timer_busy";
  }
  snapshot.request = request;
  snapshot.state = count ? KmTimerWriteState::Pending : KmTimerWriteState::Saved;
  km271Msg(KM_TYP_MESSAGE, circuit == 0 ? "Befehl: HK1_Schaltzeiten" : "Befehl: HK2_Schaltzeiten", "");
  return nullptr;
}

bool km271QueueWrite(const uint8_t (*telegrams)[8], size_t count) {
  // Timer writes require fresh neighbour bytes and the validated timer API.
  // Raw service/MQTT commands cannot bypass it or interleave a timer request.
  if (!telegrams || !count || count > KM271_WRITE_QUEUE_CAPACITY) return false;
  for (size_t i = 0; i < count; ++i) if (timerAddress(telegrams[i])) return false;
  portENTER_CRITICAL(&sendBufMux);
  const bool connected = writesAccepted;
  const bool queued = connected && timerWriteCircuit < 0 && writeQueue.enqueue(telegrams, count);
  portEXIT_CRITICAL(&sendBufMux);
  if (queued) {
    km271Msg(KM_TYP_DEBUG, "write request queued", "");
  } else {
    km271Msg(KM_TYP_MESSAGE, connected ? "write request rejected: invalid request or queue full"
                                     : "write request rejected: Logamatic not connected", "");
  }
  return queued;
}

static bool commitSendBuf(const uint8_t buf[8]) {
  return km271QueueWrite(reinterpret_cast<const uint8_t (*)[8]>(buf), 1);
}

// A lost/ambiguous handshake invalidates all pending requests: never replay old
// writes after reconnect. Keep cached values, but invalidate their live status.
static void restartKmLogging(const char *reason) {
  portENTER_CRITICAL(&sendBufMux);
  writesAccepted = false;
  const size_t discarded = writeQueue.size();
  writeQueue.clear();
  if (timerWriteCircuit >= 0) timerSnapshots[timerWriteCircuit].state = KmTimerWriteState::Failed;
  timerWriteCircuit = -1;
  portEXIT_CRITICAL(&sendBufMux);
  if (reason && (discarded || kmSerialStats.logModeActive)) {
    char message[192];
    if (discarded) {
      snprintf(message, sizeof(message), "write aborted: %u pending, %u acknowledged; %s",
               static_cast<unsigned>(discarded), static_cast<unsigned>(requestAcknowledged), reason);
      km271Msg(KM_TYP_MESSAGE, message, "");
    } else {
      km271Msg(KM_TYP_MESSAGE, reason, "");
    }
  }
  writeActive = false;
  writeRetries = 0;
  requestAcknowledged = 0;
  kmSerialStats.logModeActive = false;
  chartValuesReceived = 0;
  alarmsCurrent = 0;
  for (auto &snapshot : timerSnapshots) {
    if (snapshot.valid) ++snapshot.revision;
    snapshot.valid = 0;
  }
  if (KmRxBlockState != KM_TSK_START) ++timerSession;
  km271RefreshActive = false;
  KmRxBlockState = KM_TSK_START;
  kmRxStatus = KM_RX_RESYNC;
}

static void rejectKmWrite() {
  // NAK explicitly rejects this transfer. Retry the retained bytes only; never
  // substitute another queued telegram. Later independent requests may proceed.
  if (writeRetries < KM271_WRITE_RETRIES) {
    ++writeRetries;
    KmRxBlockState = KM_TSK_LOGGING;
    return;
  }
  if (timerWriteCircuit >= 0) {
    restartKmLogging("timer write rejected; reload complete timer state before retry");
    return;
  }
  portENTER_CRITICAL(&sendBufMux);
  writeQueue.discardRequest();
  portEXIT_CRITICAL(&sendBufMux);
  char message[128];
  snprintf(message, sizeof(message), "write request aborted after NAK retries: %u acknowledged; remaining request discarded",
           static_cast<unsigned>(requestAcknowledged));
  km271Msg(KM_TYP_MESSAGE, message, "");
  writeActive = false;
  writeRetries = 0;
  requestAcknowledged = 0;
  KmRxBlockState = KM_TSK_LOGGING;
}

/**
 * *******************************************************************
 * @brief   Main Handling of KM271
 * @details receice serial data and call all other function
 * @param   none
 * @return  none
 * *******************************************************************/
void cyclicKM271() {
  static uint8_t enabledCircuits = 0xff;
  const uint8_t enabled = uint8_t(config.km271.use_hc1) | (uint8_t(config.km271.use_hc2) << 1);
  if (enabledCircuits != 0xff && enabled != enabledCircuits)
    restartKmLogging("heating circuit configuration changed; reload timer state");
  enabledCircuits = enabled;
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - kmSerialStats.lastRxTime) > KM271_RX_TIMEOUT) {
    restartKmLogging("no data from Logamatic - serial link considered down");
  } else if ((KmRxBlockState == KM_TSK_LG_READY || KmRxBlockState == KM_TSK_LG_CMD ||
              KmRxBlockState == KM_TSK_WRITE_READY || KmRxBlockState == KM_TSK_WRITE_ACK) &&
             now - handshakeStarted >= KM271_ACK_TIMEOUT) {
    restartKmLogging("Logamatic handshake timeout; result unknown");
  }

  if (Serial2.readBytes(&rxByte, 1)) { // Wait for RX byte, if timeout, just loop and read again

    kmSerialStats.RxBytes++;             // increase received bytes
    kmSerialStats.lastRxTime = millis(); // liveness of the serial link

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
      kmRxStatus = KM_RX_IDLE; // Handler may explicitly request resynchronisation.
      if (!kmRxBcc) {                                    // Block is valid
        handleRxBlock(kmRxBuf.buf, kmRxBuf.len, rxByte); // Handle RX block, provide BCC for debug logging, too
      } else {
        sendTxBlock(KmCNAK, sizeof(KmCNAK)); // Send NAK, ask for re-sending the block
      }
      break;
    } // end-case
  }   // end-if

  if (!config.km271.use_alarmMsg) alarmsCurrent = 0;

  // A parameter transfer does not end the established logging session.
  kmSerialStats.logModeActive = (KmRxBlockState == KM_TSK_LOGGING ||
                                 KmRxBlockState == KM_TSK_WRITE_READY ||
                                 KmRxBlockState == KM_TSK_WRITE_ACK);
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
  if (len <= 0) return;
  const bool stx = (len == 1 && data[0] == KM_STX);
  const bool dle = (len == 1 && data[0] == KM_DLE);
  const bool nak = (len == 1 && data[0] == KM_NAK);

  switch (KmRxBlockState) {
  case KM_TSK_START:
    if (stx) {
      sendTxBlock(KmCSTX, sizeof(KmCSTX));
      handshakeStarted = millis();
      KmRxBlockState = KM_TSK_LG_READY;
    }
    break;
  case KM_TSK_LG_READY:
    if (dle) {
      sendTxBlock(KmCLogMode, sizeof(KmCLogMode));
      handshakeStarted = millis();
      KmRxBlockState = KM_TSK_LG_CMD;
    } else {
      restartKmLogging(nullptr);
    }
    break;
  case KM_TSK_LG_CMD:
    if (dle) {
      alarmsCurrent = 0;
      km271RefreshActive = true;
      KmRxBlockState = KM_TSK_LOGGING;
      portENTER_CRITICAL(&sendBufMux);
      writesAccepted = true;
      portEXIT_CRITICAL(&sendBufMux);
    } else {
      restartKmLogging(nullptr);
    }
    break;
  case KM_TSK_LOGGING:
    if (stx) {
      if (!writeActive) {
        portENTER_CRITICAL(&sendBufMux);
        writeActive = writeQueue.front(activeWrite);
        portEXIT_CRITICAL(&sendBufMux);
        writeRetries = 0;
      }
      if (writeActive) {
        sendTxBlock(KmCSTX, sizeof(KmCSTX));
        handshakeStarted = millis();
        KmRxBlockState = KM_TSK_WRITE_READY;
      } else {
        sendTxBlock(KmCDLE, sizeof(KmCDLE));
      }
    } else if (!dle && !nak) {
      // Only payload blocks reach the decoder, never single control bytes.
      if (len >= 3) parseInfo(data, len);
      sendTxBlock(KmCDLE, sizeof(KmCDLE));
    }
    break;
  case KM_TSK_WRITE_READY:
    if (dle) {
      sendTxBlock(activeWrite.data, sizeof(activeWrite.data));
      handshakeStarted = millis();
      KmRxBlockState = KM_TSK_WRITE_ACK;
    } else if (nak) {
      rejectKmWrite();
    } else {
      restartKmLogging("unexpected response to write request");
    }
    break;
  case KM_TSK_WRITE_ACK:
    if (dle) {
      applyAcknowledgedConfig(activeWrite.data);
      portENTER_CRITICAL(&sendBufMux);
      writeQueue.pop();
      portEXIT_CRITICAL(&sendBufMux);
      ++requestAcknowledged;
      if (activeWrite.last) {
        if (timerWriteCircuit >= 0) {
          timerSnapshots[timerWriteCircuit].state = KmTimerWriteState::Saved;
          timerWriteCircuit = -1;
        }
        char message[96];
        snprintf(message, sizeof(message), "write request acknowledged: %u telegram(s)",
                 static_cast<unsigned>(requestAcknowledged));
        km271Msg(KM_TYP_DEBUG, message, "");
        requestAcknowledged = 0;
      }
      writeActive = false;
      writeRetries = 0;
      KmRxBlockState = KM_TSK_LOGGING;
    } else if (nak) {
      rejectKmWrite();
    } else {
      restartKmLogging("unexpected write acknowledgement; result unknown");
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

// Shared by real receive telegrams and acknowledged local writes. Updating
// one field keeps numerical cache, formatted WebUI text and MQTT consistent.
static void updateKmConfigField(uint16_t reg, uint8_t offset, uint8_t value) {
  switch ((static_cast<uint32_t>(reg) << 8) | offset) {
  case 0x000001: // 0000:1 hc1_summer_mode_threshold
    kmConfigNum.hc1_summer_mode_threshold = value;
    snprintf(kmConfigStr.hc1_summer_mode_threshold, sizeof(kmConfigStr.hc1_summer_mode_threshold), "%s",
             KM_CFG_ARRAY::SUMMER[config.mqtt.lang][limit(0, kmConfigNum.hc1_summer_mode_threshold - 9, 22)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc1_summer_mode_threshold); // "CFG_Sommer_ab"            => "0000:1,p:-9,a"
    break;
  case 0x000002: // 0000:2 hc1_night_temp
    kmConfigNum.hc1_night_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc1_night_temp, sizeof(kmConfigStr.hc1_night_temp), "%0.1f °C", kmConfigNum.hc1_night_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_NIGHT_TEMP[config.mqtt.lang],
             kmConfigStr.hc1_night_temp); // "CFG_HK1_Nachttemperatur"  => "0000:2,d:2"
    break;
  case 0x000003: // 0000:3 hc1_day_temp
    kmConfigNum.hc1_day_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc1_day_temp, sizeof(kmConfigStr.hc1_day_temp), "%0.1f °C", kmConfigNum.hc1_day_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_DAY_TEMP[config.mqtt.lang],
             kmConfigStr.hc1_day_temp); // "CFG_HK1_Tagtemperatur"     => "0000:3,d:2"
    break;
  case 0x000004: // 0000:4 hc1_operation_mode
    kmConfigNum.hc1_operation_mode = value;
    snprintf(kmConfigStr.hc1_operation_mode, sizeof(kmConfigStr.hc1_operation_mode), "%s",
             KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.hc1_operation_mode, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_OPMODE[config.mqtt.lang],
             kmConfigStr.hc1_operation_mode); // "CFG_HK1_Betriebsart"       => "0000:4,a:4"
    break;
  case 0x000005: // 0000:5 hc1_holiday_temp
    kmConfigNum.hc1_holiday_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc1_holiday_temp, sizeof(kmConfigStr.hc1_holiday_temp), "%0.1f °C", kmConfigNum.hc1_holiday_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[config.mqtt.lang],
             kmConfigStr.hc1_holiday_temp); // "CFG_HK1_Urlaubtemperatur"   => "0000:5,d:2"
    break;
  case 0x000e04: // 000e:4 hc1_interpretation
    kmConfigNum.hc1_interpretation = value;
    snprintf(kmConfigStr.hc1_interpretation, sizeof(kmConfigStr.hc1_interpretation), "%i °C", kmConfigNum.hc1_interpretation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_INTERPR[config.mqtt.lang],
             kmConfigStr.hc1_interpretation); // CFG_HK1_Auslegung"          => "000e:4"
    break;
  case 0x001500: // 0015:0 hc1_switch_on_temperature
    kmConfigNum.hc1_switch_on_temperature = value;
    snprintf(kmConfigStr.hc1_switch_on_temperature, sizeof(kmConfigStr.hc1_switch_on_temperature), "%s",
             KM_CFG_ARRAY::SWITCH_ON_TEMP[config.mqtt.lang][limit(0, kmConfigNum.hc1_switch_on_temperature, 10)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[config.mqtt.lang],
             kmConfigStr.hc1_switch_on_temperature); // "CFG_HK1_Aufschalttemperatur"  => "0015:0,a"
    break;
  case 0x001502: // 0015:2 hc1_switch_off_threshold
    kmConfigNum.hc1_switch_off_threshold = decodeNegValue(value);
    snprintf(kmConfigStr.hc1_switch_off_threshold, sizeof(kmConfigStr.hc1_switch_off_threshold), "%i °C", kmConfigNum.hc1_switch_off_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc1_switch_off_threshold); // CFG_HK1_Aussenhalt_ab"         => "0015:2,s"
    break;
  case 0x001c01: // 001c:1 hc1_reduction_mode
    kmConfigNum.hc1_reduction_mode = value;
    snprintf(kmConfigStr.hc1_reduction_mode, sizeof(kmConfigStr.hc1_reduction_mode), "%s",
             KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][limit(0, kmConfigNum.hc1_reduction_mode, 3)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_REDUCTION_MODE[config.mqtt.lang],
             kmConfigStr.hc1_reduction_mode); // "CFG_HK1_Absenkungsart"    => "001c:1,a"
    break;
  case 0x003105: // 0031:5 hc1_frost_protection_threshold
    kmConfigNum.hc1_frost_protection_threshold = decodeNegValue(value);
    snprintf(kmConfigStr.hc1_frost_protection_threshold, sizeof(kmConfigStr.hc1_frost_protection_threshold), "%i °C",
             kmConfigNum.hc1_frost_protection_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_FROST_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc1_frost_protection_threshold); // "CFG_Frost_ab"                 => "0031:5,s"
    break;
  case 0x010000: // 0100:0 hc1_program
    kmConfigNum.hc1_program = value;
    snprintf(kmConfigStr.hc1_program, sizeof(kmConfigStr.hc1_program), "%s",
             KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][limit(0, kmConfigNum.hc1_program, 8)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_PROGRAM[config.mqtt.lang],
             kmConfigStr.hc1_program); // "CFG_HK1_Programm"  => "0100:0"
    break;
  case 0x010003: // 0100:3 hc1_holiday_days
    kmConfigNum.hc1_holiday_days = value;
    snprintf(kmConfigStr.hc1_holiday_days, sizeof(kmConfigStr.hc1_holiday_days), "%i %s", kmConfigNum.hc1_holiday_days,
             MQTT_MSG::DAYS[config.mqtt.lang]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[config.mqtt.lang],
             kmConfigStr.hc1_holiday_days); // "CFG_HK1_Ferien_Tage"  => "0100:5"
    break;
  case 0x003801: // 0038:1 hc2_summer_mode_threshold
    kmConfigNum.hc2_summer_mode_threshold = value;
    snprintf(kmConfigStr.hc2_summer_mode_threshold, sizeof(kmConfigStr.hc2_summer_mode_threshold), "%s",
             KM_CFG_ARRAY::SUMMER[config.mqtt.lang][limit(0, kmConfigNum.hc2_summer_mode_threshold - 9, 22)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc2_summer_mode_threshold); // "CFG_Sommer_ab"            => "0038:1,p:-9,a"
    break;
  case 0x003802: // 0038:2 hc2_night_temp
    kmConfigNum.hc2_night_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc2_night_temp, sizeof(kmConfigStr.hc2_night_temp), "%0.1f °C", kmConfigNum.hc2_night_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_NIGHT_TEMP[config.mqtt.lang],
             kmConfigStr.hc2_night_temp); // "CFG_HK2_Nachttemperatur"   => "0038:2,d:2"
    break;
  case 0x003803: // 0038:3 hc2_day_temp
    kmConfigNum.hc2_day_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc2_day_temp, sizeof(kmConfigStr.hc2_day_temp), "%0.1f °C", kmConfigNum.hc2_day_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_DAY_TEMP[config.mqtt.lang],
             kmConfigStr.hc2_day_temp); // "CFG_HK2_Tagtemperatur"     => "0038:3,d:2"
    break;
  case 0x003804: // 0038:4 hc2_operation_mode
    kmConfigNum.hc2_operation_mode = value;
    snprintf(kmConfigStr.hc2_operation_mode, sizeof(kmConfigStr.hc2_operation_mode), "%s",
             KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.hc2_operation_mode, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_OPMODE[config.mqtt.lang],
             kmConfigStr.hc2_operation_mode); // "CFG_HK2_Betriebsart"       => "0038:4,a:4"
    break;
  case 0x003805: // 0038:5 hc2_holiday_temp
    kmConfigNum.hc2_holiday_temp = decode05cTemp(value);
    snprintf(kmConfigStr.hc2_holiday_temp, sizeof(kmConfigStr.hc2_holiday_temp), "%0.1f °C", kmConfigNum.hc2_holiday_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[config.mqtt.lang],
             kmConfigStr.hc2_holiday_temp); // "CFG_HK2_Urlaubtemperatur"  => "0038:5,d:2"
    break;
  case 0x004604: // 0046:4 hc2_interpretation
    kmConfigNum.hc2_interpretation = value;
    snprintf(kmConfigStr.hc2_interpretation, sizeof(kmConfigStr.hc2_interpretation), "%i °C", kmConfigNum.hc2_interpretation);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_INTERPR[config.mqtt.lang],
             kmConfigStr.hc2_interpretation); // "CFG_HK2_Auslegung"         => "0046:4"
    break;
  case 0x004d00: // 004d:0 hc2_switch_on_temperature
    kmConfigNum.hc2_switch_on_temperature = value;
    snprintf(kmConfigStr.hc2_switch_on_temperature, sizeof(kmConfigStr.hc2_switch_on_temperature), "%s",
             KM_CFG_ARRAY::SWITCH_ON_TEMP[config.mqtt.lang][limit(0, kmConfigNum.hc2_switch_on_temperature, 10)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[config.mqtt.lang],
             kmConfigStr.hc2_switch_on_temperature); // "CFG_HK1_Aufschalttemperatur"  => "004d:0,a"
    break;
  case 0x004d02: // 004d:2 hc2_switch_off_threshold
    kmConfigNum.hc2_switch_off_threshold = decodeNegValue(value);
    snprintf(kmConfigStr.hc2_switch_off_threshold, sizeof(kmConfigStr.hc2_switch_off_threshold), "%i °C", kmConfigNum.hc2_switch_off_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc2_switch_off_threshold); // CFG_HK1_Aussenhalt_ab"         => "004d:2,s"
    break;
  case 0x005401: // 0054:1 hc2_reduction_mode
    kmConfigNum.hc2_reduction_mode = value;
    snprintf(kmConfigStr.hc2_reduction_mode, sizeof(kmConfigStr.hc2_reduction_mode), "%s",
             KM_CFG_ARRAY::REDUCT_MODE[config.mqtt.lang][limit(0, kmConfigNum.hc2_reduction_mode, 3)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_REDUCTION_MODE[config.mqtt.lang],
             kmConfigStr.hc2_reduction_mode); // "CFG_HK1_Absenkungsart"    => "0054:1,a"
    break;
  case 0x006905: // 0069:5 hc2_frost_protection_threshold
    kmConfigNum.hc2_frost_protection_threshold = decodeNegValue(value);
    snprintf(kmConfigStr.hc2_frost_protection_threshold, sizeof(kmConfigStr.hc2_frost_protection_threshold), "%i °C",
             kmConfigNum.hc2_frost_protection_threshold);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_FROST_THRESHOLD[config.mqtt.lang],
             kmConfigStr.hc2_frost_protection_threshold); // "CFG_Frost_ab"                 => "0069:5,s"
    break;
  case 0x016900: // 0169:0 hc2_program
    kmConfigNum.hc2_program = value;
    snprintf(kmConfigStr.hc2_program, sizeof(kmConfigStr.hc2_program), "%s",
             KM_CFG_ARRAY::HC_PROGRAM[config.mqtt.lang][limit(0, kmConfigNum.hc2_program, 8)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_PROGRAM[config.mqtt.lang],
             kmConfigStr.hc2_program); // "CFG_HK2_Programm"  => "0169:0"
    break;
  case 0x016903: // 0169:3 hc2_holiday_days
    kmConfigNum.hc2_holiday_days = value;
    snprintf(kmConfigStr.hc2_holiday_days, sizeof(kmConfigStr.hc2_holiday_days), "%i %s", kmConfigNum.hc2_holiday_days,
             MQTT_MSG::DAYS[config.mqtt.lang]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[config.mqtt.lang],
             kmConfigStr.hc2_holiday_days); // "CFG_HK2_Ferien_Tage"  => "0169:5"
    break;
  case 0x007e03: // 007e:3 ww_temp
    kmConfigNum.ww_temp = value;
    snprintf(kmConfigStr.ww_temp, sizeof(kmConfigStr.ww_temp), "%i °C", kmConfigNum.ww_temp);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_TEMP[config.mqtt.lang],
             kmConfigStr.ww_temp); // "CFG_WW_Temperatur"  => "007e:3"
    break;
  case 0x008500: // 0085:0 ww_operation_mode
    kmConfigNum.ww_operation_mode = value;
    snprintf(kmConfigStr.ww_operation_mode, sizeof(kmConfigStr.ww_operation_mode), "%s",
             KM_CFG_ARRAY::OPMODE[config.mqtt.lang][limit(0, kmConfigNum.ww_operation_mode, 2)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_OPMODE[config.mqtt.lang],
             kmConfigStr.ww_operation_mode); // "CFG_WW_Betriebsart"  => "0085:0,a"
    break;
  case 0x008505: // 0085:5 ww_circulation
    kmConfigNum.ww_circulation = value;
    snprintf(kmConfigStr.ww_circulation, sizeof(kmConfigStr.ww_circulation), "%s",
             KM_CFG_ARRAY::CIRC_INTERVAL[config.mqtt.lang][limit(0, kmConfigNum.ww_circulation, 7)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_CIRCULATION[config.mqtt.lang],
             kmConfigStr.ww_circulation); // "CFG_WW_Zirkulation"   => "0085:5,a"
    break;
  }
}

static void updateTimerBlock(uint8_t circuit, uint8_t block, const uint8_t *bytes) {
  if (circuit > 1 || block >= Km271Timers::Blocks) return;
  KmTimerSnapshot &snapshot = timerSnapshots[circuit];
  const uint16_t bit = 1U << block;
  if (!(snapshot.valid & bit) || memcmp(snapshot.raw + block * 6, bytes, 6)) ++snapshot.revision;
  memcpy(snapshot.raw + block * 6, bytes, 6);
  snapshot.valid |= bit;
  char *texts[2][14] = {
    {kmConfigStr.hc1_timer01, kmConfigStr.hc1_timer02, kmConfigStr.hc1_timer03, kmConfigStr.hc1_timer04, kmConfigStr.hc1_timer05, kmConfigStr.hc1_timer06, kmConfigStr.hc1_timer07, kmConfigStr.hc1_timer08, kmConfigStr.hc1_timer09, kmConfigStr.hc1_timer10, kmConfigStr.hc1_timer11, kmConfigStr.hc1_timer12, kmConfigStr.hc1_timer13, kmConfigStr.hc1_timer14},
    {kmConfigStr.hc2_timer01, kmConfigStr.hc2_timer02, kmConfigStr.hc2_timer03, kmConfigStr.hc2_timer04, kmConfigStr.hc2_timer05, kmConfigStr.hc2_timer06, kmConfigStr.hc2_timer07, kmConfigStr.hc2_timer08, kmConfigStr.hc2_timer09, kmConfigStr.hc2_timer10, kmConfigStr.hc2_timer11, kmConfigStr.hc2_timer12, kmConfigStr.hc2_timer13, kmConfigStr.hc2_timer14},
  };
  const char *topics[2][14] = {
    {KM_CFG_TOPIC::HC1_TIMER01[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER02[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER03[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER04[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER05[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER06[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER07[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER08[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER09[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER10[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER11[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER12[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER13[config.mqtt.lang], KM_CFG_TOPIC::HC1_TIMER14[config.mqtt.lang]},
    {KM_CFG_TOPIC::HC2_TIMER01[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER02[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER03[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER04[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER05[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER06[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER07[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER08[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER09[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER10[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER11[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER12[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER13[config.mqtt.lang], KM_CFG_TOPIC::HC2_TIMER14[config.mqtt.lang]},
  };
  char point[3][100];
  for (uint8_t i = 0; i < 3; ++i) decodeTimer(point[i], sizeof(point[i]), bytes[i * 2], bytes[i * 2 + 1]);
  snprintf(texts[circuit][block], CFG_MAX_CHAR_TIMER, "SP%02u: %s | SP%02u: %s | SP%02u: %s",
           unsigned(block * 3 + 1), point[0], unsigned(block * 3 + 2), point[1], unsigned(block * 3 + 3), point[2]);
  ++(circuit ? kmConfigNum.hc2_timer[block] : kmConfigNum.hc1_timer[block]);
  km271Msg(KM_TYP_CONFIG, topics[circuit][block], texts[circuit][block]);
}

// This is an assumed adoption after transport ACK, not a synthetic receive
// block. Unknown writes/date-time/timer data never invent configuration values.
static void applyAcknowledgedConfig(const uint8_t data[8]) {
  if (timerAddress(data)) {
    updateTimerBlock(data[0] - 0x11, data[1] / 7 - 1, data + 2);
    return; // 0x65 is a real time byte here, never a keep marker.
  }
  if ((data[0] == 0x07 || data[0] == 0x08) && data[1] != 0x00 &&
      data[1] != 0x0e && data[1] != 0x15 && data[1] != 0x1c && data[1] != 0x31) return;
  uint16_t reg;
  switch (data[0]) {
  case 0x07:
    reg = data[1];
    break;
  case 0x08:
    if (!config.km271.use_hc2) return;
    reg = 0x0038 + data[1];
    break;
  case 0x0c:
    if (data[1] == 0x07) reg = 0x007e;
    else if (data[1] == 0x0e) reg = 0x0085;
    else return;
    break;
  case 0x11:
    if (!config.km271.use_hc1 || data[1] != 0) return;
    reg = 0x0100;
    break;
  case 0x12:
    if (!config.km271.use_hc2 || data[1] != 0) return;
    reg = 0x0169;
    break;
  default:
    return;
  }
  for (uint8_t offset = 0; offset < 6; ++offset) {
    if (data[2 + offset] == 0x65) continue; // Keep the previously known value.
    // These two HK1 fields are also received when HK1 is disabled.
    if (data[0] == 0x07 && !config.km271.use_hc1 &&
        !((reg == 0x0000 && offset == 1) || (reg == 0x0031 && offset == 5))) continue;
    updateKmConfigField(reg, offset, data[2 + offset]);
  }
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

  char tmpMessage[300] = {'\0'};

  uint kmregister = (data[0] * 256) + data[1];

  // kmRxBuf.buf is a fixed-size, never-cleared buffer - a block shorter
  // than this floor would have every case below read data[2] (or beyond)
  // from stale bytes left over by a previous, longer block instead of
  // from this message, and silently publish that as a fresh, valid
  // reading. This is a conservative minimum (every case references at
  // least data[2]), not a full per-register length check - some
  // registers need more than 3 bytes and could still rarely read a
  // couple of stale trailing bytes from a corrupt-but-checksum-valid
  // short block. See docs/firmware-security-review.md #4 in the
  // logamatic project for the full writeup of this finding.
  if (len < 3) {
    snprintf(tmpMessage, sizeof(tmpMessage), "block too short to parse safely: register 0x%04x, len %d", kmregister, len);
    km271Msg(KM_TYP_DEBUG, tmpMessage, "");
    return;
  }

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

  for (uint8_t circuit = 0; circuit < 2; ++circuit) {
    const uint16_t base = circuit ? 0x0169 : 0x0100;
    if (kmregister <= base || kmregister > base + 98 || (kmregister - base) % 7) continue;
    if (len < 8 || !(circuit ? config.km271.use_hc2 : config.km271.use_hc1)) return;
    const uint8_t block = (kmregister - base) / 7 - 1;
    if (timerWriteCircuit >= 0 && memcmp(timerSnapshots[circuit].raw + block * 6, data + 2, 6))
      restartKmLogging("timer changed externally during write; reload required");
    updateTimerBlock(circuit, block, data + 2);
    return;
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
    updateKmConfigField(0x0000, 1, data[2 + 1]);

    if (config.km271.use_hc1) {
      updateKmConfigField(0x0000, 2, data[2 + 2]);
      updateKmConfigField(0x0000, 3, data[2 + 3]);
      updateKmConfigField(0x0000, 4, data[2 + 4]);
      updateKmConfigField(0x0000, 5, data[2 + 5]);
    }
    break;

  case 0x000e:
    if (config.km271.use_hc1) {
      kmConfigNum.hc1_max_temp = data[2 + 2];
      snprintf(kmConfigStr.hc1_max_temp, sizeof(kmConfigStr.hc1_max_temp), "%i °C", kmConfigNum.hc1_max_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC1_MAX_TEMP[config.mqtt.lang],
               kmConfigStr.hc1_max_temp); // "CFG_HK1_Max_Temperatur"    => "000e:2"

      updateKmConfigField(0x000e, 4, data[2 + 4]);
    }
    break;

  case 0x0015:
    if (config.km271.use_hc1) {
      updateKmConfigField(0x0015, 0, data[2 + 0]);
      updateKmConfigField(0x0015, 2, data[2 + 2]);
    }
    break;

  case 0x001c:
    if (config.km271.use_hc1) {
      updateKmConfigField(0x001c, 1, data[2 + 1]);

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

    updateKmConfigField(0x0031, 5, data[2 + 5]);
    break;

  case 0x0038:

    if (config.km271.use_hc2) {
      updateKmConfigField(0x0038, 1, data[2 + 1]);
      updateKmConfigField(0x0038, 2, data[2 + 2]);
      updateKmConfigField(0x0038, 3, data[2 + 3]);
      updateKmConfigField(0x0038, 4, data[2 + 4]);
      updateKmConfigField(0x0038, 5, data[2 + 5]);
    }
    break;

  case 0x0046:
    if (config.km271.use_hc2) {
      kmConfigNum.hc2_max_temp = data[2 + 2];
      snprintf(kmConfigStr.hc2_max_temp, sizeof(kmConfigStr.hc2_max_temp), "%i °C", kmConfigNum.hc2_max_temp);
      km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::HC2_MAX_TEMP[config.mqtt.lang],
               kmConfigStr.hc2_max_temp); // "CFG_HK2_Max_Temperatur"    => "0046:2"

      updateKmConfigField(0x0046, 4, data[2 + 4]);
    }
    break;

  case 0x004d:
    kmConfigNum.ww_priority = data[2 + 1];
    snprintf(kmConfigStr.ww_priority, sizeof(kmConfigStr.ww_priority), "%s",
             KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.ww_priority, 1)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PRIO[config.mqtt.lang],
             kmConfigStr.ww_priority); // "CFG_WW_Vorrang"   => "004d:1,a"

    if (config.km271.use_hc2) {
      updateKmConfigField(0x004d, 0, data[2 + 0]);
      updateKmConfigField(0x004d, 2, data[2 + 2]);
    }
    break;

  case 0x0054:
    if (config.km271.use_hc2) {
      updateKmConfigField(0x0054, 1, data[2 + 1]);

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

      updateKmConfigField(0x0069, 5, data[2 + 5]);
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
    updateKmConfigField(0x007e, 3, data[2 + 3]);
    break;

  case 0x0085:
    updateKmConfigField(0x0085, 0, data[2 + 0]);

    kmConfigNum.ww_processing = data[2 + 3];
    snprintf(kmConfigStr.ww_processing, sizeof(kmConfigStr.ww_processing), "%s",
             KM_CFG_ARRAY::ON_OFF[config.mqtt.lang][limit(0, kmConfigNum.ww_processing, 1)]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::WW_PROCESSING[config.mqtt.lang],
             kmConfigStr.ww_processing); // "CFG_WW_Aufbereitung"  => "0085:3,a"

    updateKmConfigField(0x0085, 5, data[2 + 5]);
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
      updateKmConfigField(0x0100, 0, data[2 + 0]);
      updateKmConfigField(0x0100, 3, data[2 + 3]);
    }
    break;

  case 0x0169:
    if (config.km271.use_hc2) {
      updateKmConfigField(0x0169, 0, data[2 + 0]);
      updateKmConfigField(0x0169, 3, data[2 + 3]);
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

  case 0x01e0: // Clock correction: signed seconds/day, not half-hours.
    kmConfigNum.time_offset = decodeNegValue(data[2 + 1]);
    snprintf(kmConfigStr.time_offset, sizeof(kmConfigStr.time_offset), "%d %s", kmConfigNum.time_offset, MQTT_MSG::SECONDS_PER_DAY[config.mqtt.lang]);
    km271Msg(KM_TYP_CONFIG, KM_CFG_TOPIC::TIME_OFFSET[config.mqtt.lang],
             kmConfigStr.time_offset); // "CFG_Uhrzeit_Offset"    => "01e0:1,s"
    break;

  case 0x0300: // 0x0300 : Error Buffer 1
    if (config.km271.use_alarmMsg && len >= 9) {
      memcpy(alarmTelegrams[0], data, sizeof(alarmTelegrams[0]));
      alarmsReceived |= 1U << 0;
      alarmsCurrent |= 1U << 0;
      if (decodeErrorMsg(kmAlarmMsg.alarm1, sizeof(kmAlarmMsg.alarm1), data, config.mqtt.lang)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_1[config.mqtt.lang], kmAlarmMsg.alarm1); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_1[config.mqtt.lang],
                 kmAlarmMsg.alarm1); // acknowledged alarm (History)
      }
    }
    break;

  case 0x0307: // 0x0307 : Error Buffer 2
    if (config.km271.use_alarmMsg && len >= 9) {
      memcpy(alarmTelegrams[1], data, sizeof(alarmTelegrams[1]));
      alarmsReceived |= 1U << 1;
      alarmsCurrent |= 1U << 1;
      if (decodeErrorMsg(kmAlarmMsg.alarm2, sizeof(kmAlarmMsg.alarm2), data, config.mqtt.lang)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_2[config.mqtt.lang], kmAlarmMsg.alarm2); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_2[config.mqtt.lang],
                 kmAlarmMsg.alarm2); // acknowledged alarm (History)
      }
    }
    break;

  case 0x030e: // 0x030e : Error Buffer 3
    if (config.km271.use_alarmMsg && len >= 9) {
      memcpy(alarmTelegrams[2], data, sizeof(alarmTelegrams[2]));
      alarmsReceived |= 1U << 2;
      alarmsCurrent |= 1U << 2;
      if (decodeErrorMsg(kmAlarmMsg.alarm3, sizeof(kmAlarmMsg.alarm3), data, config.mqtt.lang)) {
        km271Msg(KM_TYP_ALARM, KM_ERR_TOPIC::ERR_BUFF_3[config.mqtt.lang], kmAlarmMsg.alarm3); // unacknowledged alarm
      } else {
        km271Msg(KM_TYP_ALARM_H, KM_ERR_TOPIC::ERR_BUFF_3[config.mqtt.lang],
                 kmAlarmMsg.alarm3); // acknowledged alarm (History)
      }
    }
    break;

  case 0x0315: // 0x0315 : Error Buffer 4
    if (config.km271.use_alarmMsg && len >= 9) {
      memcpy(alarmTelegrams[3], data, sizeof(alarmTelegrams[3]));
      alarmsReceived |= 1U << 3;
      alarmsCurrent |= 1U << 3;
      if (decodeErrorMsg(kmAlarmMsg.alarm4, sizeof(kmAlarmMsg.alarm4), data, config.mqtt.lang)) {
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
      chartValuesReceived |= (1U << 13);
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
      chartValuesReceived |= (1U << 6);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC1_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC1_HeatingForwardTargetTemp));
    }
    break;

  case 0x8003: // 0x8003 : Temperature (1C resolution)
    if (config.km271.use_hc1) {
      kmStatus.HC1_HeatingForwardActualTemp = (float)data[2];
      chartValuesReceived |= (1U << 7);
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
      chartValuesReceived |= (1U << 8);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::HC2_FLOW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HC2_HeatingForwardTargetTemp));
    }
    break;

  case 0x8115: // 0x8115 : Temperature (1C resolution)
    if (config.km271.use_hc2) {
      kmStatus.HC2_HeatingForwardActualTemp = (float)data[2];
      chartValuesReceived |= (1U << 9);
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
    chartValuesReceived |= (1U << 2);
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::WW_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.HotWaterTargetTemp));
    break;

  case 0x8427: // 0x8427 : Temperature (1C resolution)
    kmStatus.HotWaterActualTemp = (float)data[2];
    chartValuesReceived |= (1U << 3);
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
    chartValuesReceived |= (1U << 0);
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_SETPOINT[config.mqtt.lang], EspStrUtil::intToString(kmStatus.BoilerForwardTargetTemp));
    break;

  case 0x882b: // 0x882b : Temperature (1C resolution)
    kmStatus.BoilerForwardActualTemp = (float)data[2];
    chartValuesReceived |= (1U << 1);
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
    chartValuesReceived |= (1U << 12);
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

    if (config.oilmeter.use_virtual_meter) {
      km271calcBurnerCalcOilConsumption();
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::BOILER_CONSUMPTION[config.mqtt.lang], EspStrUtil::floatToString(kmStatus.BurnerCalcOilConsumption, 1));
    }
    break;

  case 0x893c: // 0x893c : Temperature (1C resolution, possibly negative)
    kmStatus.OutsideTemp = decodeNegValue(data[2]);
    chartValuesReceived |= (1U << 10);
    km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::OUTSIDE_TEMP[config.mqtt.lang], EspStrUtil::intToString(kmStatus.OutsideTemp));
    break;

  case 0x893d: // 0x893d : Temperature (1C resolution, possibly negative)
    kmStatus.OutsideDampedTemp = decodeNegValue(data[2]);
    chartValuesReceived |= (1U << 11);
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
      chartValuesReceived |= (1U << 4);
      km271Msg(KM_TYP_STATUS, KM_STAT_TOPIC::SOLAR_WW[config.mqtt.lang], EspStrUtil::intToString(kmStatus.SolarWW));
    }
    break;

  case 0x9146:
    if (config.km271.use_solar) {
      kmStatus.SolarCollector = decodeNegValue(data[2]);
      chartValuesReceived |= (1U << 5);
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

  // A boot-specific session rejects stale browser requests after ESP restart.
  timerSession = esp_random();

  // initialize structs
  memset((void *)&kmStatus, 0, sizeof(s_km271_status));
  chartValuesReceived = 0;
  memset((void *)&kmConfigStr, 0, sizeof(s_km271_config_str));
  memset((void *)&kmConfigNum, 0, sizeof(s_km271_config_num));

  // start the link-liveness window here rather than at 0, so the timeout is
  // measured from when we actually opened the port
  kmSerialStats.lastRxTime = millis();
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
  // seconds since the last byte from the Logamatic - lets a dashboard show how
  // long a link has been down, instead of only that it is
  infoJSON["rx_age_s"] = (millis() - kmSerialStats.lastRxTime) / 1000;
  portENTER_CRITICAL(&sendBufMux);
  const bool writePending = (writeQueue.size() != 0);
  portEXIT_CRITICAL(&sendBufMux);
  infoJSON["send_cmd_busy"] = writePending;
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
  uint8_t local_buf[8];

  char dateTimeInfo[128] = {'\0'}; // Date and time info String

  // Refuse to write an implausible date to the boiler. The Logamatic's
  // day/night heating program is driven by exactly this clock, so a bogus
  // value silently changes when the house is heated. The web UI builds its
  // tm from separate date and time messages, so a partially-filled struct
  // (year 1900, month 0, day 0) can reach here.
  if (dti.tm_year < 116 || dti.tm_mon < 0 || dti.tm_mon > 11 || dti.tm_mday < 1 || dti.tm_mday > 31) {
    km271Msg(KM_TYP_MESSAGE, "date/time not set: implausible date", "");
    return;
  }
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
  local_buf[0] = 0x01;        // address
  local_buf[1] = 0x00;        // address
  local_buf[2] = dti.tm_sec;  // seconds
  local_buf[3] = dti.tm_min;  // minutes
  local_buf[4] = dti.tm_hour; // hours (bit 0-4)
  if (dti.tm_isdst > 0)
    local_buf[4] |= (1 << 6) & 0x40;                     // if time ist DST  (bit 6)
  local_buf[5] = dti.tm_mday;                            // day of month
  local_buf[6] = dti.tm_mon + 1;                         // month
  local_buf[6] |= (((dti.tm_wday + 6) % 7) << 4) & 0x70; // day of week (Logamatic: 0=monday...6=sunday / wday: 0=sunday...6=saturday)
  local_buf[7] = dti.tm_year;                            // year year < 100 means 19xx / year > 100 means 20xx
  if (!commitSendBuf(local_buf)) return;

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
  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%s: (%s) %02d.%02d.%d - %02i:%02i:%02i", MQTT_MSG::DATETIME_QUEUED[config.mqtt.lang], wday,
           dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec);
  km271Msg(KM_TYP_MESSAGE, dateTimeInfo, "");
}

/**
 * *******************************************************************
 * @brief   set actual date and time to buderus
 * @param   dti: date and time info structure
 * @return  true if a valid current time was queued
 * *******************************************************************/
bool km271SetDateTimeNTP() {
  uint8_t local_buf[8];

  char dateTimeInfo[128] = {'\0'}; // Date and time info String
  time_t now;                      // this is the epoch
  tm dti;                          // the structure tm holds time information in a more convient way
  time(&now);                      // read the current time
  localtime_r(&now, &dti);         // update the structure tm with the current time

  // Refuse to write an unsynchronised clock to the boiler: until NTP resolves,
  // time() returns an epoch-1970 value. The automatic caller in main.cpp
  // already gates on getLocalTime(), but the "cmd/datetime" MQTT topic calls
  // this directly - so an HA automation firing right after a reboot could set
  // the Logamatic's clock, and therefore its heating program, to 1970.
  if (dti.tm_year < 116) { // years since 1900, so 116 = 2016
    km271Msg(KM_TYP_MESSAGE, "date/time not set: no valid time source (NTP not synced)", "");
    return false;
  }

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
  local_buf[0] = 0x01;        // address
  local_buf[1] = 0x00;        // address
  local_buf[2] = dti.tm_sec;  // seconds
  local_buf[3] = dti.tm_min;  // minutes
  local_buf[4] = dti.tm_hour; // hours (bit 0-4)
  if (dti.tm_isdst > 0)
    local_buf[4] |= (1 << 6) & 0x40;                     // if time ist DST  (bit 6)
  local_buf[5] = dti.tm_mday;                            // day of month
  local_buf[6] = dti.tm_mon + 1;                         // month
  local_buf[6] |= (((dti.tm_wday + 6) % 7) << 4) & 0x70; // day of week (Logamatic: 0=monday...6=sunday / wday: 0=sunday...6=saturday)
  local_buf[7] = dti.tm_year;                            // year
  if (!commitSendBuf(local_buf)) return false;

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

  snprintf(dateTimeInfo, sizeof(dateTimeInfo), "%s: (%s) %02d.%02d.%d - %02i:%02i:%02i - DST:%d", MQTT_MSG::DATETIME_QUEUED[config.mqtt.lang], wday,
           dti.tm_mday, (dti.tm_mon + 1), (dti.tm_year + 1900), dti.tm_hour, dti.tm_min, dti.tm_sec, (dti.tm_isdst > 0));
  km271Msg(KM_TYP_MESSAGE, dateTimeInfo, "");
  return true;
}

/**
 * *******************************************************************
 * @brief   prepare and send setvalues to buderus controller
 * @param   sendCmd: send command
 * @param   cmdPara: parameter as integer
 * @return  none
 * *******************************************************************/
void km271sendCmd(e_km271_sendCmd sendCmd, int cmdPara) {
  uint8_t local_buf[8];

  switch (sendCmd) {
  case KM271_SENDCMD_HC1_OPMODE:
    if (cmdPara >= 0 && cmdPara <= 2) {
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x0E; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = cmdPara; // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x0E; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = cmdPara; // Resolution: 1 °C - Range: 30 – 90 °C WE: 75 °C
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x11;    // Data-Type HK1 Schaltuhr
      local_buf[1] = 0x00;    // Offset
      local_buf[2] = cmdPara; // Programmnummer 0..8
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x11; // Data-Type HK1 Schaltuhr
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = cmdPara; // Holiday days -  Resolution: 1 Day - Range: 0 – 99 Days
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x12;    // Data-Type
      local_buf[1] = 0x00;    // Offset
      local_buf[2] = cmdPara; // Programmnummer 0..8
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x12; // Data-Type HK2
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = cmdPara; // Resolution: 1 Day - Range: 0 – 99 Days
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x0C;    // Data-Type für Warmwasser (0x0C)
      local_buf[1] = 0x0E;    // Offset
      local_buf[2] = cmdPara; // 0:Night | 1:Day | 2:AUTO
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = cmdPara; // 9:Winter | 10°-30° | 31:Summer
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = cmdPara; // 9:Winter | 10°-30° | 31:Summer
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x31; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x31; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x15; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x15; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = (cmdPara > 0) ? cmdPara : cmdPara + 256; // -20° ... +10° (add 256 if value is negative)
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x0C; // Data-Type für Warmwater (0x0C)
      local_buf[1] = 0x07; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = cmdPara; // 30°-60°
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x0C; // Data-Type für Warmwater (0x0C)
      local_buf[1] = 0x0E; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = cmdPara; // 0:OFF - 1..6 - 7:ON
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07;    // Data-Type für HK1 (0x07)
      local_buf[1] = 0x15;    // Offset
      local_buf[2] = cmdPara; // Resolution: 1 °C - Range: 0 – 10 °C
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08;    // Data-Type für HK2 (0x08)
      local_buf[1] = 0x15;    // Offset
      local_buf[2] = cmdPara; // Resolution: 1 °C - Range: 0 – 10 °C
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x1c; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = cmdPara; // {"off", "fixed", "room", "outdoors"}
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x1c; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = cmdPara; // {"off", "fixed", "room", "outdoors"}
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
  uint8_t local_buf[8];

  switch (sendCmd) {
  case KM271_SENDCMD_HC1_DAY_SETPOINT:
    if (cmdPara >= 10 && cmdPara <= 30) {
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK2 (0x08)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = 0x65;
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x07; // Data-Type für HK1 (0x07)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      if (!commitSendBuf(local_buf)) return;
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
      local_buf[0] = 0x08; // Data-Type für HK1 (0x08)
      local_buf[1] = 0x00; // Offset
      local_buf[2] = 0x65;
      local_buf[3] = 0x65;
      local_buf[4] = 0x65;
      local_buf[5] = 0x65;
      local_buf[6] = 0x65;
      local_buf[7] = trunc(2.0 * cmdPara + 0.5); // Resolution: 0.5 °C - Range: 10 – 30 °C
      if (!commitSendBuf(local_buf)) return;
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
 * @details Queue a pre-checked service telegram. A zero Data-Type remains
 *          rejected for compatibility; queue admission failures are reported too.
 * @return  true only if the telegram was queued, not yet acknowledged.
 * *******************************************************************/
bool km271sendServiceCmd(uint8_t cmdPara[8]) {
  if (cmdPara[0] == 0) {
    km271Msg(KM_TYP_MESSAGE, "service message rejected: first byte must not be 00", "");
    return false;
  }
  return commitSendBuf(cmdPara);
}

/**
 * *******************************************************************
 * @brief   returns the status if LogMode is active
 * @param   none
 * @return  bool state of LogModeActive
 * *******************************************************************/
bool km271GetLogMode() { return kmSerialStats.logModeActive; }
uint8_t km271ChartValidity() {
  if (!kmInitDone || !km271GetLogMode()) return 0;
  uint8_t valid = 0;
  for (uint8_t i = 0; i < 6; ++i) {
    const uint16_t pair = 3U << (2 * i);
    if ((chartValuesReceived & pair) == pair) valid |= (1U << i);
  }
  if (!config.km271.use_ww) valid &= ~(1U << 1);
  if (!config.km271.use_solar) valid &= ~(1U << 2);
  if (!config.km271.use_hc1) valid &= ~(1U << 3);
  if (!config.km271.use_hc2) valid &= ~(1U << 4);
  if (chartValuesReceived & (1U << 12)) valid |= (1U << 6);
  if (config.km271.use_hc1 && (chartValuesReceived & (1U << 13))) valid |= (1U << 7);
  return valid;
}
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
 * @details errMsgText now covers the full official Buderus fault-code
 *          range 0-213 (Buderus doc 7747004149, "Technische Information -
 *          Monitordaten System 4000", section 3.1 "Aktuelle Fehlerliste"),
 *          so the raw error number IS the array index directly - no
 *          lookup table needed anymore. Any code outside the documented
 *          range (214-255) clamps to index 214, the generic "unknown
 *          error" fallback entry.
 *          Previously this only recognized 26 of the ~213 documented
 *          codes and silently mapped everything else to "unknown error"
 *          - e.g. code 93 (Handschalter Heizkreis 1) or a phantom-module
 *          fault would show as "unknown error" with no way to tell what
 *          actually happened without reading the raw serial bytes.
 * @param   errorNr Error Number from Logamatic
 * @return  index for error message in errMsgText
 * *******************************************************************/
uint8_t getErrorTextIndex(uint8_t errorNr) { return limit(0, errorNr, 214); }

bool km271GetAlarmData(uint8_t index, uint8_t *data) {
  if (index >= 4 || !(alarmsReceived & (1U << index))) return false;
  memcpy(data, alarmTelegrams[index], sizeof(alarmTelegrams[index]));
  return true;
}

uint8_t km271GetAlarmValidity() { return alarmsCurrent; }

const char *km271GetAlarmTitle(uint8_t code, uint8_t language) {
  return errMsgText.idx[constrain(language, 0, MAX_LANG - 1)][getErrorTextIndex(code)];
}

// Format the last received alarm in the requested language without changing MQTT messages.
void km271GetAlarmText(uint8_t index, char *text, unsigned int size, uint8_t language) {
  if (size == 0) return;
  text[0] = '\0';
  if (index < 4 && (alarmsReceived & (1U << index))) {
    decodeErrorMsg(text, size, alarmTelegrams[index], constrain(language, 0, MAX_LANG - 1));
  }
}

/**
 * *******************************************************************
 * @brief   generate error message
 * @param   errorNr Error Number from Logamatic
 * @return  true = unackloaded alarm / false = acknowled alarm
 * *******************************************************************/
bool decodeErrorMsg(char *errorMsg, unsigned int size, const uint8_t *data, uint8_t language) {
  // no error
  if (data[2] == 0) {
    snprintf(errorMsg, size, "%s", errMsgText.idx[language][0]);
    return false;
  } else {
    uint8_t errIndex = getErrorTextIndex(data[2]);
    // errMsgText only covers the officially documented range (see
    // getErrorTextIndex) - if this raw code fell back to the generic
    // "unknown error" entry, still surface the actual number so it can be
    // looked up (or reported/added to the table) instead of being lost.
    // Discovering that this was the case, with no way to get the number
    // back short of capturing and manually decoding the raw serial
    // telegram, is exactly what motivated expanding the table in the
    // first place - see docs/firmware-improvement-error-table.md in the
    // logamatic project.
    char codeSuffix[16] = "";
    if (errIndex == 214) {
      snprintf(codeSuffix, sizeof(codeSuffix), " [code %d]", data[2]);
    }
    // error already acknowledged
    if (data[6] != 0xFF) {
      // Aussenfuehler defekt (>> 16:31 -3 Tage | << 20:40 -2 Tage)
      snprintf(errorMsg, size, "%s%s (>> %02i:%02i -%i %s | << %02i:%02i -%i %s)", errMsgText.idx[language][errIndex], codeSuffix, data[3],
               data[4], (data[5] + data[8]), MQTT_MSG::DAYS[language], data[6], data[7], data[8], MQTT_MSG::DAYS[language]);
      return false;
    }
    // unacknowledged error
    else {
      // example: Aussenfuehler defekt (>> 16:31 -3 Tage)
      snprintf(errorMsg, size, "%s%s (>> %02i:%02i -%i %s)", errMsgText.idx[language][errIndex], codeSuffix, data[3], data[4], data[5],
               MQTT_MSG::DAYS[language]);
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
 * @brief   calculate the virtual oil consumption
 * @param   none
 * @return  none
 * *******************************************************************/
void km271calcBurnerCalcOilConsumption() {
  if (config.oilmeter.use_virtual_meter && config.oilmeter.oil_density_kg_l != 0) {
    kmStatus.BurnerCalcOilConsumption =
        ((double)kmStatus.BurnerOperatingDuration_Sum / 60 * config.oilmeter.consumption_kg_h / config.oilmeter.oil_density_kg_l) -
        config.oilmeter.virt_calc_offset;
  }
}

/**
 * *******************************************************************
 * @brief   send all km271 status values
 * @param   noen
 * @return  none
 * *******************************************************************/
void sendAllKmStatValues() {

  // Nothing has been decoded from the boiler yet, so kmStatus is still the
  // zero-initialised struct. Replaying it announces a burner runtime of 0, a
  // boiler temperature of 0 and so on, which subscribers cannot tell apart
  // from a real reading - and for the cumulative counters Home Assistant
  // records the drop to 0 and the jump back as consumption, which is enough
  // to ruin the daily/monthly statistics derived from them. The boiler's own
  // initial dump publishes every value as it arrives, so nothing is lost by
  // waiting for it. See #161.
  if (!kmInitDone) {
    return;
  }

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

  if (config.oilmeter.use_virtual_meter) {
    km271calcBurnerCalcOilConsumption();
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

  // Same as in sendAllKmStatValues(): before the first complete dump the
  // config strings are empty and the numbers are 0, so publishing them would
  // report setpoints the controller never sent.
  if (!kmInitDone) {
    return;
  }

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
