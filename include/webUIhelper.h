#pragma once

/* P R O T O T Y P E S ********************************************************/
void getBuildDateTime(char *formatted_date);
const char *onOffString(uint8_t value);
const char *errOkString(uint8_t value);
void webReadLogBufferCyclic();
bool webLogRefreshActive();
