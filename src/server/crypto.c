#include "crypto.h"
#include "core/types.h"
#include "core/file.h"
#include "core/logging.h"
#include "server/debug.h"
#include <stdlib.h>
#if defined(_WIN32) || defined (_WIN64)
#include <Windows.h>
#include <wincrypt.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#else
#include <string.h>
#include <unistd.h>
#define MAX_PATH 260
#endif

// Key file paths.
#define PATH_KEY_PRI "bgdm.key"
#define PATH_KEY_PUB "bgdm.pub"
#define PATH_SIG_EXT ".sig"
#define KEY_SIZE 4096
#define CRYPT_CONTAINER TEXT("gw2bgdm")

#if defined(_WIN32) || defined (_WIN64)
static HCRYPTPROV g_prov;
static HCRYPTKEY g_key_pub;
static HCRYPTKEY g_key_pri;
#endif

static u8 g_pubkey_data[KEY_SIZE];
static u32 g_pubkey_bytes;


u8* crypto_pubkey_data()
{
	return g_pubkey_data;
}

u32 crypto_pubkey_bytes()
{
	return g_pubkey_bytes;
}


bool crypto_create(void)
{

	i8 pub_key[MAX_PATH] = { 0 };
	i8 pri_key[MAX_PATH] = { 0 };

#if defined(_WIN32) || defined (_WIN64)
	if (!CryptAcquireContext(&g_prov, CRYPT_CONTAINER, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, 0/*CRYPT_MACHINE_KEYSET*/))
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
			if (!CryptAcquireContext(&g_prov, CRYPT_CONTAINER, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_NEWKEYSET /*| CRYPT_MACHINE_KEYSET*/))
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	GetModuleFileNameA(NULL, pub_key, MAX_PATH);
	GetModuleFileNameA(NULL, pri_key, MAX_PATH);
	PathRemoveFileSpecA(pub_key);
	PathRemoveFileSpecA(pri_key);
	PathCombineA(pub_key, pub_key, PATH_KEY_PUB);
	PathCombineA(pri_key, pri_key, PATH_KEY_PRI);

	if (file_exists(pri_key) == false && file_exists(pub_key) == false)
	{
		HCRYPTKEY key;
		if (CryptGenKey(g_prov, AT_KEYEXCHANGE, (4096 << 16) | CRYPT_ARCHIVABLE | CRYPT_EXPORTABLE, &key))
		{
			u8 buf[KEY_SIZE];
			DWORD size;

			size = KEY_SIZE;
			CryptExportKey(key, 0, PUBLICKEYBLOB, 0, buf, &size);
			file_write(pub_key, buf, size);

			size = KEY_SIZE;
			CryptExportKey(key, 0, PRIVATEKEYBLOB, 0, buf, &size);
			file_write(pri_key, buf, size);

			CryptDestroyKey(key);
		}
	}

#else
	i8 cwd[MAX_PATH];
	getcwd(cwd, MAX_PATH);
	sprintf(pub_key, "%s/%s", cwd, PATH_KEY_PUB);
	sprintf(pri_key, "%s/%s", cwd, PATH_KEY_PRI);

	if (file_exists(pri_key) == false || file_exists(pub_key) == false)
	{
		LOG_ERR("[server] unable to find private/public key pair at %s", pri_key);
		return false;
	}

#endif


	u32 size;
	u8* data = file_read(pub_key, &size);
	if (data)
	{
#if defined(_WIN32) || defined (_WIN64)
		CryptImportKey(g_prov, data, size, 0, 0, &g_key_pub);
#endif
		memset(g_pubkey_data, 0, sizeof(g_pubkey_data));
		g_pubkey_bytes = min(size, (u32)sizeof(g_pubkey_data));
		memcpy(g_pubkey_data, data, g_pubkey_bytes);
		free(data);
	}

#if defined(_WIN32) || defined (_WIN64)
	data = file_read(pri_key, &size);
	if (data)
	{
		CryptImportKey(g_prov, data, size, 0, 0, &g_key_pri);
		free(data);
	}
#endif

	return true;
}

void crypto_destroy(void)
{
#if defined(_WIN32) || defined (_WIN64)
	if (g_key_pri)
	{
		CryptDestroyKey(g_key_pri);
		g_key_pri = 0;
	}

	if (g_key_pub)
	{
		CryptDestroyKey(g_key_pub);
		g_key_pub = 0;
	}

	if (g_prov)
	{
		CryptReleaseContext(g_prov, 0);
		g_prov = 0;
	}
#endif
}

#if defined(_WIN32) || defined (_WIN64)
bool crypto_sign_bytes(void const* data, u32 size, u8** dst, u32 *dst_bytes, i8 const* path)
{

	if (g_key_pri == 0 || data == 0 || size == 0 || dst == 0 || dst_bytes == 0)
	{
		return false;
	}

	bool res = false;
	HCRYPTHASH hash = 0;
	if (CryptCreateHash(g_prov, CALG_SHA_256, 0, 0, &hash))
	{
		if (CryptHashData(hash, data, size, 0))
		{
			static u8 sig[1024];
			DWORD sig_bytes = ARRAYSIZE(sig);
			memset(sig, 0, sig_bytes);
			if (CryptSignHash(hash, AT_KEYEXCHANGE, 0, 0, sig, &sig_bytes))
			{
				res = true;
				*dst = sig;
				*dst_bytes = sig_bytes;

				if (path) {
					i8 newPath[MAX_PATH] = { 0 };
					wsprintfA(newPath, path);
					if (PathRenameExtensionA(newPath, PATH_SIG_EXT)) {
						file_write(newPath, sig, sig_bytes);
					}
				}
			}
		}
		CryptDestroyHash(hash);
	}

	return res;
}

bool crypto_sign(i8 const* path)
{
	static i8 newPath[MAX_PATH] = { 0 };

	if (g_key_pri == 0)
	{
		return false;
	}

	u32 size;
	u8* data = file_read(path, &size);
	if (data == 0)
	{
		return false;
	}

	u8 *sig = 0;
	DWORD sig_bytes = 0;
	bool res = crypto_sign_bytes(data, size, &sig, (u32*)&sig_bytes, NULL);
	if (res)
	{
		wsprintfA(newPath, path);
		if (PathRenameExtensionA(newPath, PATH_SIG_EXT))
		{
			res = file_write(newPath, sig, sig_bytes);
		}
	}

	free(data);

	return res;
}
#endif