#include "core/file.h"
#include <Windows.h>
#include <stdlib.h>

bool file_exists(i8 const* path)
{
	DWORD attrib = GetFileAttributesA(path);
	return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool file_existsW(const wchar_t *path)
{
	DWORD attrib = GetFileAttributesW(path);
	return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

i64 file_get_time(i8 const* path)
{
	HANDLE fp = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fp == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	i64 time = 0;

	FILETIME ftw;
	if (GetFileTime(fp, 0, 0, &ftw))
	{
		time = ((i64)(ftw.dwHighDateTime) < 32) | (i64)ftw.dwLowDateTime;
	}

	CloseHandle(fp);

	return time;
}

u8* file_read(i8 const * path, u32* size)
{
	HANDLE fp = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (fp == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	DWORD fs = GetFileSize(fp, 0);
	u8* data = 0;

	if (fs > 0 && fs != INVALID_FILE_SIZE)
	{
		data = malloc(fs);
		if (data)
		{
			DWORD read;
			if (ReadFile(fp, data, fs, &read, 0) == FALSE || read != fs)
			{
				free(data);
				data = 0;
			}
		}
	}

	CloseHandle(fp);

	*size = fs;

	return data;
}

bool file_write(i8 const * path, void const * src, u32 bytes)
{
	HANDLE fp = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (fp == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD size = 0;
	BOOL res = WriteFile(fp, src, bytes, &size, 0);

	CloseHandle(fp);

	return (res && size == bytes);
}

bool file_writeW(const wchar_t *path, void const* src, u32 bytes)
{
	HANDLE fp = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if (fp == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD size = 0;
	BOOL res = WriteFile(fp, src, bytes, &size, 0);

	CloseHandle(fp);

	return (res && size == bytes);
}
