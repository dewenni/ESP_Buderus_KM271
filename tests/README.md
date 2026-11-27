# Build checks

Run from the Buderus repository with Python 3.9+:

```sh
python3 -B -m unittest discover -s tests
```

The simulation check runs the actual version script in a temporary project.
It verifies generation without `web/output`, unchanged output timestamps on a
repeat run, version updates and preservation of the `web/sim.json` source and
other simulation values. It does not compile firmware or test a device.

For the WebUI generator regression checks, see the sibling EspWebUI repository.

Alarm memory decoding and state transitions (Node.js 18+, no packages):

```sh
node --test tests/alarms.test.cjs
```

These checks execute the actual WebUI decoder and cover open/cleared messages,
relative day offsets, unknown codes, incomplete reception, disabled reception,
serial/browser disconnects and cached slots after reconnect. They do not verify
serial hardware, the meaning of a controller acknowledgement, or browser layout.
The second timestamp still needs comparison with real fault clearing and manual
acknowledgement on the test controller. The WebUI currently labels it “Cleared”.

The Python host suite also invokes these Node.js checks when `node` is on PATH;
it reports a skipped test otherwise. Install Node.js 18+ for the full host suite.

Crash download state and chunk validation (Node.js 18+, no packages):

```sh
node --test tests/crash_diagnostics.test.cjs
```

These tests execute the actual browser code with a simulated WebSocket and DOM.
They cover chunk bounds and identity, request matching, interrupted transfers,
completed downloads without implicit erase, and cancelled deletion. The Python
host suite includes them when Node.js is available. They do not test flash,
ESP-IDF panic capture or browser layout. Hardware procedure and ELF archive
requirements are documented in `Doc/Absturzdiagnose.md`.

KM271 write queue and serial handshake replay (C++11 compiler `c++`, Python 3.9+):

```sh
python3 -B -m unittest discover -s tests -p 'test_km271_writes.py'
```

The full Python suite includes this check and skips it if `c++` is unavailable.
It compiles the actual production prefix of `src/km271.cpp` up to the application
decoder `parseInfo`, with the real timer encoder, `km271.h` and `km271WriteQueue.h`. Only Arduino
clock/UART, critical sections, hardware configuration and message delivery are
stubbed. The production helpers and 27 parameter senders are compiled too,
using the real DE/EN dictionaries. The value decoder is replaced except for
the actual received WW/clock config cases and timer-register dispatch. Test sources and the executable are
generated in a temporary directory.

It covers atomic batch admission, capacity and FIFO wrap-around, owned command
bytes, sequential requests, distinct send permission/final ACK, no log restart
on success, preserved validity, NAK retries and group cancellation, timeouts,
late ACK, reconnect, incoming framing/checksum and clock rollover. It also
checks all 27 parameter updates and config-message topic/payload in DE/EN,
ACK-only cache updates, untouched neighbouring/status values, partial failure,
keep bytes, unknown commands and subsequent real WW config reception. MQTT
publication uses the existing production message path; no broker is contacted.
Critical
section stubs verify that UART/logging run outside the lock; they do not model
real scheduling or prove thread safety on the ESP32. The checks do not confirm
device timing, parameter adoption, actual MQTT delivery or browser refresh.
Hardware procedure and limits are in `Doc/KM271-Schreiben.md`.

The clock-correction replay covers the display-confirmed `01E0` payloads for
0, -6 and +4 seconds/day from the original KM271 (2026-11-27), plus -30/+30.
It checks signed decoding without half-degree scaling, the second payload-byte
position independently of the unknown first byte, and DE/EN config text with
the existing MQTT topics. It does not test long-term clock accuracy, real MQTT
delivery or the rendered WebUI/Home Assistant unit.

ID04 adds `km271_timers.cpp` to that replay. It tests all 18 recorded positive
timer vectors from `fixtures/km271_timers.json` with the production UART framer
and ACK updater, then tests the production timer encoder/cache/admission:
cross-block ordering, stable positions and neighbours, both circuits, all blocks,
special time bytes, MQTT topic/text, multi-day allocation, capacity, midnight,
stale revisions/sessions, concurrent writes, short packets, partial NAK, timeout,
late ACK and external mutation. The fixture source is a direct original-KM271
test from 2026-11-27; replay does not access a device.

Timer browser model and request state (Node.js 18+, no packages):

```sh
node --test tests/weekly_program.test.cjs
```

The Python suite also invokes these checks. They run the actual raw decoder,
local validator and dialog state logic, covering recorded snapshots, neighbours,
capacity, conflicts, 24:00/week boundary, malformed/non-pairable data, no sends
on local edits/cancel, request matching, final-ACK-only success, stale/incomplete
data, disconnect and lost replies without automatic resend. A small DOM double
models controls and events; it cannot verify layout, tooltips or native focus.
Safari layout/dialog checks and the remaining ESP/WebUI/MQTT hardware procedure
are described in `Doc/KM271-Schaltzeiten.md`.

Equipment page navigation and contextual help (Node.js 18+, no packages):

```sh
node --test tests/equipment_pages.test.cjs
```

The Python suite includes this check. It executes production navigation/help
code with a DOM double: local tabs and keyboard navigation, contextual help,
translated descriptions, close events and no device sends. It does not prove
browser layout, native touch/focus behavior or controller adoption. See
`Doc/WebUI-Anlagenseiten.md` for the remaining browser/hardware checks.
