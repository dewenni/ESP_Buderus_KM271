"""Compile production GitHub asset selection for both chips, without networking."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

STUB = r'''
#pragma once
#include <cstdio>
#include <cstring>
#include <vector>
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
struct GithubReleaseAsset { const char *name = nullptr; };
struct GithubRelease {
  const char *tag_name = nullptr;
  const char *html_url = nullptr;
  std::vector<GithubReleaseAsset> assets;
};
static GithubRelease response;
class GithubReleaseOTA {
public:
  GithubReleaseOTA(const char *, const char *) {}
  void setProgressCallback(void (*)(int)) {}
  GithubRelease getLatestRelease() { return response; }
  int flashFirmware(GithubRelease, const char *) { return 0; }
  void freeRelease(GithubRelease &) {}
};
'''

HARNESS = r'''
#include <cassert>
#include <algorithm>
#include "github.cpp"
int main() {
  const char *classic = "buderus_km271_ota_update_v6.0.2.bin";
  const char *s3 = "buderus_km271_esp32s3_update_v6.0.2.bin";
  assert(strstr(s3, "ota") == nullptr); // Legacy ESP32 selector must ignore S3.
  const char *expected = CONFIG_IDF_TARGET_ESP32S3 ? s3 : classic;
  const char *other = CONFIG_IDF_TARGET_ESP32S3 ? classic : s3;
  GithubRelease release;
  GithubReleaseInfo info{};
  response = {"v6.0.2", "https://example.test/release", {
    {nullptr}, {"some_ota.bin"}, {other},
    {"buderus_km271_esp32s3_flash_v6.0.2.bin"}, {expected}}};
  assert(ghGetLatestRelease(&release, &info));
  assert(strcmp(info.asset, expected) == 0);
  std::reverse(response.assets.begin(), response.assets.end());
  assert(ghGetLatestRelease(&release, &info));
  assert(strcmp(info.asset, expected) == 0);
  // A previously found asset must not survive a release without a match.
  response.assets = {{other}, {"buderus_km271_ota_update_v6.0.1.bin"},
                     {"buderus_km271_esp32s3_update_v6.0.1.bin"}};
  assert(ghGetLatestRelease(&release, &info));
  assert(info.asset[0] == '\0');
  assert(strcmp(info.tag, "v6.0.2") == 0);
  response.assets = {};
  assert(ghGetLatestRelease(&release, &info) && info.asset[0] == '\0');
  response.tag_name = nullptr;
  assert(!ghGetLatestRelease(&release, &info) && info.asset[0] == '\0');
  response.tag_name = "v6.0.2";
  response.html_url = nullptr;
  assert(!ghGetLatestRelease(&release, &info));
  assert(!ghGetLatestRelease(nullptr, &info));
  assert(!ghGetLatestRelease(&release, nullptr));
}
'''


class GithubOtaTests(unittest.TestCase):
    def test_chip_specific_asset_selection(self):
        compiler = shutil.which('c++')
        if not compiler:
            self.skipTest('c++ is required')
        with tempfile.TemporaryDirectory() as directory:
            tmp = Path(directory)
            (tmp / 'Arduino.h').write_text(STUB)
            (tmp / 'GithubReleaseOTA.h').write_text('#include <Arduino.h>\n')
            (tmp / 'message.h').write_text('')
            (tmp / 'check.cpp').write_text(HARNESS)
            for s3 in (0, 1):
                with self.subTest(esp32s3=s3):
                    exe = tmp / 'check'
                    result = subprocess.run([compiler, '-std=c++17',
                                    f'-DCONFIG_IDF_TARGET_ESP32S3={s3}',
                                    '-I', str(tmp), '-I', str(ROOT / 'include'),
                                    '-I', str(ROOT / 'src'), str(tmp / 'check.cpp'),
                                    '-o', str(exe)], capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    subprocess.run([str(exe)], check=True, capture_output=True)


if __name__ == '__main__':
    unittest.main()
