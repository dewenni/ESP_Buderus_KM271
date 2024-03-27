#pragma once

/* I N C L U D E S ****************************************************/
#include <HTTPClient.h>
#include <basics.h>

/* P R O T O T Y P E S ********************************************************/
int checkGithubUpdates(const char *owner, const char *repo, char *version, size_t size_version, char *url, size_t size_url);
int compareVersions(char *version1, char *version2);