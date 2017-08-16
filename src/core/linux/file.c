#include "core/file.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

bool file_exists(i8 const* path)
{
	return (access(path, F_OK) != -1);
}

i64 file_get_time(i8 const* path)
{
	struct stat st;
	if (stat(path, &st) != 0)
	{
		return 0;
	}
	
	return (i64)st.st_mtime;
}

u8* file_read(i8 const * path, u32* size)
{
	FILE* fp = fopen(path, "rb");
	if (fp == 0) 
	{
		return 0;
	}

	struct stat st = { 0 };
	stat(path, &st);
	u32 fs = (u32)st.st_size;	
	
	u8* data = 0;

	if (fs > 0)
	{
		data = malloc(fs);
		if (data)
		{
			if (fread(data, fs, 1, fp) != 1) 
			{
				free(data);
				data = 0;
			}
		}
	}

	fclose(fp);

	*size = fs;

	return data;
}

bool file_write(i8 const * path, void const * src, u32 bytes)
{
	FILE* fp = fopen(path, "wb");
	if (fp == 0)
	{
		return false;
	}
	
	bool res = (fwrite(src, bytes, 1, fp) == 1);
	fclose(fp);

	return res;
}
