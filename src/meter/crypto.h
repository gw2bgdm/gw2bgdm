#pragma once
#include "core/types.h"

// Creates the crypto interface.
bool crypto_create(void);

// Destroys the crypto interface.
void crypto_destroy(void);

// Checks if crypto was properly initialized
bool crypto_initialized(void);

// Set the provider pub key
bool crypto_set_pubkey(void const* key_data, u32 key_bytes);

// Returns a random sequence into the given buffer.
void crypto_random(void* dst, u32 dst_bytes);

// Verifies a cryptographic signature.
bool crypto_verify(void const* sig, u32 sig_bytes, void const* src, u32 src_bytes);
