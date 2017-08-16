#include "server/context.h"
#include "server/debug.h"
#include "server/lru_cache.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined (_WIN64)
#include "meter/utf.h"
#endif

// Maximum number of users to support at once.
#define MAX_USERS 100000

// Maximum age of a user in the table, in microseconds.
#define MAX_AGE 30000000

bool context_create(Context* ctx)
{
	ctx->lru = lru_create();
	return (ctx->lru != 0);
}

void context_destroy(Context* ctx)
{
	lru_destroy(ctx->lru);
	return;
}


User* context_find_user(Context* ctx, i8* name, u32 shard_id)
{
	return lru_find_user(ctx->lru, name, shard_id);
}

User* context_insert_user(Context* ctx, u32 addr, u32 port, i64 now)
{
	return lru_insert_user(ctx->lru, addr, port, now);
}

void context_update_name(Context* ctx, User* user, i8* name)
{
	lru_update_name(ctx->lru, user, name);
}


void context_update_account(Context* ctx, User* user, i8* acctName, i8* charName)
{
	if (!acctName || acctName[0] == 0)
		return;

	if (user->utf8_name[0] == 0 && user->utf8_charName[0]) {
		strncpy(user->utf8_name, user->utf8_charName, ARRAYSIZE(user->utf8_name));
		user->utf8_name[ARRAYSIZE(user->utf8_name) - 1] = 0;
	}

	if (user->utf8_acctName[0] &&
		user->utf8_charName[0] &&
		strncmp(user->utf8_charName, charName, ARRAYSIZE(user->utf8_charName)) == 0)
	{
		return;
	}

	strncpy(user->utf8_acctName, acctName, ARRAYSIZE(user->utf8_acctName));
	strncpy(user->utf8_charName, charName, ARRAYSIZE(user->utf8_charName));

	LOG(18, "[client:%d.%d.%d.%d:%d] UPDATE ACCT %s (%s) <ver=%s>",
		(user->net_addr & 0xFF),
		(user->net_addr >> 8) & 0xFF,
		(user->net_addr >> 16) & 0xFF,
		(user->net_addr >> 24) & 0xFF,
		user->net_port,
		user->utf8_charName,
		user->utf8_acctName,
		user->ver_str
	);

}
