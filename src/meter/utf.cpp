#include "utf.h"
#include <atlstr.h>
#include <atlconv.h>
#include <atlenc.h>
#include <Strsafe.h>

char *utf16_to_utf8(const wchar_t* utf16, char* buff, size_t bytes)
{
	if (!utf16 || !buff || bytes == 0)
		return NULL;

	const ATL::CAtlStringA utf8 = CW2A(utf16, CP_UTF8);
	StringCbCopyA(buff, bytes, utf8.GetString());
	return buff;
}


wchar_t *utf8_to_utf16(const char* utf8, wchar_t* buff, size_t bytes)
{
	if (!utf8 || !buff || bytes == 0)
		return NULL;

	const ATL::CAtlStringW utf16 = CA2W(utf8, CP_UTF8);
	StringCbCopyW(buff, bytes, utf16.GetString());
	return buff;
}

char *utf8_to_base64(const char *utf8, char* buff, size_t bytes)
{
	if (!utf8 || !buff || bytes == 0)
		return NULL;

	int srcLen = lstrlenA(utf8);
	int dstLen = ATL::Base64EncodeGetRequiredLength(srcLen);
	if (dstLen > bytes)
		return NULL;

	ATL::Base64Encode((BYTE*)utf8, srcLen, buff, &dstLen);
	return buff;
}

char *base64_to_utf8(const char *base64, char* buff, size_t bytes)
{
	if (!base64 || !buff || bytes == 0)
		return NULL;

	int srcLen = lstrlenA(base64);
	int dstLen = ATL::Base64DecodeGetRequiredLength(srcLen);
	if (dstLen > bytes)
		return NULL;

	ATL::Base64Decode(base64, srcLen, (BYTE*)buff, &dstLen);
	return buff;
}