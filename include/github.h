#include <GithubReleaseOTA.h>

struct GithubReleaseInfo {
  char tag[12];
  char asset[128];
  char url[256];
};

bool ghGetLatestRelease(GithubRelease *release, GithubReleaseInfo *info);
int ghStartOtaUpdate(GithubRelease release, const char *asset);
void ghSetProgressCallback(void (*callback)(int));