#include "core/types.h"
#include "core/debug.h"
#include "meter/offsets.h"
#include "meter/enums.h"
#include "meter/process.h"
#include "meter/ForeignFncs.h"
#include "meter/lru_cache.h"
#include <uthash.h>
#include <utarray.h>
#include <assert.h>
#include <intrin.h>

#pragma intrinsic(__movsb)

/* convenience forms of HASH_FIND/HASH_ADD/HASH_DEL */
#define HASH_FIND_INT64(head,findint,out)							\
    HASH_FIND(hh,head,findint,sizeof(uint64_t),out)
#define HASH_ADD_INT64(head,intfield,add)							\
    HASH_ADD(hh,head,intfield,sizeof(uint64_t),add)
#define HASH_REPLACE_INT64(head,intfield,add,replaced)				\
    HASH_REPLACE(hh,head,intfield,sizeof(uint64_t),add,replaced)

#define MAX_CACHE_SIZE 400

struct ValueType {
	int idx;
	wchar_t name[64];
};

struct CacheEntry {
	uint64_t key;
	struct ValueType value;
	UT_hash_handle hh;
};

static struct CacheEntry *g_cache = NULL;
struct CacheEntry *g_store = NULL;
static UT_array *g_tracker = NULL;
static UT_array *g_unresolved = NULL;
static UT_array *g_unresolvedHash = NULL;
static bool g_init = false;
static volatile LONG g_lock = 0;

#define ENTER_SPIN_LOCK(lock) while (InterlockedCompareExchange(&lock, 1, 0)) {}
#define EXIT_SPIN_LOCK(lock) InterlockedExchange(&lock, 0);

typedef struct {
	int type;
	uint64_t key;
} pair_t;

UT_icd pair_icd = { sizeof(pair_t), NULL, NULL, NULL };

struct CacheEntry* entry_alloc()
{
	if (!g_store || !g_tracker)
		return NULL;

	int *pidx = (int*)utarray_back(g_tracker);
	if (pidx) {
		uint32_t idx = *pidx;
		utarray_pop_back(g_tracker);
		assert(idx < MAX_CACHE_SIZE);
		if (idx < MAX_CACHE_SIZE) {
			struct CacheEntry* entry = &g_store[idx];
			assert(entry->value.idx == -1);
			entry->value.idx = idx;
			return entry;
		}
	}
	assert("entry_alloc() failed!");
	return NULL;
}

void entry_free(struct CacheEntry* entry)
{
	if (!g_store || !g_tracker || !entry)
		return;

	assert(entry->value.idx >= 0 && entry->value.idx < MAX_CACHE_SIZE);

	utarray_push_back(g_tracker, &entry->value.idx);
	memset(entry, 0, sizeof(*entry));
	entry->value.idx = -1;
}

// Creates the context.
bool lru_create()
{
	// Allocate the mem tracker & unresolved array
	utarray_new(g_tracker, &ut_int_icd);
	if (!g_tracker)
	{
		return false;
	}

	utarray_new(g_unresolved, &pair_icd);
	if (!g_unresolved)
	{
		utarray_free(g_tracker);
		return false;
	}

	// Allocate memory for the cache
	g_store = calloc(MAX_CACHE_SIZE, sizeof(*g_store));
	if (g_store) {

		// "Free" all entries
		for (int i = 0; i < MAX_CACHE_SIZE; i++) {
			struct CacheEntry* entry = &g_store[i];
			entry->value.idx = i;
			entry_free(entry);
		}

		DBGPRINT(TEXT("DecodedName cache created (capcaity=%d)"), MAX_CACHE_SIZE);
		g_init = true;
	}

	return g_init;
}

// Destroys the context.
void lru_destroy()
{

	struct CacheEntry* cur, *tmp;
	HASH_ITER(hh, g_cache, cur, tmp)
	{
		HASH_DEL(g_cache, cur);
		entry_free(cur);
	}

	if (g_store)
		free(g_store);

	if (g_tracker)
		utarray_free(g_tracker);

	if (g_unresolved)
		utarray_free(g_unresolved);

	DBGPRINT(TEXT("DecodedName cache destoryed"));
	g_init = false;
}

static void cache_del(uint64_t key)
{
	struct CacheEntry *entry = 0;
	ENTER_SPIN_LOCK(g_lock);
	HASH_FIND_INT64(g_cache, &key, entry);
	if (entry) {
		assert(key == entry->key);
		DBGPRINT(TEXT("cache delete <0x%llx, %s>"), entry->key, entry->value.name);
		HASH_DEL(g_cache, entry);
		entry_free(entry);
	}
	EXIT_SPIN_LOCK(g_lock);
}

