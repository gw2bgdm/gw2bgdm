#include <stdint.h>
#include <uthash.h>
#include <utarray.h>
#include <assert.h>
#if defined(_WIN32) || defined (_WIN64)
#include <intrin.h>
#include <Shlwapi.h>
#include <Strsafe.h>
#else
#include <math.h>
#include <pthread.h>
#include "core/strutil.h"
#endif
#include "core/types.h"
#include "core/debug.h"
#include "server/debug.h"
#include "server/network.h"
#include "server/context.h"
#include "server/lru_cache.h"

#if defined(_WIN32) || defined (_WIN64)
#pragma intrinsic(__movsb)
#pragma comment(lib, "shlwapi.lib")
#endif

// LRU cache in C using uthash
// http://uthash.sourceforge.net/


/* convenience forms of HASH_FIND/HASH_ADD/HASH_DEL */
#define HASH_FIND_INT64(head,findint,out)							\
HASH_FIND(hh,head,findint,sizeof(uint64_t),out)
#define HASH_ADD_INT64(head,intfield,add)							\
HASH_ADD(hh,head,intfield,sizeof(uint64_t),add)
#define HASH_REPLACE_INT64(head,intfield,add,replaced)				\
HASH_REPLACE(hh,head,intfield,sizeof(uint64_t),add,replaced)


#define HASH_FIND_WSTR(head,findstr,out)											\
HASH_FIND(hh,head,findstr,(unsigned)wcslen(findstr)*sizeof(wchar_t),out)
#define HASH_ADD_PWSTR(head,pwstr,add)												\
HASH_ADD_KEYPTR(hh, head, pwstr, wcslen(pwstr), add);
#define HASH_ADD_WSTR(head,strfield,add)											\
HASH_ADD(hh,head,strfield[0],(unsigned)wcslen(add->strfield)*sizeof(wchar_t),add)


#define MAX_CACHE_SIZE 2500

#if defined(_WIN32) || defined (_WIN64)
static const char TrimChars[] = { ' ', '\r', '\n', '\0' };
#define trim(x) StrTrimA(x, TrimChars)
#endif

struct CacheEntry;

struct HashEntry {
	char key[MSG_NAME_SIZE_UTF8];
	struct CacheEntry *value;
	UT_hash_handle hh;
};

struct ValueType {
	int idx;
	struct User user;
	struct HashEntry he;
};

struct CacheEntry {
	uint64_t key;
	struct ValueType value;
	UT_hash_handle hh;
};

typedef struct Lru {
#if defined(_WIN32) || defined (_WIN64)
	volatile LONG lock;
#else
	pthread_mutex_t lock;
#endif
	struct CacheEntry *cache;
	struct CacheEntry *store;
	struct HashEntry *lookup;
	UT_array *tracker;
	bool init;
} Lru;

static struct Lru g_lru;

#if defined(_WIN32) || defined (_WIN64)
#define ENTER_SPIN_LOCK(lock) while (InterlockedCompareExchange(&lock, 1, 0)) {}
#define EXIT_SPIN_LOCK(lock) InterlockedExchange(&lock, 0);
#else
#define ENTER_SPIN_LOCK(lock) pthread_mutex_lock(&lock);
#define EXIT_SPIN_LOCK(lock) pthread_mutex_unlock(&lock);
#endif

typedef struct {
	int type;
	uint64_t key;
} pair_t;

UT_icd pair_icd = { sizeof(pair_t), NULL, NULL, NULL };

struct CacheEntry* entry_alloc(Lru* lru)
{
	if (!lru || !lru->store || !lru->tracker)
		return NULL;

	int *pidx = (int*)utarray_back(lru->tracker);
	if (pidx) {
		uint32_t idx = *pidx;
		utarray_pop_back(lru->tracker);
		assert(idx < MAX_CACHE_SIZE);
		if (idx < MAX_CACHE_SIZE) {
			struct CacheEntry* entry = &lru->store[idx];
			assert(entry->value.idx == -1);
			entry->value.idx = idx;
			entry->value.he.value = entry;
			return entry;
		}
	}
	assert("entry_alloc() failed!");
	return NULL;
}

void entry_free(Lru* lru, struct CacheEntry* entry)
{
	if (!lru || !lru->store || !lru->tracker || !entry)
		return;

	assert(entry->value.idx >= 0 && entry->value.idx < MAX_CACHE_SIZE);

	utarray_push_back(lru->tracker, &entry->value.idx);
	memset(entry, 0, sizeof(*entry));
	entry->value.idx = -1;
}

// Creates the context.
struct Lru* lru_create()
{
	Lru* lru = &g_lru;

