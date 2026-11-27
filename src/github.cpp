#include <Arduino.h>

#include <github.h>
#include <message.h>

// NOTE: GithubReleaseOTA is used without setCa(), so the download is encrypted
// but the server certificate is NOT verified, and the image itself is
// unsigned. Anyone able to spoof DNS or MITM the device's egress can serve
// arbitrary firmware here. Treat this button as trusted-network-only.
// Older releases are labelled explicitly in the Tools page before installation.
#define GITHUB_OWNER "dewenni"
#define GITHUB_REPO "ESP_Buderus_KM271"

static const char *TAG = "GITHUB"; // LOG TAG

GithubReleaseOTA ota(GITHUB_OWNER, GITHUB_REPO);

void ghSetProgressCallback(void (*callback)(int)) { ota.setProgressCallback(callback); }

bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info) {

  if (release == nullptr || info == nullptr) {
    return false;
  }
  *info = {};

  // Get the latest release from GitHub
  *release = ota.getLatestRelease();

  // Überprüfe, ob das Release erfolgreich abgerufen wurde
  if (release->tag_name == nullptr || release->html_url == nullptr) {
    return false; // Fehler, wenn die Daten fehlen
  }

  snprintf(info->tag, sizeof(info->tag), "%s", release->tag_name);
  snprintf(info->url, sizeof(info->url), "%s", release->html_url);
  ESP_LOGI(TAG, "GitHb latest Release: %s", info->tag);

  // Match the complete application image name for this chip and release.
  // Keep the legacy ESP32 name so existing releases remain usable.
  // S3 names omit "ota" so older ESP32 firmware cannot select them.
  char expectedAsset[128];
#if CONFIG_IDF_TARGET_ESP32S3
  const char *prefix = "buderus_km271_esp32s3_update_";
#else
  const char *prefix = "buderus_km271_ota_update_";
#endif
  if (snprintf(expectedAsset, sizeof(expectedAsset), "%s%s.bin", prefix, release->tag_name) >= static_cast<int>(sizeof(expectedAsset))) {
    return false;
  }
  const char *otaAssetName = NULL;
  for (const auto &asset : release->assets) {
    if (asset.name != NULL && strcmp(asset.name, expectedAsset) == 0) {
      otaAssetName = asset.name;
      break;
    }
  }

  // copy the asset name to the info struct
  if (otaAssetName != NULL) {
    ESP_LOGI(TAG, "OTA Asset found: %s", otaAssetName);
    snprintf(info->asset, sizeof(info->asset), "%s", otaAssetName);
    return true;
  } else {
    ESP_LOGE(TAG, "No OTA Asset found!");
    return true;
  }
}

int ghStartOtaUpdate(GithubRelease release, const char *asset) {
  int result = ota.flashFirmware(release, asset);

  if (result == 0) {
    ESP_LOGI(TAG, "Firmware updated successfully");
  } else {
    ESP_LOGE(TAG, "Firmware update failed: %i", result);

  }
  return result;
}
void ghFreeRelease(GithubRelease &release) {
  ota.freeRelease(release);
  release = {};
}
