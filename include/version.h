#pragma once

#include <basics.h>
#include <HTTPClient.h>

int checkGithubUpdates(const char *owner, const char *repo, char *version, size_t size_version, char *url, size_t size_url);
int compareVersions(char *version1, char *version2);