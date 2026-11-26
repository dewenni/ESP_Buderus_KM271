#include <crashDiagnostics.h>
#include <config.h>
#include <basics.h>
#include <algorithm>
#include <webUI.h>
#include <esp_core_dump.h>
#include <esp_partition.h>
#include <esp_system.h>

// All access runs on the loop task, including erase and chunk reads.
static const esp_partition_t *partition;
static esp_err_t dumpState = ESP_ERR_NOT_FOUND;
static size_t dumpOffset, dumpSize;
static uint32_t bootId;
static esp_core_dump_summary_t summary;
static bool summaryValid;
static char panicReason[160];

void crashDiagnosticsSetup() {
  bootId = esp_random();
  partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, nullptr);
  if (!partition) return;
  // The SDK erase API leaves a 0xffffffff size marker. Its checksum check
  // reports ESP_ERR_INVALID_SIZE for this normal empty state as well.
  uint32_t storedSize = 0;
  dumpState = esp_partition_read(partition, 0, &storedSize, sizeof(storedSize));
  if (dumpState != ESP_OK) return;
  if (storedSize == UINT32_MAX) {
    dumpState = ESP_ERR_NOT_FOUND;
    return;
  }
  dumpState = esp_core_dump_image_check();
  if (dumpState != ESP_OK) return;
  size_t address = 0;
  dumpState = esp_core_dump_image_get(&address, &dumpSize);
  if (dumpState != ESP_OK) return;
  if (address < partition->address || address - partition->address > partition->size ||
      dumpSize > partition->size - (address - partition->address)) {
    dumpState = ESP_ERR_INVALID_SIZE;
    return;
  }
  dumpOffset = address - partition->address;
  summaryValid = esp_core_dump_get_summary(&summary) == ESP_OK && summary.exc_task[0] && summary.exc_pc;
  esp_core_dump_get_panic_reason(panicReason, sizeof(panicReason));
  ESP_LOGW("CRASH", "Saved core dump: %u bytes, summary %s", unsigned(dumpSize), summaryValid ? "available" : "unavailable");
}

// Hex transport keeps even quoted passwords within the existing 256-byte queue.
static bool authorized(const char *hex) {
  if (!hex || !serviceAccessEnabled()) return false;
  const char *password = servicePassword();
  const size_t length = strlen(password);
  if (strlen(hex) != length * 2) return false;
  const char digits[] = "0123456789abcdef";
  unsigned difference = 0;
  for (size_t i = 0; i < length; ++i) {
    const uint8_t c = password[i];
    difference |= hex[i * 2] ^ digits[c >> 4];
    difference |= hex[i * 2 + 1] ^ digits[c & 15];
  }
  return difference == 0;
}

void crashDiagnosticsReply(uint32_t clientId, const char *request) {
  JsonDocument input, reply;
  if (deserializeJson(input, request) || !input["id"].is<uint32_t>()) return;
  reply["type"] = "crashDiagnostics";
  reply["id"] = input["id"];
  reply["boot"] = bootId;
  const char *op = input["op"] | "";
  const char *error = nullptr;
  if (strcmp(op, "status") != 0 && !authorized(input["password"] | "")) error = "crash_auth";
  else if (strcmp(op, "status") != 0 && EspSysUtil::OTA::getInstance().isActive()) error = "crash_busy";
  else if (!strcmp(op, "erase")) {
    if (!input["boot"].is<uint32_t>() || input["boot"].as<uint32_t>() != bootId) error = "crash_changed";
    else if (!partition || esp_core_dump_image_erase() != ESP_OK) error = "crash_error";
    else {
      dumpState = ESP_ERR_NOT_FOUND;
      dumpSize = 0;
      summaryValid = false;
      panicReason[0] = 0;
      reply["erased"] = true;
    }
  } else if (!strcmp(op, "read")) {
    const uint32_t offset = input["offset"] | UINT32_MAX;
    if (dumpState != ESP_OK || !input["boot"].is<uint32_t>() || input["boot"].as<uint32_t>() != bootId || offset >= dumpSize)
      error = "crash_changed";
    else {
      uint8_t bytes[384];
      char hex[sizeof(bytes) * 2 + 1];
      const size_t count = std::min<size_t>(sizeof(bytes), dumpSize - offset);
      if (esp_partition_read(partition, dumpOffset + offset, bytes, count) != ESP_OK) error = "crash_error";
      else {
        const char digits[] = "0123456789abcdef";
        for (size_t i = 0; i < count; ++i) {
          hex[i * 2] = digits[bytes[i] >> 4];
          hex[i * 2 + 1] = digits[bytes[i] & 15];
        }
        hex[count * 2] = 0;
        reply["offset"] = offset;
        reply["data"] = hex;
      }
    }
  } else if (strcmp(op, "status") && strcmp(op, "info")) error = "crash_error";

  reply["state"] = !partition ? "crash_no_partition" : dumpState == ESP_OK ? "crash_available" :
                     dumpState == ESP_ERR_NOT_FOUND ? "crash_none" : "crash_invalid";
  reply["size"] = dumpState == ESP_OK ? dumpSize : 0;
  reply["reset"] = esp_reset_reason();
  if (!error && !strcmp(op, "info") && dumpState == ESP_OK) {
    reply["reason"] = panicReason;
    if (summaryValid) {
      reply["task"] = summary.exc_task;
      reply["pc"] = summary.exc_pc;
      reply["sha256"] = reinterpret_cast<const char *>(summary.app_elf_sha256);
      JsonArray trace = reply["backtrace"].to<JsonArray>();
      for (size_t i = 0; i < std::min(size_t(summary.exc_bt_info.depth), size_t(16)); ++i) trace.add(summary.exc_bt_info.bt[i]);
      reply["corrupted"] = summary.exc_bt_info.corrupted;
    }
  }
  if (error) reply["error"] = error;
  webUI.wsSendClient(clientId, reply);
}
