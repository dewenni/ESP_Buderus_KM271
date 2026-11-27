// Included by km271_writes.cpp: exercise the production cache, encoder and UART.
#include "timer_fixtures.cpp"

static std::vector<uint8_t> hexBytes(const std::string &hex) {
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i < hex.size(); i += 2) bytes.push_back(std::stoul(hex.substr(i, 2), nullptr, 16));
  return bytes;
}
static void receiveTimers(uint8_t hc, const uint8_t *raw) {
  for (uint8_t block = 0; block < 14; ++block) {
    const uint16_t reg = (hc ? 0x169 : 0x100) + 7 * (block + 1);
    uint8_t data[8] = {uint8_t(reg >> 8), uint8_t(reg)};
    memcpy(data + 2, raw + block * 6, 6);
    parseInfo(data, 8);
  }
}
static void finishTimerRequest() {
  while (writeQueue.size()) {
    Km271WriteQueue::Telegram next;
    assert(writeQueue.front(next));
    beginWrite(next.data); finishWrite();
  }
}
static void seedTimers() {
  connect();
  for (uint8_t hc = 0; hc < 2; ++hc) {
    const auto raw = hexBytes(timerFixtures[0].before[hc]);
    receiveTimers(hc, raw.data());
    assert(km271GetTimers(hc).valid == Km271Timers::Complete);
  }
}
static void testTimers() {
  using namespace Km271Timers;
  // All recorded before/frame/after vectors use the real ACK updater/framer.
  for (const auto &fixture : timerFixtures) {
    connect();
    for (uint8_t hc = 0; hc < 2; ++hc) {
      const auto raw = hexBytes(fixture.before[hc]); receiveTimers(hc, raw.data());
    }
    for (const auto &wire : fixture.frames) {
      const auto bytes = hexBytes(wire);
      uint8_t data[8], count = 0;
      size_t pos = 0;
      while (count < 8) {
        data[count] = bytes[pos++];
        if (data[count] == KM_DLE) { assert(bytes[pos] == KM_DLE); ++pos; }
        ++count;
      }
      assert(frame(data, 8) == bytes);
      assert(writeQueue.enqueue(reinterpret_cast<const uint8_t (*)[8]>(data), 1));
      beginWrite(data); finishWrite();
    }
    for (uint8_t hc = 0; hc < 2; ++hc) {
      const auto expected = hexBytes(fixture.after[hc]);
      assert(!memcmp(km271GetTimers(hc).raw, expected.data(), Bytes));
    }
  }

  for (uint8_t hc = 0; hc < 2; ++hc) {
    seedTimers();
    Change change;
    change.on = 2; change.off = 3; change.days = 2; change.from = 370; change.to = 1330;
    uint8_t before[Bytes]; memcpy(before, km271GetTimers(hc).raw, Bytes);
    const auto revision = km271GetTimers(hc).revision;
    assert(!strcmp(km271WriteTimers(hc, timerSession, revision - 1, 1, change), "timer_stale"));
    assert(!strcmp(km271WriteTimers(hc, timerSession + 1, revision, 1, change), "timer_stale"));
    assert(writeQueue.size() == 0);
    assert(!km271WriteTimers(hc, timerSession, revision, 123, change));
    assert(writeQueue.size() == 2 && km271TimersBusy());
    assert(km271GetTimers(hc).state == KmTimerWriteState::Pending);
    assert(!memcmp(km271GetTimers(hc).raw, before, Bytes));
    const uint8_t scalar[][8] = {{0x0c, 7, 0x65, 0x65, 0x65, 53, 0x65, 0x65}};
    assert(!km271QueueWrite(scalar, 1));
    assert(!strcmp(km271WriteTimers(1 - hc, timerSession, km271GetTimers(1 - hc).revision, 124, change), "timer_busy"));
    Km271WriteQueue::Telegram next;
    assert(writeQueue.front(next) && next.data[0] == 0x11 + hc && next.data[1] == 14);
    beginWrite(next.data); finishWrite();
    assert(km271GetTimers(hc).state == KmTimerWriteState::Pending);
    assert(writeQueue.front(next) && next.data[1] == 7);
    beginWrite(next.data); finishWrite();
    assert(km271GetTimers(hc).state == KmTimerWriteState::Saved && !km271TimersBusy());
    assert(km271GetTimers(hc).raw[4] == 0x21 && km271GetTimers(hc).raw[5] == 37);
    assert(km271GetTimers(hc).raw[6] == 0x20 && km271GetTimers(hc).raw[7] == 133);
    for (unsigned i = 0; i < Bytes; ++i) if (i < 4 || i > 7) assert(km271GetTimers(hc).raw[i] == before[i]);
    assert(!memcmp(km271GetTimers(1 - hc).raw, hexBytes(timerFixtures[0].before[1 - hc]).data(), Bytes));

    // 0x10 and 0x65 are actual times, not keep bytes; MQTT texts use the ACK path.
    change.on = 0; change.off = 1; change.days = 1; change.from = 160; change.to = 1010;
    configMessages.clear();
    assert(!km271WriteTimers(hc, timerSession, km271GetTimers(hc).revision, 125, change));
    finishTimerRequest();
    assert(km271GetTimers(hc).raw[1] == 0x10 && km271GetTimers(hc).raw[3] == 0x65);
    assert(!configMessages.empty() && configMessages.back().second.find("02:40") != std::string::npos && configMessages.back().second.find("16:50") != std::string::npos);
    assert(configMessages.back().first == (hc ? KM_CFG_TOPIC::HC2_TIMER01[config.mqtt.lang] : KM_CFG_TOPIC::HC1_TIMER01[config.mqtt.lang]));

    // Incomplete receive and raw service timer writes cannot bypass admission.
    const uint8_t rawWrite[][8] = {{uint8_t(0x11 + hc), 7, 1, 2, 3, 4, 5, 6}};
    assert(!km271QueueWrite(rawWrite, 1));
    uint8_t shortBlock[] = {uint8_t(hc ? 1 : 1), uint8_t(hc ? 0x70 : 7), 0, 0};
    const auto oldRevision = km271GetTimers(hc).revision;
    parseInfo(shortBlock, sizeof(shortBlock));
    assert(km271GetTimers(hc).revision == oldRevision);

    // First block succeeds, second is NAKed: keep known ACK bytes, mark failure,
    // invalidate both caches, discard pending parts and require fresh receive.
    change.on = 2; change.off = 3; change.days = 2; change.from = 380; change.to = 1340;
    assert(!km271WriteTimers(hc, timerSession, oldRevision, 126, change));
    assert(writeQueue.front(next)); beginWrite(next.data); finishWrite();
    assert(writeQueue.front(next));
    for (unsigned i = 0; i <= KM271_WRITE_RETRIES; ++i) { beginWrite(next.data); feed(KM_NAK); }
    assert(writeQueue.size() == 0 && !km271TimersBusy());
    assert(km271GetTimers(hc).state == KmTimerWriteState::Failed && !km271GetTimers(hc).valid);
    assert(km271GetTimers(hc).raw[7] == 134 && km271GetTimers(hc).raw[5] == 37);
    feed(KM_DLE); // late ACK does not report success
    assert(km271GetTimers(hc).state == KmTimerWriteState::Failed);
  }

  // Multi-day add touches every one of the 14 blocks, then removes a cross-block pair.
  seedTimers();
  uint8_t emptyRaw[Bytes];
  for (unsigned i = 0; i < Points; ++i) { emptyRaw[2 * i] = 0xc2; emptyRaw[2 * i + 1] = 0x90; }
  receiveTimers(0, emptyRaw);
  Change add; add.add = true; add.days = 127;
  for (uint16_t hour : {uint16_t(0), uint16_t(8), uint16_t(16)}) {
    add.from = hour * 60; add.to = (hour + 1) * 60;
    assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 200 + hour, add));
    finishTimerRequest();
  }
  Period periods[MaxPeriods]; uint8_t count;
  assert(decode(km271GetTimers(0).raw, periods, count) && count == 21);
  assert(!strcmp(km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 210, add), "timer_overlap"));
  Change remove; remove.remove = true; remove.on = 2; remove.off = 3;
  assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 211, remove));
  finishTimerRequest();
  assert(empty(km271GetTimers(0).raw + 4) && empty(km271GetTimers(0).raw + 6));
  // Free positions, not a new position order, are reused by add.
  add.days = 2; add.from = 120; add.to = 180;
  assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 212, add));
  finishTimerRequest();
  assert(km271GetTimers(0).raw[4] == 0x21 && km271GetTimers(0).raw[6] == 0x20);

  // Sunday 24:00 is a real OFF, distinct from an unused point.
  receiveTimers(0, emptyRaw);
  add.days = 64; add.from = 1430; add.to = 1440;
  assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 220, add)); finishTimerRequest();
  assert(km271GetTimers(0).raw[2] == 0xc0 && km271GetTimers(0).raw[3] == 0x90);
  assert(decode(km271GetTimers(0).raw, periods, count) && count == 1 && periods[0].end == Week);

  // External mutation cancels queued writes instead of overwriting fresh neighbours.
  seedTimers();
  Change edit; edit.on = 0; edit.off = 1; edit.days = 1; edit.from = 370; edit.to = 1310;
  assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 230, edit));
  uint8_t changedBlock[] = {1, 7, 1, 35, 0, 131, 0x21, 36};
  parseInfo(changedBlock, sizeof(changedBlock));
  assert(!km271TimersBusy() && !writeQueue.size() && km271GetTimers(0).state == KmTimerWriteState::Failed);
  assert(km271GetTimers(0).valid != Complete);

  // A timer timeout also reports failure; late ACK/reconnect cannot revive it.
  for (bool sent : {false, true}) {
    seedTimers();
    assert(!km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 240, edit));
    feed(KM_STX);
    if (sent) feed(KM_DLE);
    clockMs += KM271_ACK_TIMEOUT;
    cyclicKM271();
    assert(!km271TimersBusy() && !writeQueue.size() && km271GetTimers(0).state == KmTimerWriteState::Failed);
    assert(!km271GetTimers(0).valid && !km271GetTimers(1).valid);
    feed(KM_DLE);
    feed(KM_STX); feed(KM_DLE); feed(KM_DLE);
    assert(km271GetTimers(0).state == KmTimerWriteState::Failed);
    assert(!strcmp(km271WriteTimers(0, timerSession, km271GetTimers(0).revision, 241, edit), "timer_loading"));
  }
  std::puts("KM271 timers: recorded vectors, raw cache, encoder and ACK/failure checks passed");
}
