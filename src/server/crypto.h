#pragma once
#include "core/types.h"

// Creates the cryptography interface.
bool crypto_create(void);

// Destroys the cryptography interface.
void crypto_destroy(void);

#if defined(_WIN32) || defined (_WIN64)
// Signs a file.
bool crypto_sign(i8 const* path);

// Get signature from bytes
bool crypto_sign_bytes(void const* data, u32 size, u8** dst, u32 *dst_bytes, i8 const* path);
#endif

// Get pubkey size & bytes
u8* crypto_pubkey_data();
u32 crypto_pubkey_bytes();