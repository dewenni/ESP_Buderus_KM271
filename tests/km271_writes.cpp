// engine.cpp is the unmodified production transport prefix, generated in a
// temporary directory by test_km271_writes.py. No ESP or UART is accessed.
#include "engine.cpp"

static std::vector<std::vector<uint8_t>> received;
static std::vector<std::string> messages;
static size_t infoMessages = 0;
static size_t debugMessages = 0;
#include "receive_config.cpp"
static std::vector<std::pair<std::string, std::string>> configMessages;
void km271Msg(e_kmMsgTyp type, const char *text, const char *value) {
  assert(!criticalDepth);
  messages.emplace_back(text);
  if (type == KM_TYP_MESSAGE) ++infoMessages;
  if (type == KM_TYP_DEBUG) ++debugMessages;
  if (type == KM_TYP_CONFIG) configMessages.emplace_back(text, value);
}

static void feed(uint8_t byte) {
  Serial2.incoming.push_back(byte);
  cyclicKM271();
  assert(!criticalDepth);
}

static std::vector<uint8_t> frame(const uint8_t *data, size_t length) {
  std::vector<uint8_t> result;
  uint8_t checksum = KM_DLE ^ KM_ETX;
  for (size_t i = 0; i < length; ++i) {
    result.push_back(data[i]);
    checksum ^= data[i];
    if (data[i] == KM_DLE) {
      result.push_back(KM_DLE);
      checksum ^= KM_DLE;
    }
  }
  result.push_back(KM_DLE);
  result.push_back(KM_ETX);
  result.push_back(checksum);
  return result;
}

static void connect() {
  restartKmLogging(nullptr);
  Serial2.incoming.clear();
  Serial2.outgoing.clear();
  received.clear();
  messages.clear();
  kmSerialStats.lastRxTime = clockMs;
  feed(KM_DLE); // unsolicited DLE cannot start logmode
  assert(Serial2.outgoing.empty());
  feed(KM_STX);
  assert(Serial2.outgoing.back() == std::vector<uint8_t>{KM_STX});
  feed(KM_DLE);
  assert(Serial2.outgoing.back() == frame(KmCLogMode, 3));
  feed(KM_DLE);
  assert(KmRxBlockState == KM_TSK_LOGGING && kmSerialStats.logModeActive);
  assert(km271RefreshActive);
  km271RefreshActive = false; // completed initial refresh
  chartValuesReceived = 0x123;
  alarmsCurrent = 0x0f;
  Serial2.outgoing.clear();
}

static void beginWrite(const uint8_t expected[8]) {
  feed(KM_STX);
  assert(KmRxBlockState == KM_TSK_WRITE_READY);
  assert(Serial2.outgoing.back() == std::vector<uint8_t>{KM_STX});
  assert(kmSerialStats.logModeActive);
  feed(KM_DLE);
  assert(KmRxBlockState == KM_TSK_WRITE_ACK);
  assert(Serial2.outgoing.back() == frame(expected, 8));
  assert(kmSerialStats.logModeActive);
}

static void finishWrite() {
  const size_t sent = Serial2.outgoing.size();
  feed(KM_DLE);
  assert(KmRxBlockState == KM_TSK_LOGGING);
  assert(sent == Serial2.outgoing.size()); // ACK never sends another telegram
  assert(!km271RefreshActive && chartValuesReceived == 0x123 && alarmsCurrent == 0x0f);
}

