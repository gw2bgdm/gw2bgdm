#include "meter/autolock.h"
#include "meter/config.h"
#include <Windows.h>	// ARRAYSIZE, lstrlenA
#include <uthash.h>


const AutoLockTarget g_raid_bosses[] = {
	{ 0x3C4E, 0x6F1B6,	L"Vale Guardian" },
	{ 0x3C45, 0x847A5,	L"Gorseval" },
	{ 0x3C0F, 0x653AE,	L"Sabetha" },
	{ 0x3EFB, 0x97981,	L"Slothasor" },
	{ 0x3ED8, 0x0,		L"Berg" },
	{ 0x3F09, 0x0,		L"Zane" },
	{ 0x3EFD, 0x0,		L"Narella" },
	{ 0x3EF3, 0x0,		L"Matthias" },
	{ 0x3F6B, 0x0,		L"Keep Construct" },
	{ 0x3F76, 0xA77AD,	L"Xera" },
	{ 0x3F9E, 0xADCE0,	L"Xera" },
	{ 0x432A, 0xB23B6,	L"Cairn" },
	{ 0x4314, 0xB1ED3,	L"Mursaat Overseer" },
	{ 0x4324, 0x0,		L"Samarog" },
	{ 0x4302, 0xB091A,	L"Deimos" },
};

struct autolock_entry {
	i32 key;
	bool value;
	UT_hash_handle hh;
};

static struct autolock_entry *g_autolock_raid = NULL;
static struct autolock_entry *g_autolock_custom = NULL;

#pragma warning( push )
#pragma warning( disable : 4127 )

void autolock_set_internal(struct autolock_entry **p_autolock, u32 id, bool autolock)
{
	if (id == 0) return;
	struct autolock_entry *entry = 0;
	HASH_FIND_INT(*p_autolock, &id, entry);
	if (!autolock && entry) {
		HASH_DEL(*p_autolock, entry);
		free(entry);
	}
	else if (autolock && !entry) {
		entry = malloc(sizeof(struct autolock_entry));
		if (entry) {
			entry->key = id;
			entry->value = 0;
			HASH_ADD_INT(*p_autolock, key, entry);
		}
	}
}

void autolock_set(u32 id, bool autolock)
{
	bool isRaid = false;
	for (int i = 0; i < ARRAYSIZE(g_raid_bosses); i++) {
		if (id == g_raid_bosses[i].npc_id) {
			isRaid = true;
			break;
		}
	}

	if (isRaid) autolock_set_internal(&g_autolock_raid, id, autolock);
	else autolock_set_internal(&g_autolock_custom, id, autolock);
}

bool autolock_get_internal(struct autolock_entry **p_autolock, u32 id)
{
	if (id == 0) return false;
	struct autolock_entry *entry = 0;
	HASH_FIND_INT(*p_autolock, &id, entry);
	if (entry)
		return true;
	return false;
}

bool autolock_get(u32 id)
{
	if (autolock_get_internal(&g_autolock_raid, id))
		return true;
	return autolock_get_internal(&g_autolock_custom, id);
}

void autolock_iter_internal(struct autolock_entry **p_autolock, autolock_cb cb)
{
	struct autolock_entry* cur, *tmp;
	HASH_ITER(hh, *p_autolock, cur, tmp)
	{
		cb(cur);
	}
}

void autolock_iter(autolock_cb cb, bool is_custom)
{
	autolock_iter_internal(is_custom ? &g_autolock_custom : &g_autolock_raid, cb);
}

#pragma warning( pop )

void autolock_init()
{
	char *autolock_raid = config_get_str(CONFIG_SECTION_GLOBAL, "AutoLockRaid", NULL);
	char *autolock_custom = config_get_str(CONFIG_SECTION_GLOBAL, "AutoLockCustom", NULL);

	for (int i = 0; i < 2; i++) {
		char *buf = (i == 0) ? autolock_raid : autolock_custom;
		if (!buf) continue;

		size_t len = lstrlenA(buf);
		if (len == 0) continue;

		i8* ctx = buf;
		for (i8* token = strtok_s(buf, "|", &ctx); token; token = strtok_s(NULL, "|", &ctx))
		{
			autolock_set_internal((i > 0) ? &g_autolock_custom : &g_autolock_raid, atoi(token), true);
		}
	}

	if (autolock_raid) free(autolock_raid);
	if (autolock_custom) free(autolock_custom);
}

void autolock_free_internal(struct autolock_entry **p_autolock)
{
	struct autolock_entry* cur, *tmp;
	HASH_ITER(hh, *p_autolock, cur, tmp)
	{
		HASH_DEL(*p_autolock, cur);
		free(cur);
	}
	*p_autolock = NULL;
}

void autolock_free()
{
	autolock_free_internal(&g_autolock_raid);
	autolock_free_internal(&g_autolock_custom);
}

void autolock_allraid()
{
	// Push all raid bosses
	for (int i = 0; i < ARRAYSIZE(g_raid_bosses); i++) {
		autolock_set_internal(&g_autolock_raid, g_raid_bosses[i].npc_id, true);
	}
}

void autolock_freeraid()
{
	autolock_free_internal(&g_autolock_raid);
}

void autolock_freecustom()
{
	autolock_free_internal(&g_autolock_custom);
}