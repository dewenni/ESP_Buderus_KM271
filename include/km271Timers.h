#pragma once

#include <stdint.h>
#include <string.h>

// Three two-byte points per block; positions never change when editing.
namespace Km271Timers {
constexpr uint8_t Blocks = 14, Points = 42, Bytes = 84, MaxPeriods = 21;
constexpr uint16_t Complete = (1U << Blocks) - 1;
constexpr uint16_t Week = 7 * 1440;
struct Period { uint8_t on, off; uint16_t start, end; };
struct Change {
  bool add = false, remove = false, next = false;
  uint8_t on = 0, off = 0, days = 0;
  uint16_t from = 0, to = 0;
};
inline bool empty(const uint8_t *p) { return p[0] == 0xc2 && p[1] == 0x90; }

// Accept only unambiguous alternating points. Unsupported data is preserved,
// never silently paired by physical position or normalised for display/writes.
inline bool decode(const uint8_t raw[Bytes], Period periods[MaxPeriods], uint8_t &count) {
  struct Point { uint16_t time; uint8_t index; bool on; } points[Points];
  uint8_t used = 0;
  count = 0;
  for (uint8_t i = 0; i < Points; ++i) {
    const uint8_t *p = raw + i * 2;
    if (empty(p)) continue;
    const uint8_t day = p[0] >> 5, state = p[0] & 31;
    if (day > 6 || state > 1 || p[1] > 144 || (state && p[1] == 144)) return false;
    Point point = {uint16_t((day * 1440 + p[1] * 10) % Week), i, state == 1};
    uint8_t j = used;
    while (j && points[j - 1].time > point.time) { points[j] = points[j - 1]; --j; }
    points[j] = point;
    ++used;
  }
  if (!used) return true;
  if (used % 2) return false;
  for (uint8_t i = 0; i < used; ++i) {
    const Point &p = points[i], &next = points[(i + 1) % used];
    if (p.on == next.on || p.time == next.time) return false;
    if (p.on) periods[count++] = {p.index, next.index, p.time,
                                 uint16_t(next.time + (next.time <= p.time ? Week : 0))};
  }
  return true;
}

inline void setPoint(uint8_t *raw, uint8_t index, uint16_t time, bool on) {
  // Keep midnight OFF as 24:00 of the preceding day, including Sunday.
  time %= Week;
  if (!on && time % 1440 == 0) {
    raw[index * 2] = uint8_t((((time / 1440) + 6) % 7) << 5);
    raw[index * 2 + 1] = 144;
  } else {
    raw[index * 2] = uint8_t((time / 1440) << 5) | uint8_t(on);
    raw[index * 2 + 1] = uint8_t((time % 1440) / 10);
  }
}

// Returns a translated-message key on error; writes only the local output.
inline const char *apply(const uint8_t raw[Bytes], const Change &change, uint8_t output[Bytes]) {
  Period periods[MaxPeriods];
  uint8_t count;
  if (!decode(raw, periods, count)) return "timer_invalid";
  if (!change.add) {
    bool found = false;
    for (uint8_t i = 0; i < count; ++i)
      found |= periods[i].on == change.on && periods[i].off == change.off;
    if (!found) return "timer_stale";
  }
  if (!change.remove && (!change.days || change.days > 127 || change.from >= 1440 ||
      change.to > 1440 || change.from % 10 || change.to % 10 ||
      change.to + (change.next ? 1440 : 0) <= change.from)) return "timer_order";
  if (!change.add && !change.remove && (change.days & (change.days - 1))) return "timer_invalid";
  memcpy(output, raw, Bytes);
  if (!change.add) {
    output[change.on * 2] = output[change.off * 2] = 0xc2;
    output[change.on * 2 + 1] = output[change.off * 2 + 1] = 0x90;
  }
  if (!change.remove) for (uint8_t day = 0; day < 7; ++day) {
    if (!(change.days & (1U << day))) continue;
    const uint16_t start = day * 1440 + change.from;
    const uint16_t end = day * 1440 + change.to + (change.next ? 1440 : 0);
    // Enforce disjoint intervals, including points at a shared boundary.
    for (uint8_t i = 0; i < count; ++i) {
      if (!change.add && periods[i].on == change.on) continue;
      for (int shift = -int(Week); shift <= int(Week); shift += Week)
        if (int(start) <= int(periods[i].end) + shift && int(end) >= int(periods[i].start) + shift)
          return "timer_overlap";
    }
    uint8_t on = Points, off = Points;
    if (!change.add) { on = change.on; off = change.off; }
    else for (uint8_t i = 0; i < Points; ++i) if (empty(output + i * 2)) {
      if (on == Points) on = i;
      else { off = i; break; }
    }
    if (off == Points) return "timer_capacity";
    setPoint(output, on, start, true);
    setPoint(output, off, end, false);
  }
  if (!decode(output, periods, count)) return "timer_overlap";
  return nullptr;
}
} // namespace Km271Timers
