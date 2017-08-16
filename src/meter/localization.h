#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOCALTEXT(_idx, _text) _idx,
enum LocalizationStrings {

	LOCALIZED_TEXT_START = -1,

#include "meter/localization.inc"

	LOCALIZED_TEXT_END
};
#undef LOCALTEXT

size_t localtext_size();
bool localtext_init(wchar_t *directory, wchar_t *filename);
void localtext_load_defaults();
bool localtext_load_file(const wchar_t *file);
bool localtext_save_file(const wchar_t *file);
const char *localtext_get(int idx);
const char *localtext_get_fmt(int idx, const char *fmt, ...);
const char *localtext_get_name(int idx);
bool localtext_set(int idx, const char *name, const char *text);
bool localtext_set_name(const char *name, const char *text);

#ifdef LOCALTEXT
#undef LOCALTEXT
#endif
#define LOCALTEXT		localtext_get
#define LOCALTEXT_FMT	localtext_get_fmt

#ifdef __cplusplus
}
#endif