	// Allocate the mem tracker & unresolved array
	utarray_new(lru->tracker, &ut_int_icd);
	if (!lru->tracker)
	{
		return NULL;
	}

	// Allocate memory for the cache
	lru->store = calloc(MAX_CACHE_SIZE, sizeof(*lru->store));
	if (lru->store) {

		// "Free" all entries
		for (int i = MAX_CACHE_SIZE - 1; i >= 0; i--) {
			struct CacheEntry* entry = &lru->store[i];
			entry->value.idx = i;
			entry_free(lru, entry);
		}

		LOG_DEBUG("User cache created (capcaity=%d)", MAX_CACHE_SIZE);
		lru->init = true;
	}
	return lru;
}

// Destroys the context.
void lru_destroy(Lru* lru)
{
	if (!lru)
		return;

	struct CacheEntry* cur, *tmp;
	HASH_ITER(hh, lru->cache, cur, tmp)
	{
		HASH_DEL(lru->cache, cur);
		entry_free(lru, cur);
	}

	if (lru->store)
		free(lru->store);

	if (lru->tracker)
		utarray_free(lru->tracker);

	LOG_DEBUG("User cache destoryed");
	memset(lru, 0, sizeof(*lru));
}


static void cache_del(Lru* lru, uint64_t key, struct HashEntry* he)
{
	struct CacheEntry *entry = 0;
	ENTER_SPIN_LOCK(lru->lock);
	HASH_FIND_INT64(lru->cache, &key, entry);
	if (entry) {
		assert(key == entry->key);
		LOG_DEBUG("cache delete <0x%llx, [%s:%d] %s>",
			entry->key, entry->value.user.net_str, entry->value.user.net_port, entry->value.user.utf8_acctName);
		if (he == NULL && entry->value.he.key[0] != 0)
			HASH_FIND_STR(lru->lookup, entry->value.he.key, he);
		if (he && he->value == entry) {
			HASH_DEL(lru->lookup, he);
		}
		HASH_DEL(lru->cache, entry);
		entry_free(lru, entry);
	}
	EXIT_SPIN_LOCK(lru->lock);
}


static struct CacheEntry *cache_find(Lru* lru, uint64_t key)
{
	if (!lru)
		return NULL;

	struct CacheEntry *entry = 0;
	ENTER_SPIN_LOCK(lru->lock);
	HASH_FIND_INT64(lru->cache, &key, entry);
	if (entry) {
		assert(key == entry->key);
		// remove it (so the subsequent add will throw it on the front of the list)
		HASH_DEL(lru->cache, entry);
		HASH_ADD_INT64(lru->cache, key, entry);
	}
	EXIT_SPIN_LOCK(lru->lock);
	return entry;
}

static struct CacheEntry *cache_find_name(Lru* lru, const char* key)
{
	if (!lru)
		return NULL;

	struct HashEntry *he = 0;
	ENTER_SPIN_LOCK(lru->lock);
	HASH_FIND_STR(lru->lookup, key, he);
	if (he) {
		struct CacheEntry *entry = he->value;
		assert(he->value != NULL);
		assert(strcmp(key, he->key) == 0);
		// remove it (so the subsequent add will throw it on the front of the list)
		HASH_DEL(lru->cache, entry);
		HASH_ADD_INT64(lru->cache, key, entry);
	}
	EXIT_SPIN_LOCK(lru->lock);
	return he ? he->value : NULL;
}

static struct CacheEntry *cache_update_lookup(Lru *lru, uint64_t key, char* lookup)
{
	if (!lru || !lookup || lookup[0] == 0)
		return NULL;

	trim(lookup);

	struct CacheEntry *entry = cache_find(lru, key);
	if (!entry) {
		// should never get here since the key
		// was generated from a strcut User* which is part of our store
		assert(entry != NULL);
		return NULL;
	}

	struct HashEntry* tmpHe = NULL;
	struct HashEntry* he = &entry->value.he;

	ENTER_SPIN_LOCK(lru->lock);
	// First try to remove the old lookup key
	tmpHe = NULL;
	HASH_FIND_STR(lru->lookup, he->key, tmpHe);
	if (tmpHe && tmpHe->value == entry) {
		HASH_DEL(lru->lookup, tmpHe);
	}
	// Remove the new key if already exists (reconnect)
	tmpHe = NULL;
	HASH_FIND_STR(lru->lookup, lookup, tmpHe);
	if (tmpHe) {
		HASH_DEL(lru->lookup, tmpHe);
	}
	strncpy(he->key, lookup, ARRAYSIZE(he->key));
	HASH_ADD_STR(lru->lookup, key, he);
	EXIT_SPIN_LOCK(lru->lock);
	return entry;
}

