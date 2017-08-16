#pragma once
#include "core/types.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if the file exists at the given path.
bool file_exists(i8 const* path);
bool file_existsW(const wchar_t *path);

// Returns the last modify time for the given path.
i64 file_get_time(i8 const* path);

// Reads a file from disk and returns an allocated buffer.
u8* file_read(i8 const* path, u32* size);

// Writes a buffer to disk.
bool file_write(i8 const* path, void const* src, u32 bytes);
bool file_writeW(const wchar_t *path, void const* src, u32 bytes);

#ifdef __cplusplus
}
#endif
