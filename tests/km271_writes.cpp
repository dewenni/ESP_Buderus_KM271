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

#include "route_config.cpp"

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
    kmConfigNum.hc2_heating_system = 1;
    struct Example { e_km271_sendCmd command; float value; const char *topic; std::string text; bool fractional; const char *elementId; };
    const Example examples[] = {
      {KM271_SENDCMD_HC1_MAX_TEMP, 75.0f, KM_CFG_TOPIC::HC1_MAX_TEMP[language], "75 °C", false, "p02_hc1_max_temp"},
      {KM271_SENDCMD_HC1_TEMP_OFFSET, -1.5f, KM_CFG_TOPIC::HC1_TEMP_OFFSET[language], "-1.5 °C", true, "p02_hc1_temp_offset"},
      {KM271_SENDCMD_HC1_HEATING_SYSTEM, 1.0f, KM_CFG_TOPIC::HC1_HEATING_SYSTEM[language], KM_CFG_ARRAY::HEATING_SYSTEM[language][1], false, "p02_hc1_heatsys"},
      {KM271_SENDCMD_HC2_MAX_TEMP, 75.0f, KM_CFG_TOPIC::HC2_MAX_TEMP[language], "75 °C", false, "p02_hc2_max_temp"},
      {KM271_SENDCMD_HC2_TEMP_OFFSET, -1.5f, KM_CFG_TOPIC::HC2_TEMP_OFFSET[language], "-1.5 °C", true, "p02_hc2_temp_offset"},
      {KM271_SENDCMD_HC2_HEATING_SYSTEM, 1.0f, KM_CFG_TOPIC::HC2_HEATING_SYSTEM[language], KM_CFG_ARRAY::HEATING_SYSTEM[language][1], false, "p02_hc2_heatsys"},
      {KM271_SENDCMD_WW_PROCESSING, 1, KM_CFG_TOPIC::WW_PROCESSING[language], KM_CFG_ARRAY::ON_OFF[language][1], false, "p02_ww_processing"},
      {KM271_SENDCMD_WW_PRIORITY, 1, KM_CFG_TOPIC::WW_PRIO[language], KM_CFG_ARRAY::ON_OFF[language][1], false, "p02_ww_priority"},
      {KM271_SENDCMD_HC1_OPMODE, 2.0f, KM_CFG_TOPIC::HC1_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false, "p02_hc1_opmode_auto"},
      {KM271_SENDCMD_HC1_DESIGN_TEMP, 70.0f, KM_CFG_TOPIC::HC1_INTERPR[language], "70 °C", false, "p02_hc1_interp"},
      {KM271_SENDCMD_HC1_PROGRAMM, 3.0f, KM_CFG_TOPIC::HC1_PROGRAM[language], KM_CFG_ARRAY::HC_PROGRAM[language][3], false, "p02_hc1_prg"},
      {KM271_SENDCMD_HC1_HOLIDAYS, 12.0f, KM_CFG_TOPIC::HC1_HOLIDAY_DAYS[language], std::string("12 ") + MQTT_MSG::DAYS[language], false, "p02_hc1_holiday_days"},
      {KM271_SENDCMD_HC1_SUMMER, 21.0f, KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[language], KM_CFG_ARRAY::SUMMER[language][12], false, "p02_hc1_summer_th"},
      {KM271_SENDCMD_HC1_FROST, -5.0f, KM_CFG_TOPIC::HC1_FROST_THRESHOLD[language], "-5 °C", false, "p02_hc1_frost_prot_th"},
      {KM271_SENDCMD_HC1_SWITCH_OFF_THRESHOLD, -4.0f, KM_CFG_TOPIC::HC1_SWITCH_OFF_THRESHOLD[language], "-4 °C", false, "p02_hc1_sw_off_th"},
      {KM271_SENDCMD_HC1_SWITCH_ON_TEMP, 3.0f, KM_CFG_TOPIC::HC1_SWITCH_ON_TEMP[language], KM_CFG_ARRAY::SWITCH_ON_TEMP[language][3], false, "p02_hc1_sw_on_temp"},
      {KM271_SENDCMD_HC1_REDUCTION_MODE, 2.0f, KM_CFG_TOPIC::HC1_REDUCTION_MODE[language], KM_CFG_ARRAY::REDUCT_MODE[language][2], false, "p02_hc1_reduct_mode"},
      {KM271_SENDCMD_HC1_DAY_SETPOINT, 21.5f, KM_CFG_TOPIC::HC1_DAY_TEMP[language], "21.5 °C", true, "p02_hc1_day_temp"},
      {KM271_SENDCMD_HC1_NIGHT_SETPOINT, 17.5f, KM_CFG_TOPIC::HC1_NIGHT_TEMP[language], "17.5 °C", true, "p02_hc1_night_temp"},
      {KM271_SENDCMD_HC1_HOLIDAY_SETPOINT, 16.5f, KM_CFG_TOPIC::HC1_HOLIDAY_TEMP[language], "16.5 °C", true, "p02_hc1_holiday_temp"},
      {KM271_SENDCMD_HC2_OPMODE, 2.0f, KM_CFG_TOPIC::HC2_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false, "p02_hc2_opmode_auto"},
      {KM271_SENDCMD_HC2_DESIGN_TEMP, 70.0f, KM_CFG_TOPIC::HC2_INTERPR[language], "70 °C", false, "p02_hc2_interp"},
      {KM271_SENDCMD_HC2_PROGRAMM, 3.0f, KM_CFG_TOPIC::HC2_PROGRAM[language], KM_CFG_ARRAY::HC_PROGRAM[language][3], false, "p02_hc2_prg"},
      {KM271_SENDCMD_HC2_HOLIDAYS, 12.0f, KM_CFG_TOPIC::HC2_HOLIDAY_DAYS[language], std::string("12 ") + MQTT_MSG::DAYS[language], false, "p02_hc2_holiday_days"},
      {KM271_SENDCMD_HC2_SUMMER, 21.0f, KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[language], KM_CFG_ARRAY::SUMMER[language][12], false, "p02_hc2_summer_th"},
      {KM271_SENDCMD_HC2_FROST, -5.0f, KM_CFG_TOPIC::HC2_FROST_THRESHOLD[language], "-5 °C", false, "p02_hc2_frost_prot_th"},
      {KM271_SENDCMD_HC2_SWITCH_OFF_THRESHOLD, -4.0f, KM_CFG_TOPIC::HC2_SWITCH_OFF_THRESHOLD[language], "-4 °C", false, "p02_hc2_sw_off_th"},
      {KM271_SENDCMD_HC2_SWITCH_ON_TEMP, 3.0f, KM_CFG_TOPIC::HC2_SWITCH_ON_TEMP[language], KM_CFG_ARRAY::SWITCH_ON_TEMP[language][3], false, "p02_hc2_sw_on_temp"},
      {KM271_SENDCMD_HC2_REDUCTION_MODE, 2.0f, KM_CFG_TOPIC::HC2_REDUCTION_MODE[language], KM_CFG_ARRAY::REDUCT_MODE[language][2], false, "p02_hc2_reduct_mode"},
      {KM271_SENDCMD_HC2_DAY_SETPOINT, 21.5f, KM_CFG_TOPIC::HC2_DAY_TEMP[language], "21.5 °C", true, "p02_hc2_day_temp"},
      {KM271_SENDCMD_HC2_NIGHT_SETPOINT, 17.5f, KM_CFG_TOPIC::HC2_NIGHT_TEMP[language], "17.5 °C", true, "p02_hc2_night_temp"},
      {KM271_SENDCMD_HC2_HOLIDAY_SETPOINT, 16.5f, KM_CFG_TOPIC::HC2_HOLIDAY_TEMP[language], "16.5 °C", true, "p02_hc2_holiday_temp"},
      {KM271_SENDCMD_WW_SETPOINT, 53, KM_CFG_TOPIC::WW_TEMP[language], "53 °C", false, "p02_ww_temp"},
      {KM271_SENDCMD_WW_OPMODE, 2, KM_CFG_TOPIC::WW_OPMODE[language], KM_CFG_ARRAY::OPMODE[language][2], false, "p02_ww_opmode_auto"},
      {KM271_SENDCMD_WW_PUMP_CYCLES, 4, KM_CFG_TOPIC::WW_CIRCULATION[language], KM_CFG_ARRAY::CIRC_INTERVAL[language][4], false, "p02_ww_circ"},
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
      // Every MQTT topic and WebUI input must enqueue the same bytes as the driver.
      connect();
      const std::string topic = std::string("setvalue/") + example.topic;
      const std::string value = std::to_string(example.value);
      assert(routeMqttConfig(topic.c_str(), value.c_str()));
      Km271WriteQueue::Telegram routed;
      assert(writeQueue.front(routed));
      assert(memcmp(routed.data, telegram.data, 8) == 0);
      connect();
      assert(routeWebConfig(example.elementId, value.c_str()));
      assert(writeQueue.front(routed));
      assert(memcmp(routed.data, telegram.data, 8) == 0);

      if (!example.fractional) {
        connect();
        assert(routeMqttConfig(topic.c_str(), "1.5"));
        assert(writeQueue.size() == 0);
        if (!strstr(example.elementId, "_opmode_")) {
          assert(routeWebConfig(example.elementId, "1.5"));
          assert(writeQueue.size() == 0);
        }
      }
      for (const char *invalid : {"", "garbage", "12junk", "nan", "inf", "1e99", "1e-99", "2147483648", "-2147483649"}) {
        connect();
        assert(routeMqttConfig(topic.c_str(), invalid));
        assert(writeQueue.size() == 0);
        if (!strstr(example.elementId, "_opmode_")) {
          assert(routeWebConfig(example.elementId, invalid));
          assert(writeQueue.size() == 0);
        }
      }
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

static void testAdditionalConfigCommands() {
  struct Example { e_km271_sendCmd command; float value; bool fractional; uint8_t bytes[8]; };
  const Example cases[] = {
    {KM271_SENDCMD_HC1_MAX_TEMP, 65, false, {7,14,0x65,0x65,65,0x65,0x65,0x65}},
    {KM271_SENDCMD_HC2_MAX_TEMP, 60, false, {8,14,0x65,0x65,60,0x65,0x65,0x65}},
    {KM271_SENDCMD_HC1_HEATING_SYSTEM, 1, false, {7,28,0x65,0x65,1,0x65,0x65,0x65}},
    {KM271_SENDCMD_HC2_HEATING_SYSTEM, 3, false, {8,28,0x65,0x65,3,0x65,0x65,0x65}},
    {KM271_SENDCMD_HC1_TEMP_OFFSET, -1.5f, true, {7,49,0x65,0x65,0x65,0xfd,0x65,0x65}},
    {KM271_SENDCMD_HC2_TEMP_OFFSET, .5f, true, {8,49,0x65,0x65,0x65,1,0x65,0x65}},
    {KM271_SENDCMD_WW_PROCESSING, 0, false, {12,14,0x65,0x65,0x65,0,0x65,0x65}},
    {KM271_SENDCMD_WW_PRIORITY, 0, false, {8,21,0x65,0,0x65,0x65,0x65,0x65}},
  };
  kmConfigNum.hc2_heating_system = 1;
  for (const auto &example : cases) {
    connect();
    if (example.fractional) km271sendCmdFlt(example.command, example.value);
    else km271sendCmd(example.command, example.value);
    assert(writeQueue.size() == 1);
    beginWrite(example.bytes); // Compare actual production framing with fixed bytes.
    finishWrite();
  }
  for (const auto &invalid : std::vector<std::pair<e_km271_sendCmd,int>>{
      {KM271_SENDCMD_HC1_MAX_TEMP,19}, {KM271_SENDCMD_HC1_MAX_TEMP,91},
      {KM271_SENDCMD_HC2_MAX_TEMP,61}, // Underfloor system from the case above.
      {KM271_SENDCMD_HC1_HEATING_SYSTEM,3}, {KM271_SENDCMD_HC2_HEATING_SYSTEM,2},
      {KM271_SENDCMD_WW_PROCESSING,-1}, {KM271_SENDCMD_WW_PRIORITY,2}}) {
    connect();
    km271sendCmd(invalid.first, invalid.second);
    assert(writeQueue.size() == 0);
  }
  for (float invalid : {-5.5f,5.5f,.25f,NAN,INFINITY,-INFINITY}) {
    connect();
    km271sendCmdFlt(KM271_SENDCMD_HC1_TEMP_OFFSET, invalid);
    assert(writeQueue.size() == 0);
  }
  // Real reception overrides an assumed negative offset, preserving neighbours.
  connect();
  const uint8_t reported[] = {0,0x31,0x65,0x65,0x65,0xff,0,0xfc};
  feed(KM_STX);
  for (uint8_t byte : frame(reported,sizeof(reported))) feed(byte);
  assert(kmConfigNum.hc1_temp_offset == -.5f);
  assert(std::string(kmConfigStr.hc1_temp_offset) == "-0.5 °C");
  assert(kmConfigNum.hc1_frost_protection_threshold == -4);

  // WW priority is a global setting even when HK2 display is disabled.
  config.km271.use_hc2 = false;
  connect();
  km271sendCmd(KM271_SENDCMD_WW_PRIORITY, 1);
  const uint8_t priority[] = {8,21,0x65,1,0x65,0x65,0x65,0x65};
  beginWrite(priority); finishWrite();
  assert(kmConfigNum.ww_priority == 1);
  config.km271.use_hc2 = true;

  // Exercise the actual new MQTT and WebSocket dispatch blocks, not a model.
  config.mqtt.lang = 0;
  connect();
  routeMqttConfig("setvalue/HK1_Temperatur_Offset", "-1.5");
  const uint8_t negative[] = {7,49,0x65,0x65,0x65,0xfd,0x65,0x65};
  beginWrite(negative); finishWrite();
  for (uint8_t language : {0,1}) {
    config.mqtt.lang = language;
    connect();
    const std::string topic = std::string("setvalue/") + KM_CFG_TOPIC::WW_PROCESSING[language];
    routeMqttConfig(topic.c_str(), language ? "off" : "Aus");
    const uint8_t off[] = {12,14,0x65,0x65,0x65,0,0x65,0x65};
    beginWrite(off); finishWrite();
  }
  for (const char *invalid : {"", "garbage", "1junk", "0.25", "nan", "inf", "1000000000000000000000000000"}) {
    connect();
    routeWebConfig("p02_hc1_heatsys", invalid);
    assert(writeQueue.size() == 0);
    routeWebConfig("p02_hc1_temp_offset", invalid);
    assert(writeQueue.size() == 0);
    const std::string topic = std::string("setvalue/") + KM_CFG_TOPIC::WW_PROCESSING[config.mqtt.lang];
    routeMqttConfig(topic.c_str(), invalid);
    assert(writeQueue.size() == 0);
  }
  connect();
  routeWebConfig("p02_ww_priority", "0");
  const uint8_t offPriority[] = {8,21,0x65,0,0x65,0x65,0x65,0x65};
  beginWrite(offPriority); finishWrite();
}

static void testWebConfigButtons() {
  const char *groups[] = {"hc1", "hc2", "ww"};
  const e_km271_sendCmd commands[] = {KM271_SENDCMD_HC1_OPMODE, KM271_SENDCMD_HC2_OPMODE, KM271_SENDCMD_WW_OPMODE};
  const char *modes[] = {"night", "day", "auto"};
  for (unsigned group = 0; group < 3; ++group) {
    for (int mode = 0; mode < 3; ++mode) {
      connect();
      km271sendCmd(commands[group], mode);
      Km271WriteQueue::Telegram expected, actual;
      assert(writeQueue.front(expected));
      const std::string id = std::string("p02_") + groups[group] + "_opmode_" + modes[mode];
      // A button's ID determines the mode, irrespective of its event payload.
      for (const char *payload : {"", "1", "garbage"}) {
        connect();
        assert(routeWebConfig(id.c_str(), payload));
        assert(writeQueue.front(actual));
        assert(writeQueue.size() == 1);
        assert(memcmp(expected.data, actual.data, 8) == 0);
      }
    }
  }
  connect();
  for (const char *id : {"unknown", "p02_hc1_day_temp_help", "p02_hc1_opmode", "p02_hc1_max_temp_txt"}) {
    assert(!routeWebConfig(id, "1"));
    assert(writeQueue.size() == 0);
  }
}

static void testMqttConfigChoices() {
  for (uint8_t language : {0, 1}) {
    config.mqtt.lang = language;
    struct Choice { const char *topic; e_km271_sendCmd command; const char *text; int value; };
    const Choice choices[] = {
      {KM_CFG_TOPIC::HC1_OPMODE[language], KM271_SENDCMD_HC1_OPMODE, KM_CFG_ARRAY::OPMODE[language][2], 2},
      {KM_CFG_TOPIC::HC2_PROGRAM[language], KM271_SENDCMD_HC2_PROGRAMM, KM_CFG_ARRAY::HC_PROGRAM[language][3], 3},
      {KM_CFG_TOPIC::HC1_REDUCTION_MODE[language], KM271_SENDCMD_HC1_REDUCTION_MODE, KM_CFG_ARRAY::REDUCT_MODE[language][2], 2},
      {KM_CFG_TOPIC::HC2_HEATING_SYSTEM[language], KM271_SENDCMD_HC2_HEATING_SYSTEM, KM_CFG_ARRAY::HEATING_SYSTEM[language][1], 1},
      {KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[language], KM271_SENDCMD_HC1_SUMMER, KM_CFG_ARRAY::SUMMER[language][0], 9},
      {KM_CFG_TOPIC::HC1_SUMMER_THRESHOLD[language], KM271_SENDCMD_HC1_SUMMER, KM_CFG_ARRAY::SUMMER[language][1], 10},
      {KM_CFG_TOPIC::HC2_SUMMER_THRESHOLD[language], KM271_SENDCMD_HC2_SUMMER, KM_CFG_ARRAY::SUMMER[language][22], 31},
      {KM_CFG_TOPIC::WW_OPMODE[language], KM271_SENDCMD_WW_OPMODE, KM_CFG_ARRAY::OPMODE[language][1], 1},
      {KM_CFG_TOPIC::WW_CIRCULATION[language], KM271_SENDCMD_WW_PUMP_CYCLES, KM_CFG_ARRAY::CIRC_INTERVAL[language][0], 0},
      {KM_CFG_TOPIC::WW_CIRCULATION[language], KM271_SENDCMD_WW_PUMP_CYCLES, KM_CFG_ARRAY::CIRC_INTERVAL[language][4], 4},
      {KM_CFG_TOPIC::WW_CIRCULATION[language], KM271_SENDCMD_WW_PUMP_CYCLES, KM_CFG_ARRAY::CIRC_INTERVAL[language][7], 7},
      {KM_CFG_TOPIC::WW_PRIO[language], KM271_SENDCMD_WW_PRIORITY, KM_CFG_ARRAY::ON_OFF[language][0], 0},
    };
    for (const auto &choice : choices) {
      connect();
      km271sendCmd(choice.command, choice.value);
      Km271WriteQueue::Telegram expected, actual;
      assert(writeQueue.front(expected));
      connect();
      const std::string topic = std::string("setvalue/") + choice.topic;
      assert(routeMqttConfig(topic.c_str(), choice.text));
      assert(writeQueue.front(actual));
      assert(memcmp(expected.data, actual.data, 8) == 0);
    }
  }
  connect();
  assert(!routeMqttConfig("setvalue/unknown", "1"));
  assert(writeQueue.size() == 0);
}

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
  testAdditionalConfigCommands();
  testMqttConfigChoices();
  testWebConfigButtons();
  testClockCorrection();
  testTimers();
  std::puts("KM271 production serial engine: all replay checks passed");
}
