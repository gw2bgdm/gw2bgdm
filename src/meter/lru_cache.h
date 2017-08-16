#pragma once
#include <wchar.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Creates the context.
bool lru_create();

// Destroys the context.
void lru_destroy();

// Inserts an item / char
bool lru_insert(uint16_t type, uint64_t ptr, wchar_t* val);

// Finds a user by name.
const wchar_t * lru_find(uint16_t type, uint64_t ptr, wchar_t* outVal, size_t outLen);

// Delete a cachced item
void lru_delete(uint16_t type, uint64_t ptr);

// Resolve IDs
void lru_resolve();

#ifdef __cplusplus
}
#endif