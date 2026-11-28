# Build checks

GitHub OTA asset selection is compiled from production `src/github.cpp` for
ESP32 and ESP32-S3 by `test_github_ota.py` (C++17 compiler required). It checks
mixed/reordered assets, legacy ESP32 names, exact chip/version matching,
missing assets, stale result clearing and invalid release metadata. GitHub
network access and flashing are stubbed; real downloads, image validation,
USB, Ethernet and OTA reboot still require hardware tests.

Firmware builds: `pio run -e esp32` and then `pio run -e esp32s3`. Run them
sequentially because WebUI generation writes shared output. Both use the pinned
EspWebUI dependency from `platformio.ini`. Release notes must have the matching
version heading. S3 update asset names deliberately omit the substring `ota`, so older ESP32
firmware which selects by that substring cannot pick an S3 image, regardless
of asset order.

The MQTT discovery replay compiles the complete production `mqttDiscovery.cpp`
with the real ArduinoJson headers from `.pio/libdeps/esp32/ArduinoJson/src`.
It requires a C++11 compiler and installed PlatformIO dependencies; it is skipped
with a reason when either is missing. Run it alone with
`python3 -B -m unittest discover -s tests -p test_mqtt_discovery.py`.
Local publication capture checks DE/EN JSON for all eight new controls, command
and state topics, slider limits/steps, valid choices, old-sensor deletion before
replacement, repeated discovery, reset, disabled groups and empty prefixes.
It also checks HK2's dynamic 60/90 °C limit, including a change during an ongoing
discovery pass. It uses real serialization but replaces the timer ticks and MQTT
publisher; no broker, Home Assistant instance or device is contacted. Actual
entity migration, dashboards/automations and command round trips require a
separate installation test.

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
stubbed. The production helpers and 35 parameter senders are compiled too,
using the real DE/EN dictionaries. The value decoder is replaced except for
the actual received WW/clock and additional HK config cases and timer-register dispatch. Test sources and the executable are
generated in a temporary directory.

It covers atomic batch admission, capacity and FIFO wrap-around, owned command
bytes, sequential requests, distinct send permission/final ACK, no log restart
on success, preserved validity, NAK retries and group cancellation, timeouts,
late ACK, reconnect, incoming framing/checksum and clock rollover. It also
checks all 35 parameter updates and config-message topic/payload in DE/EN,
ACK-only cache updates, untouched neighbouring/status values, partial failure,
keep bytes, unknown commands and subsequent real WW config reception. MQTT
publication uses the existing production message path; no broker is contacted.
Critical
section stubs verify that UART/logging run outside the lock; they do not model
real scheduling or prove thread safety on the ESP32. The checks do not confirm
device timing, parameter adoption, actual MQTT delivery or browser refresh.
Command payloads and acknowledgement limits are documented in the
[MQTT reference](../Doc/MQTT.md#write-commands). Hardware adoption and recovery
after interrupted writes require separate device testing.

The additional configuration replay checks fixed wire bytes for all eight new
commands, signed negative/half-degree adjustments, value limits, invalid enum
values, underfloor maximum 60 °C, NaN/infinity rejection and WW priority when
HK2 display is disabled. Receive and ACK paths use the same field handlers.
The UI check also verifies a received underfloor limit without replacing the
reported value text or emitting a device command. The actual shared MQTT
configuration handler is replayed for all 35 commands in DE/EN, comparing its
queued telegrams with direct driver calls. Additional cases cover localized
choices (including summer/winter's 9..31 mapping), malformed/empty values,
NaN/infinity, overflow, underflow, fractional input for integer commands and
unknown topics. The shared WebSocket configuration handler is replayed for
all 35 commands too, including invalid numeric inputs and all nine fixed-value
operation-mode buttons (41 element IDs in total). Unknown IDs and display/help
IDs must not enqueue a write. These host tests use no broker and do not
confirm adoption of the new WW or HK parameters on a running installation.

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
For user-facing behavior and transfer limits, see the
[weekly program guide](../README.md#weekly-program). End-to-end operation
through the ESP, WebUI and MQTT requires separate device testing.

Equipment page navigation and contextual help (Node.js 18+, no packages):

```sh
node --test tests/equipment_pages.test.cjs
```

The Python suite includes this check. It executes production navigation/help
code with a DOM double: local tabs and keyboard navigation, contextual help,
translated descriptions, close events and no device sends. It does not prove
browser layout, native touch/focus behavior or controller adoption. See the
[equipment page guide](../README.md#equipment-pages) for the documented UI.
Browser rendering and controller adoption require separate practical checks.
