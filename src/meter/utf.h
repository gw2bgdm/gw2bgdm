#pragma once
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

char *utf16_to_utf8(const wchar_t* utf16, char* buff, size_t bytes);
wchar_t *utf8_to_utf16(const char* utf8, wchar_t* buff, size_t bytes);

char *utf8_to_base64(const char *utf8, char* buff, size_t bytes);
char *base64_to_utf8(const char *base64, char* buff, size_t bytes);

#ifdef __cplusplus
}
#endif


