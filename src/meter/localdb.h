#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Local name resolving functions
const char *stat_name_from_id(u32 id);
const char *wep_name_from_id(u32 id);
const char *rune_name_from_id(u32 id);
const char *sigil_name_from_id(u32 id);
const char *jewel_name_from_id(u32 id);
const char *infusion_name_from_id(u32 id, u32 *ar);
const char *spec_name_from_id(u32 spec);
const char *trait_name_from_id(u32 spec, u32 id);

#ifdef __cplusplus
}
#endif