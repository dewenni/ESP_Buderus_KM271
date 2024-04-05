#include <version.h>

/**
 * *******************************************************************
 * @brief   function that checks for new releases on github
 * @param   owner         github owner of repository
 * @param   repo          repository
 * @param   version       pointer version string
 * @param   size_version  sizeof version string
 * @return  http status code
 * *******************************************************************/
int checkGithubUpdates(const char *owner, const char *repo, char *version, size_t size_version, char *url, size_t size_url) {
  const int bufferSize = 256; // max length for one line of stream
  char buffer[bufferSize];    // buffer for one line of stream
  char gitUrl[256];
  snprintf(gitUrl, sizeof(gitUrl), "https://api.github.com/repos/%s/%s/releases/latest", owner, repo);

  HTTPClient http;
  http.begin(gitUrl);
  int httpCode = http.GET();
  bool urlFound = false, nameFound = false;

  if (httpCode == HTTP_CODE_OK) {
    WiFiClient *stream = http.getStreamPtr();
    while (stream->available()) {
      size_t bytesRead = stream->readBytesUntil(',', buffer, bufferSize - 1); // read until first separator or max buffer size
      buffer[bytesRead] = '\0';

      // check for: "html_url":"
      if (!urlFound) {
        if (strstr(buffer, "\"html_url\":\"") != NULL) {
          urlFound = true;
          char *urlStart = strstr(buffer, "https://");
          char *urlEnd = strchr(urlStart, '"'); // pointer to end of url
          if (urlStart != NULL && urlEnd != NULL) {
            size_t urlLength = urlEnd - urlStart;
            if (urlLength < size_url) {
              memcpy(url, urlStart, urlLength);
              url[urlLength] = '\0';
            } else {
              // Handle buffer overflow error
              url[0] = '\0'; // Null terminate the url string
              http.end();
              return -2; // Indicate buffer overflow error
            }
          }
        }
      }
      // check for: "name":"
      else if (!nameFound) {
        if (strstr(buffer, "\"name\":\"") != NULL) {
          nameFound = true;
          char *versionStart = strstr(buffer, "\"name\":\"") + 8; // pointer to beginning of version
          char *versionEnd = strchr(versionStart, '"');           // pointer to end of version
          if (versionStart != NULL && versionEnd != NULL) {
            size_t versionLength = versionEnd - versionStart;
            if (versionLength < size_version) {
              memcpy(version, versionStart, versionLength);
              version[versionLength] = '\0';
            } else {
              // Handle buffer overflow error
              version[0] = '\0'; // Null terminate the version string
              http.end();
              return -2; // Indicate buffer overflow error
            }
          }
        }
      }
      // we have found what we're looking for
      if (nameFound && urlFound) {
        break;
      }
    } // end while
  }   // end httpCode == HTTP_CODE_OK
  else {
    // Handle HTTP request error
    http.end();
    return -1; // Indicate HTTP request error
  }

  http.end();
  return httpCode;
}

/**
 * *******************************************************************
 * @brief   check version string format
 * @param   version
 * @return  1=valid / 0=invalid
 * *******************************************************************/
int isValidVersionFormat(char *version) {
  if (version[0] != 'v')
    return 0;
  int dotCount = 0;
  int i = 1; // Start nach dem 'v'
  while (version[i] != '\0') {
    if (isdigit(version[i]) || version[i] == '.') {
      if (version[i] == '.')
        dotCount++;
    } else {
      return 0; // invalid character
    }
    i++;
  }
  return (dotCount == 2);
}

/**
 * *******************************************************************
 * @brief   check version string format
 * @param   version1
 * @param   version2
 * @return  0:v1=v2 / 1:v1>v2 / -1:v1<v2 / 255:invalid format
 * *******************************************************************/
int compareVersions(char *version1, char *version2) {
  // check version format
  if (!isValidVersionFormat(version1) || !isValidVersionFormat(version2)) {
    return 255; // invalid format
  }

  // start comparison after "v"
  char *v1 = version1 + 1;
  char *v2 = version2 + 1;

  // divide version in major, minor and patch
  int major1, minor1, patch1;
  int major2, minor2, patch2;
  if (sscanf(v1, "%d.%d.%d", &major1, &minor1, &patch1) != 3 || sscanf(v2, "%d.%d.%d", &major2, &minor2, &patch2) != 3) {
    return 255; // invalid format
  }

  // compare major, minor and patch
  if (major1 < major2) {
    return -1;
  } else if (major1 > major2) {
    return 1;
  } else { // major1 == major2
    if (minor1 < minor2) {
      return -1;
    } else if (minor1 > minor2) {
      return 1;
    } else { // minor1 == minor2
      if (patch1 < patch2) {
        return -1;
      } else if (patch1 > patch2) {
        return 1;
      } else { // patch1 == patch2
        return 0;
      }
    }
  }
}