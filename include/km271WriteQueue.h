#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Fixed RAM budget; the eventual ID04 encoder must check its complete request
// against this limit. No assumptions about timer write telegrams are made here.
constexpr size_t KM271_WRITE_QUEUE_CAPACITY = 32;

// Access is protected by sendBufMux in km271.cpp. The front remains in the
// queue until acknowledged, including during retries.
class Km271WriteQueue {
public:
  struct Telegram {
    uint8_t data[8];
    bool last;
  };

  bool enqueue(const uint8_t (*telegrams)[8], size_t count) {
    if (!telegrams || !count || count > KM271_WRITE_QUEUE_CAPACITY - used) return false;
    for (size_t i = 0; i < count; ++i) {
      Telegram &entry = entries[(head + used + i) % KM271_WRITE_QUEUE_CAPACITY];
      memcpy(entry.data, telegrams[i], sizeof(entry.data));
      entry.last = (i + 1 == count);
    }
    used += count;
    return true;
  }

  bool front(Telegram &telegram) const {
    if (!used) return false;
    telegram = entries[head];
    return true;
  }

  void pop() {
    if (!used) return;
    head = (head + 1) % KM271_WRITE_QUEUE_CAPACITY;
    --used;
  }

  void discardRequest() {
    while (used) {
      bool last = entries[head].last;
      pop();
      if (last) break;
    }
  }

  size_t size() const { return used; }
  void clear() { head = used = 0; }

private:
  Telegram entries[KM271_WRITE_QUEUE_CAPACITY] = {};
  size_t head = 0;
  size_t used = 0;
};
