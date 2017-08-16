#include "crypto.h"
#include "core/file.h"
#include "core/debug.h"
#include "meter/key.h"
#include <Windows.h>
#include <wincrypt.h>
#include <stdlib.h>

static HCRYPTPROV g_prov;
static HCRYPTKEY g_key_pub;
#define PATH_KEY_PUB ".\\bin64\\bgdm.pub"
#define CRYPT_CONTAINER TEXT("gw2bgdm")

bool crypto_create(void)
{
	if (!CryptAcquireContext(&g_prov, CRYPT_CONTAINER, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, 0/*CRYPT_MACHINE_KEYSET*/))
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
			if (!CryptAcquireContext(&g_prov, CRYPT_CONTAINER, MS_ENH_RSA_AES_PROV, PROV_RSA_AES, CRYPT_NEWKEYSET /*| CRYPT_MACHINE_KEYSET*/))
			{
				DBGPRINT(TEXT("CryptAcquireContext failed, error 0x%x"), GetLastError());
				return false;
			}
		}
		else
		{
			DBGPRINT(TEXT("CryptAcquireContext failed, error 0x%x"), GetLastError());
			return false;
		}
	}

	/*u32 size;
	u8* data = file_read(PATH_KEY_PUB, &size);
	if (data)
	{
		//CryptImportKey(g_prov, g_key_bc, sizeof(g_key_bc), 0, 0, &g_key_pub);
		CryptImportKey(g_prov, data, size, 0, 0, &g_key_pub);
		free(data);
	}
	else
	{
		DBGPRINT(TEXT("crypto error: public key %S not found, live updates disalbed"), PATH_KEY_PUB);
		crypto_destroy();
		return;
	}*/

	DBGPRINT(TEXT("crypto successfully initialized"));
	return true;
}

void crypto_destroy(void)
{
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
}

bool crypto_set_pubkey(void const* key_data, u32 key_bytes)
{
	if (!key_data || key_bytes == 0)
		return false;

	if (g_key_pub)
		return true;

	bool res = CryptImportKey(g_prov, key_data, key_bytes, 0, 0, &g_key_pub);
	if (!res)
	{
		DBGPRINT(TEXT("CryptImportKey failed, error 0x%x"), GetLastError());
	}

	return res;
}


bool crypto_initialized(void)
{
	if (g_prov)
		return true;
	return false;
}

void crypto_random(void* dst, u32 dst_bytes)
{
	if (g_prov == 0 || CryptGenRandom(g_prov, dst_bytes, dst) == FALSE)
	{
		memset(dst, 0, dst_bytes);
	}
}

bool crypto_verify(void const* sig, u32 sig_bytes, void const* src, u32 src_bytes)
{
	if (g_key_pub == 0)
	{
		return false;
	}

	bool res = false;
	HCRYPTHASH hash;
	if (CryptCreateHash(g_prov, CALG_SHA_256, 0, 0, &hash))
	{
		if (CryptHashData(hash, src, src_bytes, 0))
		{
			if (CryptVerifySignatureA(hash, sig, (DWORD)sig_bytes, g_key_pub, 0, 0))
			{
				DBGPRINT(TEXT("CryptVerifySignature success"));
				res = true;
			}
			else
			{
				// NTE_BAD_SIGNATURE
				DBGPRINT(TEXT("CryptVerifySignature failed, error 0x%x"), GetLastError());
			}
		}

		CryptDestroyHash(hash);
	}

	return res;
}
