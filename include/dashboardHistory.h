#pragma once

#include <stdint.h>

// One shared minute sample: boiler, hot water, solar, HC1, HC2, outside.
// Each pair uses the same order as data-chart in 01_dash.html. Temperatures
// are tenths of a degree; validity bits 0..5 mark pairs, 6/7 mark the states.
struct DashboardSample {
  uint32_t time; // monotonic seconds since boot, independent of NTP adjustments
  int16_t temperatures[12];
  uint8_t valid;
  uint8_t states; // bit 0: burner, bit 1: hot water priority
};
static_assert(sizeof(DashboardSample) == 32, "Keep the history sample compact");

class DashboardHistoryBuffer {
public:
  static constexpr uint32_t Capacity = 61;
  void append(const DashboardSample &sample) {
    rows[latest % Capacity] = sample;
    ++latest;
    if (count < Capacity) ++count;
  }
  uint32_t end() const { return latest; }
  const DashboardSample *get(uint32_t sequence) const {
    return sequence && sequence <= latest && latest - sequence < count ? &rows[(sequence - 1) % Capacity] : nullptr;
  }
  uint32_t first(uint32_t now) const {
    uint32_t sequence = latest - count + 1;
    while (sequence <= latest && now - get(sequence)->time > 3600) ++sequence;
    return sequence;
  }

private:
  DashboardSample rows[Capacity]{};
  uint32_t latest = 0;
  uint32_t count = 0;
};

void dashboardHistoryCyclic();
void dashboardHistoryReply(uint32_t clientId, const char *request);
