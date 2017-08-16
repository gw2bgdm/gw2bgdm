#pragma once
#include <stdint.h>
#include <wchar.h>
#include "core/message.h"

typedef struct AutoLockTarget
{
	uint32_t		npc_id;
	uint32_t		species_id;
	wchar_t			name[MSG_NAME_SIZE * 2];
} AutoLockTarget;

#ifdef __cplusplus
extern "C" {
#endif

extern const AutoLockTarget g_raid_bosses[];

void autolock_init();
void autolock_free();
void autolock_allraid();
void autolock_freeraid();
void autolock_freecustom();
bool autolock_get(u32 id);
void autolock_set(u32 id, bool autolock);
typedef void(*autolock_cb)(void *data);
void autolock_iter(autolock_cb cb, bool is_custom);

#ifdef __cplusplus
}
#endif