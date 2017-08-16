#pragma once
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Reads memory in a process
#define PROCESS_READ(b, x) process_read(b, x, sizeof(*(x)))

// Type generic process memory read function creator.
#define PROCESS_READ_T(t) __forceinline t process_read_##t(u64 base) { t x; PROCESS_READ(base, &x); return x; } 

// Creates the process interface.
void process_create(i8 const* wnd, i8 const* cls);

// Scans and follows a relative address jump.
u64 process_follow(i8 const* pattern, i8 const* mask, i32 off);

// Returns the process handle.
void *process_get_handle(void);

// Returns the process id.
u32 process_get_id(void);

// Returns the process window.
void *process_get_window(void);

// Reads memory from a process.
bool process_read(u64 base, void *dst, u32 bytes);

// Scans for a pattern in a process.
u64 process_scan(void const* sig, i8 const* msk);

// generate relative address
u64 process_follow_rel_addr(u64 adr);

// Type generic process memory reading functions.
PROCESS_READ_T(i8);
PROCESS_READ_T(i16);
PROCESS_READ_T(i32);
PROCESS_READ_T(i64);
PROCESS_READ_T(u8);
PROCESS_READ_T(u16);
PROCESS_READ_T(u32);
PROCESS_READ_T(u64);
PROCESS_READ_T(f32);
PROCESS_READ_T(f64);
PROCESS_READ_T(vec2);
PROCESS_READ_T(vec3);
PROCESS_READ_T(vec4);


#ifdef __cplusplus
}
#endif
