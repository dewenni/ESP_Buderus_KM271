#include <dashboardHistory.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <km271.h>
#include <webUI.h>

static DashboardHistoryBuffer history;
static uint32_t lastSample = 0;
static bool started = false;

static uint32_t historySeconds() { return esp_timer_get_time() / 1000000ULL; }

// Called on the loop task, including when no browser is connected.
void dashboardHistoryCyclic() {
  const uint32_t now = historySeconds();
  const uint8_t valid = km271ChartValidity();
  if (!started && !valid) return; // Never turn the zero-initialized status into measurements.
  if (started && now - lastSample < 60) return;
  const auto &status = *km271GetStatusValueAdr();
  DashboardSample sample{};
  sample.time = now;
  sample.valid = valid;
  const int16_t values[] = {
      status.BoilerForwardTargetTemp, status.BoilerForwardActualTemp,
      status.HotWaterTargetTemp, status.HotWaterActualTemp,
      status.SolarWW, status.SolarCollector,
      status.HC1_HeatingForwardTargetTemp, status.HC1_HeatingForwardActualTemp,
      status.HC2_HeatingForwardTargetTemp, status.HC2_HeatingForwardActualTemp,
      status.OutsideTemp, status.OutsideDampedTemp};
  for (uint8_t i = 0; i < 12; ++i) sample.temperatures[i] = values[i] * 10;
  sample.states = (status.BurnerStates != 0 ? 1 : 0) | ((status.HC1_OperatingStates_1 & (1U << 3)) ? 2 : 0);
  history.append(sample);
  lastSample = now;
  started = true;
}

// Reuse ArduinoJson and EspWebUI's WebSocket serialization. Only four samples
// are expanded to JSON at once; the fixed ring is never copied per client.
void dashboardHistoryReply(uint32_t clientId, const char *request) {
  unsigned long requestId, cursor, end;
  char extra;
  if (!request || sscanf(request, "%lu:%lu:%lu%c", &requestId, &cursor, &end, &extra) != 3 ||
      requestId > UINT32_MAX || cursor > UINT32_MAX || end > UINT32_MAX) return;
  const uint32_t now = historySeconds();
  const uint32_t first = history.first(now), latest = history.end();
  const bool reset = !cursor || cursor < first || cursor > latest + 1 || (end && (end > latest || end < cursor));
  if (reset) { cursor = first; end = latest; }
  else if (!end) end = latest;
  static const uint32_t boot = esp_random();
  JsonDocument doc;
  doc["type"] = "dashboardHistory";
  doc["request"] = requestId;
  doc["boot"] = boot;
  doc["reset"] = reset;
  doc["now"] = now;
  doc["end"] = end;
  const uint32_t elapsed = now - lastSample;
  doc["nextIn"] = end < latest ? 1 : !started ? 5 : elapsed < 60 ? 60 - elapsed : 1;
  JsonArray rows = doc["rows"].to<JsonArray>();
  for (uint8_t i = 0; cursor <= end && i < 4; ++i, ++cursor) {
    const DashboardSample *sample = history.get(cursor);
    if (!sample) return; // A retry starts a fresh snapshot if the ring was overwritten.
    JsonArray row = rows.add<JsonArray>();
    row.add(sample->time);
    row.add(sample->valid);
    row.add(sample->states);
    for (const auto value : sample->temperatures) row.add(value);
  }
  doc["next"] = cursor <= end ? cursor : 0;
  webUI.wsSendClient(clientId, doc);
}
