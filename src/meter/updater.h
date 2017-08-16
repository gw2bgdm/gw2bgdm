#pragma once
#include "core/types.h"
#include "core/message.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Creates the updater.
bool updater_create(i8 const* path, i32 version);

// Destroys the updater.
void updater_destroy(void);

// Returns the current version.
u32 updater_get_cur_version(void);

// Returns last known server version
u32 updater_get_srv_version(void);

// Returns the current version string
i8 const *updater_get_version_str(void);

// Returns the last timestamp of srv connection to updater
time_t updater_get_srv_time(void);

// Returns true if the updater is updating.
bool updater_is_updating(u32* offset, u32* size);

// Handles an update message.
void updater_handle_msg_begin(MsgServerUpdateBegin* msg);

// Handles an update message.
void updater_handle_msg_piece(MsgServerUpdatePiece* msg);

// Updates the update subsystem state. Returns true if the application needs a restart.
bool updater_update(i64 now);

#ifdef __cplusplus
}
#endif