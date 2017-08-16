#pragma once
#include <stdint.h>
#include <wchar.h>


#ifdef __cplusplus
extern "C" {
#endif

	struct Lru;
	struct User;

	// Creates the context.
	struct Lru* lru_create();

	// Destroys the context.
	void lru_destroy(struct Lru* lru);

	// Inserts a user into the context. Returns the user on success.
	User* lru_insert_user(struct Lru* lru, uint32_t addr, uint32_t port, int64_t now);

	// Finds a user by name.
	User* lru_find_user(struct Lru* lru, char* name, uint32_t shard_id);

	// Updates a user's name.
	void lru_update_name(struct Lru* lru, User* user, char* name);

#ifdef __cplusplus
}
#endif