struct CacheEntry *cache_find(uint64_t key)
{
	struct CacheEntry *entry = 0;
	ENTER_SPIN_LOCK(g_lock);
	HASH_FIND_INT64(g_cache, &key, entry);
	if (entry) {
		assert(key == entry->key);
		// remove it (so the subsequent add will throw it on the front of the list)
		HASH_DEL(g_cache, entry);
		HASH_ADD_INT64(g_cache, key, entry);
	}
	EXIT_SPIN_LOCK(g_lock);
	return entry;
}


static void cache_insert(uint64_t key, wchar_t *value)
{
	struct CacheEntry *entry = cache_find(key);

	if (!entry) {

		ENTER_SPIN_LOCK(g_lock);

		// prune the cache to MAX_CACHE_SIZE
		if (HASH_COUNT(g_cache) >= MAX_CACHE_SIZE) {
			// prune the first entry (list is based on insertion order so this deletes the oldest item)
			// save entry beforehand as since we delete the first item g_cache will change to the next
			entry = g_cache;
			DBGPRINT(TEXT("CACHE_CAPACITY(%d) reached, evicting <0x%llx, %s>"),
				HASH_COUNT(g_cache), entry->key, entry->value.name);
			HASH_DEL(g_cache, entry);
			entry_free(entry);
		}

		entry = entry_alloc();
		if (entry) {
			entry->key = key;
			HASH_ADD_INT64(g_cache, key, entry);
		}

		EXIT_SPIN_LOCK(g_lock);
	}

	// entry_alloc() should always succeed
	assert(entry != NULL);

	if (!entry)
		return;

	assert(key == entry->key);
	wcsncpy(entry->value.name, value, ARRAYSIZE(entry->value.name));
}

static uint64_t KeyFromPtr(uint16_t type, uint64_t ptr)
{
	if (type == CLASS_TYPE_CHAR ||
		type == CLASS_TYPE_AGENT ||
		type == CLASS_TYPE_SPECIESID || 
		type == CLASS_TYPE_SKILLID)
		return ptr;

	return HashIdFromPtr(type, ptr);
}

const wchar_t * lru_find(uint16_t type, uint64_t ptr, wchar_t* outVal, size_t outLen)
{
	if (!g_init || !ptr)
		return NULL;

	uint64_t key = KeyFromPtr(type, ptr);
	if (!key)
		return NULL;

	const struct CacheEntry *entry = cache_find(key);
	if (!entry) {

		bool bFound = false;
		for (pair_t *p = (pair_t*)utarray_front(g_unresolved);
			p != NULL;
			p = (pair_t*)utarray_next(g_unresolved, p))
		{
			if (p->key == key)
				bFound = true;
		}

		pair_t p = { 0 };
		p.key = key;
		p.type = type;
		if (!bFound)
			utarray_push_back(g_unresolved, &p);

		return NULL;
	}

	if (entry->value.name[0] != 0) {
		if (outVal && outLen) {
			SIZE_T copyBytes = min(outLen * sizeof(wchar_t), sizeof(entry->value.name));
			__movsb((u8*)outVal, (u8*)entry->value.name, copyBytes);
			outVal[outLen - 1] = 0;
		}
		return entry->value.name;
	}

	return NULL;
}

void lru_delete(uint16_t type, uint64_t ptr)
{
	if (!g_init || !ptr)
		return;

	uint64_t key = KeyFromPtr(type, ptr);
	if (!key)
		return;

	cache_del(key);
}


static void __fastcall cbDecodeText(uint8_t* ctx, wchar_t* decodedText)
{
	DBGPRINT(TEXT("[ctx=%p] [text=%s]"), ctx, decodedText);
	if (ctx && decodedText && decodedText[0] != 0) {
		cache_insert((uint64_t)ctx, decodedText);
	}
}

void lru_resolve()
{
	if (!g_init)
		return;

	for (pair_t *p = (pair_t*)utarray_front(g_unresolved);
		p != NULL;
		p = (pair_t*)utarray_next(g_unresolved, p))
	{
		DBGPRINT(TEXT("Resolving [type=%d] [key=%p]"), p->type, p->key);

		if (p->type == CLASS_TYPE_AGENT) {

			uint64_t aptr = p->key;
			WmAgent_GetCodedName(aptr, cbDecodeText, aptr);

		} else if (p->type == CLASS_TYPE_CHAR) {

			// NOT NEEDED ANYORE
			// WE ALWAYS GET THE NAME FROM THE AGENT
			assert("DON'T GET HERE");

		} else {
		
			uint32_t hashId = (uint32_t)p->key;
			uint8_t* codedText = CodedTextFromHashId(hashId, 0);
			if (codedText) {
				DecodeText(codedText, cbDecodeText, hashId);
			}
		}
	}
	utarray_clear(g_unresolved);
}