static void testAcknowledgedConfig() {
  for (uint8_t language : {0, 1}) {
    config.mqtt.lang = language;
    struct Example { e_km271_sendCmd command; float value; const char *topic; std::string text; bool fractional; };
    const Example examples[] = {
      {KM271_SENDCMD_HC1_OPMODE, 2.0f, KM_CFG_TOPIC::HC1_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false},
      {KM271_SENDCMD_HC1_DESIGN_TEMP, 70.0f, KM_CFG_TOPIC::HC1_INTERPR[language], "70 °C", false},
      {KM271_SENDCMD_HC1_PROGRAMM, 3.0f, KM_CFG_TOPIC::HC1_PROGRAM[language], KM_CFG_ARRAY::HC_PROGRAM[language][3], false},
      {KM271_SENDCMD_HC1_HOLIDAYS, 12.0f, KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[language], std::string("12 ") + MQTT_MSG::DAYS[language], false},
      {KM271_SENDCMD_HC1_SUMMER, 21.0f, KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[language], KM_CFG_ARRAY::SUMMER[language][12], false},
      {KM271_SENDCMD_HC1_FROST, -5.0f, KM_CFG_TOPIC::HC1_FROST_THRESHOLD[language], "-5 °C", false},
      {KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, -4.0f, KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[language], "-4 °C", false},
      {KM271_SENDCMD_HC1_SWITCH_ON_TEMP, 3.0f, KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[language], KM_CFG_ARRAY::SWITCH_ON_TEMP[language][3], false},
      {KM271_SENDCMD_HC1_REDUCTION_MODE, 2.0f, KM_CFG_TOPIC::HC1_REDUCTION_MODE[language], KM_CFG_ARRAY::REDUCT_MODE[language][2], false},
      {KM271_SENDCMD_HC1_DAY_SETPOINT, 21.5f, KM_CFG_TOPIC::HC1_DAY_TEMP[language], "21.5 °C", true},
      {KM271_SENDCMD_HC1_NIGHT_SETPOINT, 17.5f, KM_CFG_TOPIC::HC1_NIGHT_TEMP[language], "17.5 °C", true},
      {KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, 16.5f, KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[language], "16.5 °C", true},
      {KM271_SENDCMD_HC2_OPMODE, 2.0f, KM_CFG_TOPIC::HC2_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false},
      {KM271_SENDCMD_HC2_DESIGN_TEMP, 70.0f, KM_CFG_TOPIC::HC2_INTERPR[language], "70 °C", false},
      {KM271_SENDCMD_HC2_PROGRAMM, 3.0f, KM_CFG_TOPIC::HC2_PROGRAM[language], KM_CFG_ARRAY::HC_PROGRAM[language][3], false},
      {KM271_SENDCMD_HC2_HOLIDAYS, 12.0f, KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[language], std::string("12 ") + MQTT_MSG::DAYS[language], false},
      {KM271_SENDCMD_HC2_SUMMER, 21.0f, KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[language], KM_CFG_ARRAY::SUMMER[language][12], false},
      {KM271_SENDCMD_HC2_FROST, -5.0f, KM_CFG_TOPIC::HC2_FROST_THRESHOLD[language], "-5 °C", false},
      {KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, -4.0f, KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[language], "-4 °C", false},
      {KM271_SENDCMD_HC2_SWITCH_ON_TEMP, 3.0f, KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[language], KM_CFG_ARRAY::SWITCH_ON_TEMP[language][3], false},
      {KM271_SENDCMD_HC2_REDUCTION_MODE, 2.0f, KM_CFG_TOPIC::HC2_REDUCTION_MODE[language], KM_CFG_ARRAY::REDUCT_MODE[language][2], false},
      {KM271_SENDCMD_HC2_DAY_SETPOINT, 21.5f, KM_CFG_TOPIC::HC2_DAY_TEMP[language], "21.5 °C", true},
      {KM271_SENDCMD_HC2_NIGHT_SETPOINT, 17.5f, KM_CFG_TOPIC::HC2_NIGHT_TEMP[language], "17.5 °C", true},
      {KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, 16.5f, KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[language], "16.5 °C", true},
      {KM271_SENDCMD_WW_SETPOINT, 53, KM_CFG_TOPIC::WW_TEMP[language], "53 °C", false},
      {KM271_SENDCMD_WW_OPMODE, 2, KM_CFG_TOPIC::WW_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false},
      {KM271_SENDCMD_WW_PUMP_CYCLES, 4, KM_CFG_TOPIC::WW_CIRCULATION[language], KM_CFG_ARRAY::CIRC_INTERVAL[language][4], false},
    };
    for (const auto &example : examples) {
      connect();
      configMessages.clear();
      infoMessages = debugMessages = 0;
      if (example.fractional) km271sendCmdFlt(example.command, example.value);
      else km271sendCmd(example.command, static_cast<int>(example.value));
      assert(configMessages.empty()); // Enqueueing is not confirmation.
      Km271WriteQueue::Telegram telegram;
      assert(writeQueue.front(telegram));
      beginWrite(telegram.data);
      assert(configMessages.empty()); // Sending bytes is not confirmation either.
      finishWrite();
      assert(configMessages.size() == 1);
      // Only the command feedback uses the info/Pushover path. Queue and ACK
      // details remain available through the existing debug channel.
      assert(infoMessages == 1 && debugMessages == 2);
      assert(configMessages[0].first == example.topic && configMessages[0].second == example.text);
    }
  }

  config.mqtt.lang = 0;
  connect();
  kmConfigNum.ww_temp = 55;
  kmConfigNum.ww_circulation = 3;
  kmConfigNum.ww_priority = 1;
  kmStatus.HotWaterTargetTemp = 49; // Actual target is a distinct received value.
  configMessages.clear();
  const uint8_t temperatures[][8] = {
      {0x0c, 7, 0x65, 0x65, 0x65, 53, 0x65, 0x65},
      {0x0c, 7, 0x65, 0x65, 0x65, 56, 0x65, 0x65}};
  assert(km271QueueWrite(temperatures, 2));
  beginWrite(temperatures[0]);
  assert(kmConfigNum.ww_temp == 55);
  finishWrite();
  assert(kmConfigNum.ww_temp == 53 && std::string(kmConfigStr.ww_temp) == "53 °C");
  beginWrite(temperatures[1]);
  for (unsigned retry = 0; retry <= KM271_WRITE_RETRIES; ++retry) {
    feed(KM_NAK);
    if (retry < KM271_WRITE_RETRIES) beginWrite(temperatures[1]);
  }
  assert(kmConfigNum.ww_temp == 53 && configMessages.size() == 1);
  assert(kmConfigNum.ww_circulation == 3 && kmConfigNum.ww_priority == 1);
  assert(kmStatus.HotWaterTargetTemp == 49);

  // A genuine received config telegram wins over assumed adoption.
  const uint8_t reported[] = {0x00, 0x7e, 0, 0, 0, 54, 0, 0};
  feed(KM_STX);
  for (uint8_t byte : frame(reported, sizeof(reported))) feed(byte);
  assert(kmConfigNum.ww_temp == 54 && std::string(kmConfigStr.ww_temp) == "54 °C");
  assert(configMessages.back().second == "54 °C");

  // Timeout after sending cannot change a cache or publish a config value.
  configMessages.clear();
  assert(km271QueueWrite(temperatures, 2));
  beginWrite(temperatures[0]);
  clockMs += KM271_ACK_TIMEOUT;
  cyclicKM271();
  feed(KM_DLE); // late ACK is ignored
  assert(kmConfigNum.ww_temp == 54 && configMessages.empty());

  // No-op and unknown service writes do not invent configuration fields.
  connect();
  configMessages.clear();
  const uint8_t unknown[][8] = {{0x07, 0x38, 0, 0, 0, 0, 0, 0},
                              {0x01, 0, 0, 0, 0, 0, 0, 0},
                              {0x0c, 7, 0x65, 0x65, 0x65, 0x65, 0x65, 0x65}};
  assert(km271QueueWrite(unknown, 3));
  for (const auto &telegram : unknown) { beginWrite(telegram); finishWrite(); }
  assert(configMessages.empty() && kmConfigNum.ww_temp == 54);

  // A multi-field service write changes exactly its known non-keep fields.
  const uint8_t service[][8] = {{0x07, 0, 0x65, 0x65, 35, 43, 0x65, 0x65}};
  assert(km271QueueWrite(service, 1));
  beginWrite(service[0]); finishWrite();
  assert(kmConfigNum.hc1_night_temp == 17.5f && kmConfigNum.hc1_day_temp == 21.5f);
  assert(configMessages.size() == 2);
}

