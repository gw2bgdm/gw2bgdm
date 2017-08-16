#pragma once
#include "core/message.h"

// A user entry.
typedef struct User
{
	struct User* next_addr;
	struct User* next_name;
	struct User* prev_addr;
	struct User* prev_name;
	i64 last_msg_damage;
	i64 last_msg_update_begin;
	i64 last_msg_update_piece;
	u64 last_msg_update_seq;
	u32 msg_update_max_size;
	u32 msg_update_retry;
	i64 user_time;
	u32 slot_addr;
	u32 slot_name;
	u32 net_addr;
	u32 net_port;
	u32 shard_id;
	u32 profession;
	bool in_combat;
	f32 time;
	u32 damage;
	u32 damage_in;
	u32 heal_out;
	ClientStats stats;
	ClientTarget targets[MSG_TARGETS];
	i8 utf8_name[MSG_NAME_SIZE_UTF8];
	i8 utf8_acctName[MSG_NAME_SIZE_UTF8 * 2];
	i8 utf8_charName[MSG_NAME_SIZE_UTF8 * 2];
	i8 net_str[32]; // INET_ADDRSTRLEN + room for port
	i8 ver_str[32];
} User;

// User context.
struct Lru;

typedef struct Context
{
	User** head_addr;
	User** head_name;
	User* users;
	u32 num_users;
	struct Lru* lru;
} Context;

// Creates the context.
bool context_create(Context* ctx);

// Destroys the context.
void context_destroy(Context* ctx);

// Inserts a user into the context. Returns the user on success.
User* context_insert_user(Context* ctx, u32 addr, u32 port, i64 now);

// Finds a user by name.
User* context_find_user(Context* ctx, i8* name, u32 shard_id);

// Updates a user's name.
void context_update_name(Context* ctx, User* user, i8* name);

// Update a user's account name.
void context_update_account(Context* ctx, User* user, i8* acctName, i8* charName);
