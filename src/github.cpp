#include <Arduino.h>

#include <github.h>
#include <message.h>

#define GITHUB_OWNER "dewenni"
#define GITHUB_REPO "ESP_Buderus_KM271"

static const char *TAG = "GITHUB"; // LOG TAG

GithubReleaseOTA ota(GITHUB_OWNER, GITHUB_REPO);

void ghSetProgressCallback(void (*callback)(int)) { ota.setProgressCallback(callback); }

bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info) {

  if (release == nullptr || info == nullptr) {
    return false;
  }

  // Get the latest release from GitHub
  *release = ota.getLatestRelease();

  // Überprüfe, ob das Release erfolgreich abgerufen wurde
  if (release->tag_name == nullptr || release->html_url == nullptr) {
    return false; // Fehler, wenn die Daten fehlen
  }

  snprintf(info->tag, sizeof(info->tag), "%s", release->tag_name);
  snprintf(info->url, sizeof(info->url), "%s", release->html_url);
  ESP_LOGI(TAG, "GitHb latest Release: %s", info->tag);

  // search for the first asset that contains "ota" in its name
  const char *otaAssetName = NULL;
  for (const auto &asset : release->assets) {
    if (asset.name != NULL && strstr(asset.name, "ota") != NULL) {
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
    return false;
  }
}

int ghStartOtaUpdate(GithubRelease release, const char *asset) {
  int result = ota.flashFirmware(release, asset);

  if (result == 0) {
    ESP_LOGI(TAG, "Firmware updated successfully");
  } else {
    ESP_LOGE(TAG, "Firmware update failed: %i", result);
    ota.freeRelease(release);
  }
  return result;
}