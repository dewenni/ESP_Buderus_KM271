#pragma once
#include <Arduino.h>
#include <cassert>
#include <deque>
#include <string>
#include <vector>
#include <message.h>

static uint32_t clockMs = 100;
static unsigned long millis() { return clockMs; }
static int criticalDepth = 0;
using portMUX_TYPE = int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(mux) (++criticalDepth)
#define portEXIT_CRITICAL(mux) (--criticalDepth)

struct HostSerial {
  std::deque<uint8_t> incoming;
  std::vector<std::vector<uint8_t>> outgoing;
  size_t readBytes(uint8_t *value, size_t count) {
    assert(!criticalDepth && count == 1);
    if (incoming.empty()) return 0;
    *value = incoming.front();
    incoming.pop_front();
    return 1;
  }
  size_t write(const uint8_t *data, size_t count) {
    assert(!criticalDepth);
    outgoing.emplace_back(data, data + count);
    return count;
  }
};
static HostSerial Serial2;
static struct {
  struct { bool use_alarmMsg = true; bool use_hc1 = true; bool use_hc2 = true; } km271;
  struct { uint8_t lang = 0; } mqtt;
} config;