static struct CacheEntry *cache_insert(Lru *lru, uint64_t key, User* value)
{
	if (!lru || !value)
		return NULL;

	struct CacheEntry *entry = cache_find(lru, key);

	if (!entry) {

		ENTER_SPIN_LOCK(lru->lock);

		// prune the cache to MAX_CACHE_SIZE
		if (HASH_COUNT(lru->cache) >= MAX_CACHE_SIZE) {
			// prune the first entry (loop is based on insertion order so this deletes the oldest item)
			// save entry beforehand as since we delete the first item g_cache will change to the next
			entry = lru->cache;
			LOG_DEBUG("CACHE_CAPACITY(%d) reached, evicting <0x%llx, [%s:%d] %s (%s)>",
				HASH_COUNT(lru->cache), entry->key, entry->value.user.net_str, entry->value.user.net_port,
				entry->value.user.utf8_name, entry->value.user.utf8_acctName);
			struct HashEntry* he = NULL;
			HASH_FIND_STR(lru->lookup, entry->value.he.key, he);
			if (he && he->value == entry) {
				LOG_DEBUG("LOOKUP_COUNT(%d), removing lookup <%s>",
					HASH_COUNT(lru->lookup), entry->value.he.key);
				HASH_DEL(lru->lookup, he);
			}
			HASH_DEL(lru->cache, entry);
			entry_free(lru, entry);
		}

		entry = entry_alloc(lru);
		if (entry) {
			entry->key = key;
			HASH_ADD_INT64(lru->cache, key, entry);
			assert(key == entry->key);
#if defined(_WIN32) || defined (_WIN64)
			__movsb((LPBYTE)&entry->value.user, (LPBYTE)value, sizeof(entry->value.user));
#else
			memcpy(&entry->value.user, value, sizeof(entry->value.user));
#endif
		}

		EXIT_SPIN_LOCK(lru->lock);

		LOG_INFO("[server] [#%d:%d] CONNECT from (%s:%d)",
			entry ? entry->value.idx : (-1), HASH_COUNT(lru->cache),
			value->net_str, value->net_port);
	}

	// entry_alloc() should always succeed
	assert(entry != NULL);
	if (!entry)
		return entry;

	if (entry->value.user.utf8_name[0] != 0) {
		cache_update_lookup(lru, key, entry->value.user.utf8_name);
	}

	return entry;
}


User* lru_insert_user(struct Lru* lru, uint32_t addr, uint32_t port, int64_t now)
{
#if defined(_WIN32) || defined (_WIN64)
	UNREFERENCED_PARAMETER(now);
#endif

	u64 key = (i64)addr << 32 | port;

	User user = { 0 };
	user.net_addr = addr;
	user.net_port = port;
	network_addr_to_str(addr, user.net_str);

	struct CacheEntry *entry = cache_insert(lru, key, &user);

	// Should never get here since the oldest
	// user should be evicted by cache_inesert
	assert(entry != NULL);
	if (!entry)
		return NULL;

	return &entry->value.user;
}

User* lru_find_user(Lru* lru, char* name, u32 shard_id)
{
#if defined(_WIN32) || defined (_WIN64)
	UNREFERENCED_PARAMETER(shard_id);
#endif

	if (!name)
		return NULL;

	trim(name);

	struct CacheEntry *entry = cache_find_name(lru, name);
	if (!entry)
		return NULL;

	//assert(shard_id == entry->value.user.shard_id);
	return &entry->value.user;
}

void lru_update_name(Lru* lru, User* user, char* name)
{
	if (!name || name[0] == 0)
		return;

	trim(name);

	if (strncmp(user->utf8_name, name, ARRAYSIZE(user->utf8_name)) == 0) {
		return;
	}

	memset(user->utf8_name, 0, sizeof(user->utf8_name));
	strncpy(user->utf8_name, name, ARRAYSIZE(user->utf8_name));

	u64 key = (i64)user->net_addr << 32 | user->net_port;
	struct CacheEntry *entry = cache_update_lookup(lru, key, user->utf8_name);
	assert(entry != NULL && strcmp(user->utf8_name, entry->value.he.key) == 0);

#if defined(_WIN32) || defined (_WIN64)
	UNREFERENCED_PARAMETER(entry);
#endif

	LOG(18, "[client:%s:%d] UPDATE NAME %s (%s) [LOOKUP_COUNT(%d)]",
		user->net_str, user->net_port, user->utf8_name,
		user->utf8_acctName, HASH_COUNT(lru->lookup));
}
