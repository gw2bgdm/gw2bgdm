#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <atlconv.h>
#include "core/file.h"
#include "meter/config.h"
#include "meter/resource.h"
#include "meter/localization.h"

#define LOCALIZATION_INI_SECT L"Localization"

typedef struct LocalText {

	LocalText() {};
	LocalText(const char *name, const char *text) {
		fieldName = name;
		fieldText = text;
	}

	ATL::CAtlStringA fieldName;
	ATL::CAtlStringA fieldText;

} LocalText;

typedef ATL::CAtlMap<ATL::CAtlStringA, LocalText*> LocalTextMap;
static ATL::CAtlArray<LocalText> m_localTextArr;
static LocalTextMap m_localTextMap;

static wchar_t *m_directory = NULL;
static wchar_t *m_filename = NULL;

size_t localtext_size()
{
	return m_localTextArr.GetCount();
}

bool localtext_init(wchar_t *directory, wchar_t *filename)
{
	m_directory = directory;
	m_filename = filename;

	// Create the language directory (if needed)
	// And load the default language file
	CreateDirectoryW(m_directory, 0);

#if !(defined BGDM_TOS_COMPLIANT)
	// Check if the chinese lang file exists
	// if not get the file from the resource and deploy
	ATL::CAtlStringW cn_path;
	cn_path.Format(L"%s\\Simplified Chinese", m_directory);
	if (!file_existsW(cn_path))
	{
		LPVOID pFile = NULL;
		DWORD dwBytes = 0;
		if (GetEmbeddedResource(IDR_RT_LANG_CHINESE, &pFile, &dwBytes)) {
			file_writeW(cn_path, pFile, dwBytes);
		}
	}
#endif	// !(defined BGDM_TOS_COMPLIANT)

	localtext_load_file(m_filename);
	return true;
}

void localtext_load_defaults()
{
	//m_localTextArr.RemoveAll();
	//m_localTextMap.RemoveAll();
	m_localTextArr.SetCount(LOCALIZED_TEXT_END);

#undef LOCALTEXT
#define LOCALTEXT(_idx, _text) \
	localtext_set(_idx, #_idx, _text);
#include "meter/localization.inc"
#undef LOCALTEXT
}

bool localtext_load_file(const wchar_t *file)
{
	static wchar_t buf[1024] = { 0 };

	localtext_load_defaults();

	if (lstrlenW(file) <= 0) return false;

	ATL::CAtlStringW path;
	path.Format(L"%s\\%s", m_directory, file);

	if (file_existsW(path)) {
		
		for (int i = 0; i < m_localTextArr.GetCount(); ++i)
		{
			const ATL::CAtlStringW utf16 = CA2W(m_localTextArr[i].fieldName, CP_UTF8);
			memset(buf, 0, sizeof(buf));
			GetPrivateProfileStringW(LOCALIZATION_INI_SECT, utf16, NULL, buf, ARRAYSIZE(buf), path.GetString());
			if (lstrlenW(buf) > 0) {
				ATL::CAtlStringA text = CW2A(buf, CP_UTF8);
				if (text.Find("\\n") || text.Find("\\r\\n")) {
					text.Replace("\\r\\n", "\r\n");
					text.Replace("\\n", "\n");
				}
				m_localTextArr[i].fieldText = text;
			}
		}
		return true;
	}
	return false;
}

bool localtext_save_file(const wchar_t *file)
{
	if (lstrlenW(file) <= 0) return false;

	ATL::CAtlStringW path;
	path.Format(L"%s\\%s", m_directory, file);

	if (!file_existsW(path)) {
		file_writeW(path, "", 0);
	}

	if (file_existsW(path)) {

		for (int i = 0; i < m_localTextArr.GetCount(); ++i)
		{
			const ATL::CAtlStringW key = CA2W(m_localTextArr[i].fieldName, CP_UTF8);
			const ATL::CAtlStringW val = CA2W(m_localTextArr[i].fieldText, CP_UTF8);
			CAtlStringW altval;
			const wchar_t *value = val.GetString();
			if (val.Find(L"\n") || val.Find(L"\r\n")) {
				altval = val;
				altval.Replace(L"\r\n", L"\\r\\n");
				altval.Replace(L"\n", L"\\n");
				value = altval.GetString();
			}
			WritePrivateProfileStringW(LOCALIZATION_INI_SECT, key, value, path.GetString());
		}
		return true;
	}
	return false;
}

const char *localtext_get(int idx)
{
	static const char null_string[] = "(null)";
	if (idx < m_localTextArr.GetCount())
		return m_localTextArr[idx].fieldText.GetString();
	return null_string;
}

const char *localtext_get_fmt(int idx, const char *fmt, ...)
{
	static ATL::CAtlStringA str;
	str.Empty();
	str = localtext_get(idx);
	va_list args;
	va_start(args, fmt);
	str.AppendFormatV(fmt, args);
	va_end(args);
	return str.GetString();
}

const char *localtext_get_name(int idx)
{
	static const char null_string[] = "(null)";
	if (idx < m_localTextArr.GetCount())
		return m_localTextArr[idx].fieldName.GetString();
	return null_string;
}

__inline bool localtext_set(int idx, const char* name, const char *text)
{
	if (idx < m_localTextArr.GetCount()) {
		m_localTextArr[idx].fieldName = name;
		m_localTextArr[idx].fieldText = text;
		m_localTextMap[name] = &m_localTextArr[idx];
		return true;
	}
	return false;
}

bool localtext_set_name(const char *name, const char *text)
{
	LocalTextMap::CPair *pair = m_localTextMap.Lookup(name);
	if (pair != nullptr) {
		pair->m_value->fieldText = text;
		return true;
	}
	return false;
}