#include "km271_timers.cpp"

static void testClockCorrection() {
  // Display-confirmed raw values from the original KM271 on 2026-11-27,
  // plus the documented range boundaries. Payload byte 0 is still unknown.
  for (uint8_t language : {0, 1}) {
    config.mqtt.lang = language;
    for (uint8_t unknown : {0x02, 0xfb}) {
      for (int seconds : {0, -6, 4, -30, 30}) {
        uint8_t payload[] = {0x01, 0xe0, unknown, uint8_t(seconds), 0x65, 0x65, 0x65, 0x65};
        configMessages.clear();
        connect();
        feed(KM_STX);
        for (uint8_t byte : frame(payload, sizeof(payload))) feed(byte);
        assert(kmConfigNum.time_offset == seconds);
        const std::string expected = std::to_string(seconds) + (language ? " s/day" : " s/Tag");
        assert(kmConfigStr.time_offset == expected);
        assert(configMessages.size() == 1);
        assert(configMessages[0].first == KM_CFG_TOPIC::TIME_OFFSET[language]);
        assert(configMessages[0].second == expected);
      }
    }
  }
}

int main() {
  const uint8_t batch[][8] = {{0x07, 0, 0x65, 0x10, 0x65, 0x65, 1, 0x65},
                              {0x08, 0, 0x65, 0x65, 0x65, 0x65, 2, 0x65}};
  const uint8_t later[][8] = {{0x0c, 7, 0x65, 0x65, 0x65, 50, 0x65, 0x65}};

  connect();
  assert(!km271QueueWrite(nullptr, 1));
  assert(!km271QueueWrite(batch, 0));
  assert(!km271QueueWrite(batch, KM271_WRITE_QUEUE_CAPACITY + 1));
  assert(writeQueue.size() == 0);
  assert(km271QueueWrite(batch, 2));
  assert(km271QueueWrite(later, 1));
  beginWrite(batch[0]);
  // Concurrent arrival while awaiting ACK appends without replacing active data.
  assert(km271QueueWrite(later, 1));
  finishWrite();
  assert(requestAcknowledged == 1);
  // NAK while requesting permission retains the same front telegram.
  feed(KM_STX);
  feed(KM_NAK);
  beginWrite(batch[1]);
  finishWrite();
  assert(requestAcknowledged == 0);
  beginWrite(later[0]); finishWrite();
  beginWrite(later[0]); finishWrite();
  assert(writeQueue.size() == 0);
  size_t sent = Serial2.outgoing.size();
  feed(KM_DLE); feed(KM_NAK);
  assert(Serial2.outgoing.size() == sent && received.empty());
  feed(KM_STX);
  assert(Serial2.outgoing.back() == std::vector<uint8_t>{KM_DLE});
  // A producer can queue a write while reception is already in progress.
  uint8_t mutableCommand[8];
  memcpy(mutableCommand, later[0], 8);
  assert(commitSendBuf(mutableCommand));
  mutableCommand[0] = 0xff; // The queue must own a copy of the original bytes.
  const uint8_t payload[] = {0x00, 0x0e, KM_DLE, 0x22};
  for (uint8_t byte : frame(payload, sizeof(payload))) feed(byte);
  assert(received.size() == 1 && received[0] == std::vector<uint8_t>(payload, payload + sizeof(payload)));
  assert(Serial2.outgoing.back() == std::vector<uint8_t>{KM_DLE});
  beginWrite(later[0]); finishWrite();
  // A bad checksum requests a retransmission, never decoding corrupted data.
  feed(KM_STX);
  auto bad = frame(payload, sizeof(payload));
  bad.back() ^= 1;
  for (uint8_t byte : bad) feed(byte);
  assert(received.size() == 1 && Serial2.outgoing.back() == std::vector<uint8_t>{KM_NAK});

  // Wrap-around, exact capacity and all-or-nothing batch admission.
  for (size_t cycle = 0; cycle < 3; ++cycle) {
    for (size_t i = 0; i < KM271_WRITE_QUEUE_CAPACITY - 1; ++i) assert(km271QueueWrite(later, 1));
    assert(!km271QueueWrite(batch, 2));
    assert(writeQueue.size() == KM271_WRITE_QUEUE_CAPACITY - 1);
    assert(km271QueueWrite(later, 1));
    assert(!km271QueueWrite(later, 1));
    for (size_t i = 0; i < KM271_WRITE_QUEUE_CAPACITY; ++i) { beginWrite(later[0]); finishWrite(); }
    assert(writeQueue.size() == 0);
  }

  // One successful part, repeated rejection of the second part, unsent suffix
  // discarded; a separately queued request survives.
  connect();
  const uint8_t triple[][8] = {{7, 0}, {7, 7}, {7, 14}};
  assert(km271QueueWrite(triple, 3));
  assert(km271QueueWrite(later, 1));
  beginWrite(triple[0]); finishWrite();
  for (unsigned retry = 0; retry <= KM271_WRITE_RETRIES; ++retry) {
    beginWrite(triple[1]);
    feed(KM_NAK);
  }
  assert(writeQueue.size() == 1 && !writeActive && requestAcknowledged == 0);
  assert(messages.back().find("1 acknowledged; remaining request discarded") != std::string::npos);
  beginWrite(later[0]); finishWrite();

  // Both handshake timeouts cancel all writes and close admission. A late ACK
  // cannot revive them; a new session sends only the log command.
  for (bool afterSend : {false, true}) {
    connect();
    assert(km271QueueWrite(batch, 2));
    feed(KM_STX);
    if (afterSend) feed(KM_DLE);
    clockMs += KM271_ACK_TIMEOUT;
    cyclicKM271();
    assert(KmRxBlockState == KM_TSK_START && !kmSerialStats.logModeActive);
    assert(writeQueue.size() == 0 && !writeActive && !alarmsCurrent && !chartValuesReceived);
    assert(!km271QueueWrite(later, 1));
    size_t beforeLate = Serial2.outgoing.size();
    feed(KM_DLE);
    assert(beforeLate == Serial2.outgoing.size());
    feed(KM_STX); feed(KM_DLE); feed(KM_DLE);
    assert(kmSerialStats.logModeActive && writeQueue.size() == 0);
  }

  connect();
  assert(km271QueueWrite(batch, 2));
  clockMs += KM271_RX_TIMEOUT + 1;
  // First arriving byte after the outage must discard the old queue first.
  feed(KM_STX);
  assert(writeQueue.size() == 0 && KmRxBlockState == KM_TSK_LG_READY);
  assert(!km271QueueWrite(later, 1));
  feed(KM_DLE); feed(KM_DLE);
  assert(kmSerialStats.logModeActive);

  connect();
  assert(km271QueueWrite(batch, 2));
  beginWrite(batch[0]);
  feed(KM_STX); // unexpected ACK, result unknown: cancel and resynchronise
  assert(KmRxBlockState == KM_TSK_START && writeQueue.size() == 0);

  // The initial log handshake also has bounded waits and rejects non-ACKs.
  for (bool afterLogCommand : {false, true}) {
    restartKmLogging(nullptr);
    feed(KM_STX);
    if (afterLogCommand) feed(KM_DLE);
    clockMs += KM271_ACK_TIMEOUT;
    cyclicKM271();
    assert(KmRxBlockState == KM_TSK_START && !writesAccepted);
    feed(KM_STX); feed(KM_DLE); feed(KM_NAK);
    assert(KmRxBlockState == KM_TSK_START && !writesAccepted);
  }

  // millis() rollover must neither expire a fresh link nor stall a timeout.
  clockMs = UINT32_MAX - 100;
  connect();
  assert(km271QueueWrite(batch, 2));
  beginWrite(batch[0]);
  clockMs += KM271_ACK_TIMEOUT;
  cyclicKM271();
  assert(KmRxBlockState == KM_TSK_START && writeQueue.size() == 0);
  testAcknowledgedConfig();
  testClockCorrection();
  testTimers();
  std::puts("KM271 production serial engine: all replay checks passed");
}
