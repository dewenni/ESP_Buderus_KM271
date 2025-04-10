#pragma once

/* P R O T O T Y P E S ********************************************************/
const char *onOffString(uint8_t value);
const char *errOkString(uint8_t value);
void webReadLogBufferCyclic();
bool webLogRefreshActive();
void webSetLogType(e_logTyp typ);
void webClearLog();