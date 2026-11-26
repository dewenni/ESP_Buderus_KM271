#pragma once
#include <stdint.h>

void crashDiagnosticsSetup();
void crashDiagnosticsReply(uint32_t clientId, const char *request);
