#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>

const char *int8ToString(int8_t value);
const char *uint8ToString(uint8_t value);
const char *uint16ToString(uint16_t value);
const char *int32ToString(int32_t value);
const char *uint64ToString(uint64_t value);
const char *intmaxToString(intmax_t value);
const char *floatToString(float value);
const char *floatToString4(float value);
const char *floatToString8(float value);
const char *doubleToString(double value);
bool stringToBool(const char *str);
char *strcat_safe(char *dest, const char *src, size_t dest_size);
void getRestartReason(char *reason, size_t reason_size);
const char *getDateTimeString();
const char *getDateString();
const char *getDateStringWeb();
const char *getTimeString();
unsigned int strHash(const char *str);
bool strDiff(unsigned int *lastHash, const char *currentValue);
void replace_underscores(const char *input, char *output, size_t output_size);
void replace_whitespace(const char *input, char *output, size_t output_size);
void to_lowercase(const char *input, char *output, size_t output_size);
bool isNumber(const char *str);
void readJSONstring(char *dest, size_t size, const char *src);
unsigned long hash(void *str, size_t len);
const char *formatBytes(unsigned long long bytes);