#pragma once

#include <basics.h>
#include <HTTPClient.h>

int checkGithubUpdates(const char *owner, const char *repo, char *version, size_t size_version);
int compareVersions(char *version1, char *version2);

/* usage example

  if ("TRIGGER" && WiFi.isConnected()){

    char gitHubVersion[32];
    checkGithubUpdates("dewenni", "ESP_Buderus_KM271", gitHubVersion, sizeof(gitHubVersion));
    
    Serial.println(gitHubVersion);
    
    char actVersion[32];
    strcpy(actVersion, VERSION);
    int result = compareVersions(actVersion, gitHubVersion);
    if (result == 1) {
        Serial.printf("Version neuer\n");
    } else if (result == -1) {
        Serial.printf("es gibt eine neue Version\n");
    } else if (result == 0) {
        Serial.printf("Version ist akutell\n");
    } else if (result == 255) {
        Serial.printf("Ungültiges Version Format\n");
    }
  }


*/