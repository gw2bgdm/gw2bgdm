#include "meter/ForeignFncs.h"
#include "meter/offsets.h"
#include "meter/enums.h"
#include "meter/game.h"
#include "meter/lru_cache.h"
#include "hacklib/ForeignClass.h"

#include <cstdint>
#include <type_traits> // std::conditional


#ifdef __cplusplus
extern "C" {
#endif

#include "meter/process.h"
#include "meter/dps.h"

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "core/debug.h"

#pragma warning( push )
#pragma warning( disable: 4189 )

DWORD ExceptHandler(const char *msg, DWORD code, EXCEPTION_POINTERS *ep, const char *file, const char *func, int line) {
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	CONTEXT *ctx = ep->ContextRecord;
	LPCTSTR fmt_dbg = TEXT("%S: 0x%08X - addr: 0x%p");
	LPCTSTR fmt_rel = TEXT("%S");

#ifdef _DEBUG
	DBGPRINT(fmt_dbg, msg, code, er->ExceptionAddress);
#else
	DBGPRINT(fmt_rel, msg);
#endif

	return EXCEPTION_EXECUTE_HANDLER;
}

#pragma warning( pop )


uint8_t* CodedTextFromHashId(uint32_t hashId, uint32_t a2)
{
	//static uintptr_t pfcCodedNameFromHashId = 0;

	typedef uint8_t*(__fastcall *fCodedTextFromHashId)(uint32_t hashId, uint32_t a2);

	__try {

		/*if (!pfcCodedNameFromHashId) {

			// Search for:
			// 7FF6BD028450:"!buffer->Count() || (CParser::Validate(buffer->Ptr(), buffer->Term(), true ) == buffer->Term())"
			// The function above it will have:
			// "CParser::Validate(buffer->Ptr(), buffer->Term(), true ) == buffer->Term()"
			// 53 57 48 83 EC 48 8B D9 E8 ?? ?? ?? ?? 48 8B 48 50 E8 ?? ?? ?? ?? 44 8B 4C 24 68 48 8D 4C 24 30 48 8B F8
			pfcCodedNameFromHashId = process_scan(
				"\x53\x57\x48\x83\xEC\x48\x8B\xD9\xE8\x00\x00\x00\x00\x48\x8B\x48\x50\xE8\x00\x00\x00\x00\x44\x8B\x4C\x24\x68\x48\x8D\x4C\x24\x30\x48\x8B\xF8",
				"xxxxxxxxx????xxxxx????xxxxxxxxxxxxx"
			);

			if (!pfcCodedNameFromHashId)
				// Invalid ptr
				pfcCodedNameFromHashId = (uintptr_t)(-1);
			else
				// func offset since we search after the RSP setup
				pfcCodedNameFromHashId -= 0xE;

			DBGPRINT(TEXT("[pfcCodedNameFromHashId=%p]"), pfcCodedNameFromHashId);
		}*/

		if (!g_ptrs.pfcCodedTextFromHashId || !hashId)
			return nullptr;

		//if (pfcCodedNameFromHashId != (uintptr_t)(-1))
		//	return ((fCodedTextFromHashId)pfcCodedNameFromHashId)(hashId, a2);

		return ((fCodedTextFromHashId)g_ptrs.pfcCodedTextFromHashId)(hashId, a2);
	}
	__except (BGDM_EXCEPTION("[CodedTextFromHashId] access violation")) {
		;
	}
	return nullptr;
}

bool DecodeText(uint8_t* codedText, cbDecodeText_t cbDecodeText, uintptr_t ctx)
{

	bool bRet = false;
	//static uintptr_t pfcDecodeText = 0;

	typedef void(__fastcall *fDecodeText)(uint8_t*, cbDecodeText_t, uintptr_t);

	__try {

		/*if (!pfcDecodeText) {

			// Search for "resultFunc"
			// 49 8B E8 48 8B F2 48 8B F9 48 85 C9 75 19 48 8D 15 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 41 B8 65 05 00 00 E8
			pfcDecodeText = process_scan(
				"\x49\x8B\xE8\x48\x8B\xF2\x48\x8B\xF9\x48\x85\xC9\x75\x19\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x41\xB8\x65\x05\x00\x00\xE8",
				"xxxxxxxxxxxxxxxxx????xxx????xxxxxxx"
			);

			if (!pfcDecodeText)
				// Invalid ptr
				pfcDecodeText = (uintptr_t)(-1);
			else
				// func offset since we search after the RSP setup
				pfcDecodeText -= 0x14;

			DBGPRINT(TEXT("[pfcDecodeText=%p]"), pfcDecodeText);
		}*/

		if (!g_ptrs.pfcDecodeCodedText || !codedText || !cbDecodeText)
			return bRet;

		/*if (pfcDecodeText != (uintptr_t)(-1)) {
			//DBGPRINT(TEXT("[codedText=%p] [ctx=%p]"), codedText, ctx);
			((fDecodeText)pfcDecodeText)(codedText, cbDecodeText, ctx);
			bRet = true;
		}*/

		((fDecodeText)g_ptrs.pfcDecodeCodedText)(codedText, cbDecodeText, ctx);
		bRet = true;
	}
	__except (BGDM_EXCEPTION("[DecodeText] access violation")) {
		;
	}
	return bRet;
}

uint32_t HashIdFromPtr(int type, uintptr_t ptr)
{
	uint32_t hashId = 0;
	switch (type) {
	case (GW2::CLASS_TYPE_ITEM):
		hashId = process_read_u32(ptr + OFF_ITEM_HASHID);
		break;
	case (GW2::CLASS_TYPE_UPGRADE):
		hashId = process_read_u32(ptr + OFF_ITEM_HASHID);
		//hashId = process_read_u32(ptr + OFF_ITEM_UPG_HASHID);
		break;
	case (GW2::CLASS_TYPE_SPEC):
		hashId = process_read_u32(ptr + OFF_SPEC_HASHID);
		break;
	case (GW2::CLASS_TYPE_TRAIT):
		hashId = process_read_u32(ptr + OFF_TRAIT_HASHID);
		break;
	case (GW2::CLASS_TYPE_SKILL):
		hashId = process_read_u32(ptr + OFF_SKILLDEF_HASHID);
		break;
	case (GW2::CLASS_TYPE_SKIN):
		hashId = process_read_u32(ptr + OFF_SKIN_HASHID);
		break;
	case (GW2::CLASS_TYPE_STAT):
		hashId = process_read_u32(ptr + OFF_STAT_HASHID);
		break;
	case (GW2::CLASS_TYPE_ITEMTYPE):
		hashId = process_read_u32(ptr + OFF_ITEM_TYPE_HASHID);
		break;
	case (GW2::CLASS_TYPE_SPECIESDEF):
		hashId = process_read_u32(ptr + OFF_SPECIES_DEF_HASHID);
		break;
	};
	return hashId;
}

bool WV_GetMetrics(uintptr_t wvctxptr, int one, D3DXVECTOR3* camPos, D3DXVECTOR3* lookAt, D3DXVECTOR3* upVec, float* fovy)
{
	bool bRet = false;
	if (!wvctxptr)
		return bRet;

	__try {

		// TODO: Crashes out of game main thread
		hl::ForeignClass wvctx = (void*)wvctxptr;
		if (wvctx && wvctx.get<int>(OFF_WVCTX_STATUS) == 1) {
			wvctx.call<void>(OFF_WVCTX_VT_GETMETRICS, one, camPos, lookAt, upVec, fovy);
			bRet = true;
		}
	}
	__except (BGDM_EXCEPTION("[WV_GetMetrics] access violation")) {
		;
	}
	return bRet;
}

bool Ag_GetPos(uintptr_t aptr, D3DXVECTOR4* outPos)
{
	bool bRet = false;
	if (!aptr)
		return bRet;

	__try {
		hl::ForeignClass agent = (void*)aptr;
		if (agent) {
			agent.call<void>(OFF_AGENT_VT_GETPOS, outPos);
			bRet = true;
		}
	}
	__except (BGDM_EXCEPTION("[Ag_GetPos] access violation")) {
		;
	}
	return bRet;
}

uintptr_t Ag_GetWmAgemt(uintptr_t aptr)
{
	uintptr_t ret = 0;
	typedef uintptr_t(__fastcall*fpGetWmAgent)(uintptr_t);

	__try {

		if (!aptr || !g_ptrs.pfcGetWmAgent)
			return ret;

		ret = ((fpGetWmAgent)g_ptrs.pfcGetWmAgent)(aptr);

	}
	__except (BGDM_EXCEPTION("[Ag_GetWmAgemt] access violation")) {
		;
	}
	return ret;
}

bool WmAgent_GetCodedName(uintptr_t aptr, cbDecodeText_t cbDecodeText, uintptr_t ctx)
{
	bool bRet = false;

	uintptr_t wmptr = Ag_GetWmAgemt(aptr);
	if (!wmptr) return false;

	__try {

		hl::ForeignClass wmAgent = (void*)wmptr;

		if (!wmAgent || !cbDecodeText || !ctx)
			return bRet;

		uint8_t* codedName = (uint8_t*)wmAgent.call<void*>(OFF_WMAGENT_VT_GET_CODED_NAME);
		if (codedName) {
			bRet = DecodeText(codedName, cbDecodeText, ctx);
		}
	}
	__except (BGDM_EXCEPTION("[WmAgent_GetCodedName] access violation")) {
		;
	}
	return bRet;
}

uintptr_t Ch_GetCombatant(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c) return 0;

		return c.call<uintptr_t>(OFF_CHAR_VT_GET_CMBTNT);
	}
	__except (BGDM_EXCEPTION("[Ch_GetCombatant] access violation")) {
		;
	}
	return 0;
}

bool Ch_GetSpeciesDef(uintptr_t cptr, uintptr_t *pSpeciesDef)
{
	bool bRet = false;

	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c || !pSpeciesDef)
			return bRet;

		c.call<void*>(OFF_CHAR_VT_GET_SPECIES_DEF, pSpeciesDef);
		if (*pSpeciesDef)
			bRet = true;
	}
	__except (BGDM_EXCEPTION("[Ch_GetSpeciesDef] access violation")) {
		;
	}
	return bRet;
}

int  Ch_GetProfession(uintptr_t cptr)
{
	int iRet = 0;
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return iRet;

		hl::ForeignClass prof = (void*)c.get<void*>(OFF_CHAR_PROFESSION);
		if (!prof)
			return iRet;

		iRet = prof.call<int>(0x0);
	}
	__except (BGDM_EXCEPTION("[Ch_GetProfession] access violation")) {
		;
	}

	return iRet;
}

uintptr_t Ag_GetChar(uintptr_t aptr, struct Array* ca)
{
	__try {

		hl::ForeignClass a = (void*)aptr;
		if (!a || !ca)
			return 0;

		uint32_t id = a.get<uint32_t>(OFF_AGENT_ID);
		if (id > ca->max)
			return 0;

		uint64_t cptr = process_read_u64(ca->data + id * 8);
		return cptr;
	}
	__except (BGDM_EXCEPTION("[Ag_GetChar] access violation")) {
		;
	}
	return false;
}

bool Ag_Validate(uint32_t id, uintptr_t aptr, uintptr_t wmptr)
{
	__try {

		hl::ForeignClass wmAgent = (void*)wmptr;
		if (!wmAgent)
			return false;

		hl::ForeignClass a = wmAgent.call<void*>(OFF_WMAGENT_VT_GET_AGENT);
		if (a && (uintptr_t)a.data() == aptr)
			return true;

		DBGPRINT(TEXT("Agent <id=%d> <aptr %p> <wmptr %p> no longer valid"), id, aptr, wmptr);
	}
	__except (BGDM_EXCEPTION("[Ag_Validate] access violation")) {
		;
	}
	return false;
}

bool Ch_Validate(uintptr_t cptr, struct Array* ca)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c || !ca)
			return false;

		hl::ForeignClass a = (void*)c.get<void*>(OFF_CHAR_AGENT);
		if (!a)
			return false;

		uint32_t id = a.get<uint32_t>(OFF_AGENT_ID);
		if (id > ca->max)
			return false;

		uint64_t parr = process_read_u64(ca->data + id * 8);
		if (cptr == parr) {
			//DBGPRINT(TEXT("<cptr %p> <id=%d> is valid"), cptr, id);
			return true;
		}

		DBGPRINT(TEXT("<aptr %p> <cptr %p> <id=%d> no longer valid"), a.data(), cptr, id);
	}
	__except (BGDM_EXCEPTION("[Ch_Validate] access violation")) {
		;
	}
	return false;
}

uint32_t Ch_GetPlayerId(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return 0;

		return c.call<uint32_t>(OFF_CHAR_VT_GET_PLAYER_ID);
	}
	__except (BGDM_EXCEPTION("[Ch_GetPlayerId] access violation")) {
		;
	}
	return 0;
}

bool Ch_IsPlayer(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return false;

		bool isPlayer = c.call<bool>(OFF_CHAR_VT_IS_PLAYER);
		return isPlayer;
	}
	__except (BGDM_EXCEPTION("[Ch_IsPlayer] access violation")) {
		;
	}
	return false;
}

bool Ch_IsAlive(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return false;

		hl::ForeignClass subClass = c + OFF_CHAR_SUBCLASS;
		if (!subClass)
			return false;

		bool isAlive = subClass.call<bool>(OFF_CHAR_VT_IS_ALIVE);
		//if (!isAlive) DBGPRINT(TEXT("<cptr %p> is %s!"), cptr, isAlive ? L"alive" : L"dead");
		return isAlive;
	}
	__except (BGDM_EXCEPTION("[Ch_IsAlive] access violation")) {
		;
	}
	return false;
}

bool Ch_IsDowned(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return false;

		bool isDowned = c.call<bool>(OFF_CHAR_VT_IS_DOWNED);
		//DBGPRINT(TEXT("<cptr %p> is %s!"), cptr, isDowned ? L"downed" : L" not downed");
		return isDowned;
	}
	__except (BGDM_EXCEPTION("[Ch_IsDowned] access violation")) {
		;
	}
	return false;
}

bool Ch_IsClone(uintptr_t cptr)
{
	__try {

		hl::ForeignClass c = (void*)cptr;
		if (!c)
			return false;

		bool isClone = c.call<bool>(OFF_CHAR_VT_IS_CLONE);
		return isClone;
	}
	__except (BGDM_EXCEPTION("[Ch_IsClone] access violation")) {
		;
	}
	return false;
}

bool Pl_HasEliteSpec(uintptr_t pptr) {

	if (!pptr) return false;

	__try {

		hl::ForeignClass player = (void*)pptr;
		if (player)
		{
			hl::ForeignClass specMgr = player.call<void*>(OFF_PLAYER_VT_GET_SPECMGR);
			if (specMgr) {

				hl::ForeignClass spec3 = specMgr.get<void*>(OFF_SPECMGR_SPECS + (GW2::SPEC_SLOT_3 * sizeof(void*)));
				if (spec3) {

					GW2::Specialization specType = spec3.get<GW2::Specialization>(OFF_SPEC_TYPE);
					switch (specType) {
					case GW2::SPEC_GUARD_DRAGONHUNTER:
					case GW2::SPEC_MES_CHRONOMANCER:
					case GW2::SPEC_ELE_TEMPEST:
					case GW2::SPEC_ENGI_SCRAPPER:
					case GW2::SPEC_THIEF_DAREDEVIL:
					case GW2::SPEC_NECRO_REAPER:
					case GW2::SPEC_RANGER_DRUID:
					case GW2::SPEC_WAR_BERSERKER:
					case GW2::SPEC_REV_HERALD:
						return true;
					}
				}
			}
		}
	}
	__except (BGDM_EXCEPTION("[Pl_HasEliteSpec] access violation")) {
		;
	}
	return false;
}

bool Pl_GetSpec(uintptr_t pptr, Spec *outSpec)
{
	bool bRet = false;

	__try {

		if (!pptr || !outSpec)
			return bRet;

		memset(outSpec, 0, sizeof(Spec));

		hl::ForeignClass player = (void*)pptr;
		if (player)
		{
			hl::ForeignClass specMgr = player.call<void*>(OFF_PLAYER_VT_GET_SPECMGR);
			if (specMgr) {

				for (size_t i = 0; i < GW2::SPEC_SLOT_END; i++) {
					hl::ForeignClass spec = specMgr.get<void*>(OFF_SPECMGR_SPECS + (i * sizeof(void*)));
					if (spec) {
						outSpec->specs[i].ptr = (u64)spec.data();
						outSpec->specs[i].name = lru_find(GW2::CLASS_TYPE_SPEC, outSpec->specs[i].ptr, NULL, 0);
						GW2::Specialization specType = spec.get<GW2::Specialization>(OFF_SPEC_TYPE);
						if (specType < GW2::SPEC_NONE || specType > GW2::SPEC_END) {
							outSpec->specs[i].id = GW2::SPEC_NONE;
						}
						else {
							outSpec->specs[i].id = specType;
						}
					}

					for (size_t j = 0; j < GW2::TRAIT_SLOT_END; j++) {
						hl::ForeignClass trait = specMgr.get<void*>(OFF_SPECMGR_TRAITS + (i * GW2::TRAIT_SLOT_END + j) * sizeof(void*));
						if (trait) {
							outSpec->traits[i][j].ptr = (u64)trait.data();
							outSpec->traits[i][j].name = lru_find(GW2::CLASS_TYPE_TRAIT, outSpec->traits[i][j].ptr, NULL, 0);
							GW2::Trait traitType = trait.get<GW2::Trait>(OFF_TRAIT_TYPE);
							if (traitType < GW2::TRAIT_NONE || traitType >= GW2::TRAIT_END) {
								outSpec->traits[i][j].id = GW2::TRAIT_NONE;
							}
							else {
								outSpec->traits[i][j].id = traitType;
							}
						}
					}
				}

				bRet = true;
			}
		}
	}
	__except (BGDM_EXCEPTION("[Pl_GetSpec] access violation")) {
		;
	}
	return bRet;
}



static inline bool WeaponIs2H(uint32_t type)
{
	switch (type)
	{
	case (GW2::WEP_TYPE_HAMMER):
	case (GW2::WEP_TYPE_LONGBOW):
	case (GW2::WEP_TYPE_SHORTBOW):
	case (GW2::WEP_TYPE_GREATSWORD):
	case (GW2::WEP_TYPE_RIFLE):
	case (GW2::WEP_TYPE_STAFF):
		return true;
	default:
		return false;
	}
}

static inline bool EquipSlotIsWeap(uint32_t slot)
{
	switch (slot)
	{
	case (GW2::EQUIP_SLOT_AQUATIC_WEAP1):
	case (GW2::EQUIP_SLOT_AQUATIC_WEAP2):
	case (GW2::EQUIP_SLOT_MAINHAND_WEAP1):
	case (GW2::EQUIP_SLOT_OFFHAND_WEAP1):
	case (GW2::EQUIP_SLOT_MAINHAND_WEAP2):
	case (GW2::EQUIP_SLOT_OFFHAND_WEAP2):
		return true;
	default:
		return false;
	}
}

static inline EquipItem *EquipItemBySlot(uint32_t slot, EquipItems *equipItems)
{
	EquipItem *eqItem = NULL;

	switch (slot) {
	case (GW2::EQUIP_SLOT_AQUATIC_HELM):
		eqItem = &equipItems->head_aqua;
		break;
	case (GW2::EQUIP_SLOT_BACK):
		eqItem = &equipItems->back;
		break;
	case (GW2::EQUIP_SLOT_CHEST):
		eqItem = &equipItems->chest;
		break;
	case (GW2::EQUIP_SLOT_BOOTS):
		eqItem = &equipItems->boots;
		break;
	case (GW2::EQUIP_SLOT_GLOVES):
		eqItem = &equipItems->gloves;
		break;
	case (GW2::EQUIP_SLOT_HELM):
		eqItem = &equipItems->head;
		break;
	case (GW2::EQUIP_SLOT_PANTS):
		eqItem = &equipItems->leggings;
		break;
	case (GW2::EQUIP_SLOT_SHOULDERS):
		eqItem = &equipItems->shoulder;
		break;
	case (GW2::EQUIP_SLOT_ACCESSORY1):
		eqItem = &equipItems->acc_ear1;
		break;
	case (GW2::EQUIP_SLOT_ACCESSORY2):
		eqItem = &equipItems->acc_ear2;
		break;
	case (GW2::EQUIP_SLOT_RING1):
		eqItem = &equipItems->acc_ring1;
		break;
	case (GW2::EQUIP_SLOT_RING2):
		eqItem = &equipItems->acc_ring2;
		break;
	case (GW2::EQUIP_SLOT_AMULET):
		eqItem = &equipItems->acc_amulet;
		break;
	case (GW2::EQUIP_SLOT_AQUATIC_WEAP1):
		eqItem = &equipItems->wep1_aqua;
		break;
	case (GW2::EQUIP_SLOT_AQUATIC_WEAP2):
		eqItem = &equipItems->wep2_aqua;
		break;
	case (GW2::EQUIP_SLOT_MAINHAND_WEAP1):
		eqItem = &equipItems->wep1_main;
		break;
	case (GW2::EQUIP_SLOT_OFFHAND_WEAP1):
		eqItem = &equipItems->wep1_off;
		break;
	case (GW2::EQUIP_SLOT_MAINHAND_WEAP2):
		eqItem = &equipItems->wep2_main;
		break;
	case (GW2::EQUIP_SLOT_OFFHAND_WEAP2):
		eqItem = &equipItems->wep2_off;
		break;
	};

	return eqItem;
}

bool Ch_GetInventory(uintptr_t cptr, EquipItems *equipItems)
{
	bool bRet = false;

	__try {

		if (!cptr || !equipItems)
			return bRet;

		memset(equipItems, 0, sizeof(*equipItems));

		hl::ForeignClass character = (void*)cptr;
		if (character)
		{
			hl::ForeignClass inventory = character.get<void*>(OFF_CHAR_INVENTORY);
			if (inventory) {

				for (uint32_t i = 0; i < GW2::EQUIP_SLOT_END; i++) {

					EquipItem *eqItem = EquipItemBySlot(i, equipItems);
					if (!eqItem)
						continue;

					eqItem->type = i;
					eqItem->is_wep = EquipSlotIsWeap(i);

					hl::ForeignClass equip = inventory.get<void*>(OFF_INVENTORY_EQUIPMENT + i * sizeof(void*));
					if (!equip) {

						eqItem->stat_def.id = 0xFFFF; // "NOEQUIP"
						if (eqItem->is_wep)
							eqItem->wep_type = GW2::WEP_TYPE_NOEQUIP;

					}
					else {

						eqItem->ptr = (u64)equip.data();

						hl::ForeignClass itemDef = equip.get<void*>(OFF_EQITEM_ITEM_DEF);
						if (itemDef) {
							eqItem->item_def.ptr = (u64)itemDef.data();
							eqItem->item_def.id = itemDef.get<uint32_t>(OFF_ITEM_DEF_ID);
							eqItem->item_def.rarity = itemDef.get<GW2::ItemRarity>(OFF_ITEM_DEF_RARITY);
							eqItem->item_def.name = lru_find(GW2::CLASS_TYPE_ITEM, eqItem->item_def.ptr, NULL, 0);
							if (eqItem->is_wep) {
								hl::ForeignClass wepDef = itemDef.get<void*>(OFF_ITEM_DEF_WEAPON);
								if (wepDef) {
									eqItem->wep_type = wepDef.get<int32_t>(OFF_WEP_TYPE);
								}
							}

							hl::ForeignClass itemType = itemDef.get<void*>(OFF_ITEM_DEF_ITEMTYPE);
							if (itemType) {
								eqItem->name = lru_find(GW2::CLASS_TYPE_ITEMTYPE, (u64)itemType.data(), NULL, 0);
							}
						}

						hl::ForeignClass statDef = equip.get<void*>(eqItem->is_wep ? OFF_EQITEM_STATS_WEP : OFF_EQITEM_STATS_ARMOR);
						if (statDef) {
							eqItem->stat_def.ptr = (u64)statDef.data();
							eqItem->stat_def.id = statDef.get<uint32_t>(OFF_STAT_ID);
							eqItem->stat_def.name = lru_find(GW2::CLASS_TYPE_STAT, eqItem->stat_def.ptr, NULL, 0);
						}

						// Runes / Sigils
						hl::ForeignClass up1Def = equip.get<void*>(eqItem->is_wep ? OFF_EQITEM_UPGRADE_WEP1 : OFF_EQITEM_UPGRADE_ARMOR);
						if (up1Def) {
							eqItem->upgrade1.ptr = (u64)up1Def.data();
							eqItem->upgrade1.id = up1Def.get<uint32_t>(OFF_ITEM_DEF_ID);
							eqItem->upgrade1.rarity = up1Def.get<GW2::ItemRarity>(OFF_ITEM_DEF_RARITY);
							eqItem->upgrade1.name = lru_find(GW2::CLASS_TYPE_UPGRADE, eqItem->upgrade1.ptr, NULL, 0);
						}

						// Only wep can have 2nd upgrade
						if (eqItem->is_wep && WeaponIs2H(eqItem->wep_type))
						{
							hl::ForeignClass up2Def = equip.get<void*>(OFF_EQITEM_UPGRADE_WEP2);
							if (up2Def) {
								eqItem->upgrade2.ptr = (u64)up2Def.data();
								eqItem->upgrade2.id = up2Def.get<uint32_t>(OFF_ITEM_DEF_ID);
								eqItem->upgrade2.rarity = up2Def.get<GW2::ItemRarity>(OFF_ITEM_DEF_RARITY);
								eqItem->upgrade2.name = lru_find(GW2::CLASS_TYPE_UPGRADE, eqItem->upgrade2.ptr, NULL, 0);
							}
						}

						// Infusions
						hl::ForeignClass upgradeDef = equip.call<void*>(OFF_EQITEM_VT_GET_UPGRADES);
						if (upgradeDef)
						{
							auto infusions = upgradeDef.get<ANet::Collection<uintptr_t, true>>(OFF_UPGRADE_DEF_INFUS_ARR);
							if (infusions.IsValid()) {
								uint32_t cap = 0;
								for (uint32_t j = 0; j < infusions.Capacity() && cap < ARRAYSIZE(eqItem->infus_arr); j++) {
									hl::ForeignClass infuse = (void*)infusions[j];
									if (infuse) {
										eqItem->infus_arr[cap].ptr = (u64)infuse.data();
										eqItem->infus_arr[cap].id = infuse.get<uint32_t>(OFF_ITEM_DEF_ID);
										eqItem->infus_arr[cap].rarity = infuse.get<GW2::ItemRarity>(OFF_ITEM_DEF_RARITY);
										eqItem->infus_arr[cap].name = lru_find(GW2::CLASS_TYPE_ITEM, eqItem->infus_arr[cap].ptr, NULL, 0);
										//DBGPRINT(TEXT("infus[%d]: id:%d  rarity:%d  ptr:%p"),
										//	cap-1, eqItem->infus_arr[cap-1].id, eqItem->infus_arr[cap-1].rarity, infuse.data());
										cap++;
									}
								}
								eqItem->infus_len = cap;
							}
						}

						// Skin
						if (itemDef) {
							hl::ForeignClass skinDef = equip.call<void*>(OFF_EQITEM_VT_GET_SKIN);
							if (skinDef) {
								eqItem->skin_def.ptr = (u64)skinDef.data();
								eqItem->skin_def.id = skinDef.get<uint32_t>(OFF_ITEM_DEF_ID);
								eqItem->skin_def.name = lru_find(GW2::CLASS_TYPE_SKIN, eqItem->skin_def.ptr, NULL, 0);
							}
						}
					}
				}

				bRet = true;
			}
		}
	}
	__except (BGDM_EXCEPTION("[Ch_GetInventory] access violation")) {
		;
	}
	return bRet;
}


uint32_t EqItem_GetUpgrades(uintptr_t eqptr, Array *outArr)
{
	uint32_t iRet = 0;
	if (!eqptr)
		return iRet;

	__try {
		hl::ForeignClass eqItem = (void*)eqptr;
		if (eqptr) {
			hl::ForeignClass upgradeDef = eqItem.call<void*>(OFF_EQITEM_VT_GET_UPGRADES);
			if (upgradeDef)
			{
				auto infusions = upgradeDef.get<ANet::Collection<uintptr_t, true>>(OFF_UPGRADE_DEF_INFUS_ARR);
				if (infusions.IsValid()) {
					for (uint32_t i = 0; i < infusions.Capacity(); i++) {
						infusions[i];
					}

					if (outArr)
						process_read((u64)upgradeDef.data() + OFF_UPGRADE_DEF_INFUS_ARR, outArr, sizeof(*outArr));
					iRet = infusions.Capacity();
				}
			}
		}
	}
	__except (BGDM_EXCEPTION("[EqItem_GetUpgrades] access violation")) {
		;
	}
	return iRet;
}

uintptr_t GetCtxContact(uintptr_t fncGetCtxContact)
{
	typedef uintptr_t(__fastcall*fpVoid)(void);

	__try {

		if (!fncGetCtxContact)
			return 0;

		return ((fpVoid)fncGetCtxContact)();
	}
	__except (BGDM_EXCEPTION("[GetCtxContact] access violation")) {
		;
	}
	return 0;
}

uintptr_t GetCtxSquad(uintptr_t fncGetCtxSquad)
{
	typedef uintptr_t(__fastcall*fpVoid)(void);

	__try {

		if (!fncGetCtxSquad)
			return 0;

		return ((fpVoid)fncGetCtxSquad)();
	}
	__except (BGDM_EXCEPTION("[GetCtxSquad] access violation")) {
		;
	}
	return 0;
}

uintptr_t CtxContact_GetContact(uintptr_t pctx, uintptr_t pptr)
{
	__try {

		if (!pptr || !pctx)
			return 0;

		hl::ForeignClass ctx = (void*)pctx;
		if (!ctx)
			return 0;

		return ctx.call<uintptr_t>(OFF_CONTACT_CTX_GET_CONTACT, (void*)pptr);

	}
	__except (BGDM_EXCEPTION("[CtxContact_GetContact] access violation")) {
		;
	}
	return 0;
}

uintptr_t CtxContact_GetContactSelf(uintptr_t pctx)
{
	__try {

		if (!pctx)
			return 0;

		hl::ForeignClass ctx = (void*)pctx;
		if (!ctx)
			return 0;

		return ctx.call<uintptr_t>(0xE8);

	}
	__except (BGDM_EXCEPTION("[CtxContact_GetContactSelf] access violation")) {
		;
	}
	return 0;
}

const wchar_t *Contact_GetAccountName(uintptr_t ctptr)
{
	__try {

		if (!ctptr)
			return 0;

		hl::ForeignClass contact = (void*)ctptr;
		if (!contact) return 0;

		return contact.call<const wchar_t *>(0x130);
	}
	__except (BGDM_EXCEPTION("[Contact_GetAccountName] access violation")) {
		;
	}
	return 0;
}

uint32_t Pl_GetSquadSubgroup(uintptr_t squadptr, uintptr_t pctx)
{

#ifdef _MSC_VER
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#elif defined(__GNUC__)
#  define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif

	PACK(
		struct squadArrElement {
		uint64_t uuid;
		uint64_t unknown1;
		uint32_t unknown2;
		uint32_t subGroup;
		uint32_t unknown3;
	});

	uint32_t retVal = 0;

	__try {

		if (!squadptr || !pctx)
			return 0;

		hl::ForeignClass squadCtx = (void*)squadptr;
		if (!squadCtx)
			return retVal;

		uint32_t groupNo = squadCtx.get<uint32_t>(OFF_SQUAD_GROUP_NUM);
		auto squadArr = squadCtx.get<ANet::Collection<squadArrElement, false>>(OFF_SQUAD_MEMBER_ARR);
		if (!squadArr.IsValid() || groupNo == 0)
			return retVal;

		//DBGPRINT(TEXT("squadCtx %p"), squadCtx.data());

		hl::ForeignClass contact = (void*)pctx;
		if (!contact)
			return retVal;

		uint64_t uuid = contact.get<uint64_t>(OFF_CONTACT_UUID);
		uint32_t isPartySquad = contact.get<uint32_t>(OFF_CONTACT_ISPARTYSQUAD);
		if (uuid == 0 || isPartySquad == 0)
			return retVal;

		for (uint32_t i = 0; i < squadArr.Capacity(); i++) {

			if (squadArr[i].uuid == uuid) {
				if (squadArr[i].subGroup < SQUAD_MAX_SUBGROUPS) {
					retVal = squadArr[i].subGroup + 1;
					return retVal;
				}
				break;
			}
		}

	}
	__except (BGDM_EXCEPTION("[Pl_GetSquadSubgroup] access violation")) {
		;
	}
	return retVal;
}


bool Pl_IsInPartyOrSquad(uintptr_t pptr, uintptr_t pctx, uintptr_t fncGetCtxContact)
{
	__try {

		if (!pptr) return false;
		if (!pctx) pctx = CtxContact_GetContact(fncGetCtxContact, pptr);
		if (!pctx) return false;

		hl::ForeignClass contact = (void*)pctx;
		if (!contact)
			return false;

		uint64_t uuid = contact.get<uint64_t>(OFF_CONTACT_UUID);
		uint32_t isPartySquad = contact.get<uint32_t>(OFF_CONTACT_ISPARTYSQUAD);
		if (uuid > 0 && isPartySquad > 0) {
			//DBGPRINT(TEXT("%p is in Squad/Party [isPartySquad=%d]"), pptr, isPartySquad);
			return true;
		}
	}
	__except (BGDM_EXCEPTION("[Pl_IsInPartyOrSquad] access violation")) {
		;
	}
	return false;
}


static bool memRead(uintptr_t offset, LPVOID data, SIZE_T size)
{
	return ReadProcessMemory(GetCurrentProcess(), (LPVOID)offset, data, size, NULL) == TRUE;
}

static bool memWrite(uintptr_t offset, LPVOID data, SIZE_T size)
{
	// Apply the hook by writing the jump.
	DWORD  oldProtect;
	if (VirtualProtect((LPVOID)offset, size, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		memcpy((void*)offset, data, size);
		VirtualProtect((LPVOID)offset, size, oldProtect, &oldProtect);
		return true;
	}
	return false;
}

static bool memPatch(uintptr_t ptr, intptr_t offset, const UCHAR *orig, const UCHAR *patch, SIZE_T size, int mode)
{
	static UCHAR buff[128];
	if (ptr == 0 || ptr == (uintptr_t)(-1)) return false;
	if (!orig || !patch || size == 0) return false;
	if (size > sizeof(buff)) size = sizeof(buff);

	__try {

		ptr += offset;
		memset(buff, 0, size);
		memRead(ptr, buff, size);

		// 3 - IsEnabled
		if (mode == 3) {
			if (memcmp(buff, patch, size) == 0) return true;
			else return false;
		}

		if (memcmp(buff, orig, size) == 0 ||
			memcmp(buff, patch, size) == 0) {

			// 2 - IsPtrValid check
			if (mode == 2) return true;

			// 1 - Patch
			// 0 - Restore
			LPVOID data = mode == 1 ? (LPVOID)patch : (LPVOID)orig;
			if (memWrite(ptr, data, size)) return true;
		}

	}
	__except (BGDM_EXCEPTION("[memPatch] access violation")) {
		;
	}
	return false;
}

static bool memScan(const char *sig, const char *mask, uintptr_t *pptr)
{
	if (pptr == 0 || *pptr == (uintptr_t)(-1)) return false;

	__try {

		if (*pptr == 0 && sig && mask) {

			*pptr = process_scan(sig, mask);
			if (!*pptr) *pptr = (uintptr_t)(-1);
			DBGPRINT(TEXT("[memScan('%s')=%p]"), mask, *pptr);
		}
	}
	__except (BGDM_EXCEPTION("[memScan] access violation")) {
		;
	}

	if (*pptr && *pptr != (uintptr_t)(-1))
		return true;
	return false;
}

static bool memScanPatch(const char *sig, const char *mask, uintptr_t *pptr, intptr_t offset, const UCHAR *orig, const UCHAR *patch, SIZE_T size, int mode)
{
	if (!memScan(sig, mask, pptr))
		return false;
	return memPatch(*pptr, offset, orig, patch, size, mode);
}

uintptr_t TR_GetHPBarPtr()
{
	static uintptr_t ptr = 0;

	if (ptr && ptr != (uintptr_t)-1) return ptr;

	// Add HP to nameplates always (as if hovering) - players
	// 32 Bit:
	/*==========================================================================================
	disabled: 00868D9D | 8B 7D F0                 | mov edi,dword ptr ss:[ebp-10]
	enabled:  00868D9D | 8B 7D EF                 | mov edi,dword ptr ss:[ebp-11]
	============================================================================================
	00868D99 | EB 02                    | jmp gw2.868D9D                                       |
	00868D9B | 33 F6                    | xor esi,esi                                          |
	00868D9D | 8B 7D F0                 | mov edi,dword ptr ss:[ebp-10]                        |
	00868DA0 | 85 FF                    | test edi,edi                                         |
	00868DA2 | 75 32                    | jne gw2.868DD6                                       |
	00868DA4 | 39 7D F8                 | cmp dword ptr ss:[ebp-8],edi                         |
	00868DA7 | 74 10                    | je gw2.868DB9                                        |
	00868DA9 | E8 F2 B1 FE FF           | call gw2.853FA0                                      |
	00868DAE | 8B C8                    | mov ecx,eax                                          |
	00868DB0 | 8B 10                    | mov edx,dword ptr ds:[eax]                           |
	00868DB2 | FF 52 48                 | call dword ptr ds:[edx+48]                           |
	00868DB5 | 85 C0                    | test eax,eax                                         |
	00868DB7 | 75 1D                    | jne gw2.868DD6                                       |
	00868DB9 | 85 F6                    | test esi,esi                                         |
	00868DBB | 75 19                    | jne gw2.868DD6                                       |
	00868DBD | 8D 4B 54                 | lea ecx,dword ptr ds:[ebx+54]                        |
	00868DC0 | C7 01 CD CC 4C 3D        | mov dword ptr ds:[ecx],3D4CCCCD                      |
	00868DC6 | 51                       | push ecx                                             |
	00868DC7 | 89 34 24                 | mov dword ptr ss:[esp],esi                           |
	00868DCA | E8 71 41 F3 FF           | call gw2.79CF40                                      |
	============================================================================================
	// 64 Bit: "333?" the hit with the JE right above it (10th hit), scroll up
	// "C7 01 CD CC 4C 3D E9" (-0x2A)
	======================================================================================================
	disabled: 00007FF6B042E90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]
	disabled: 00007FF6B042E912 | 45 85 FF                            | test r15d,r15d
	disabled: 00007FF6B042E915 | 75 33                               | jne gw2-64.7FF6B042E94A
	enabled:  00007FF6B042E90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]
	enabled:  00007FF6B042E912 | 45 85 FF                            | test r15d,r15d
	enabled:  00007FF6B042E915 | 75 33                               | jne gw2-64.7FF6B042E94A
	=======================================================================================================
	00007FF6B042E8CD | FF 50 78                            | call qword ptr ds:[rax+78]                   |
	00007FF6B042E8D0 | 49 8B 16                            | mov rdx,qword ptr ds:[r14]                   |
	00007FF6B042E8D3 | 49 8B CE                            | mov rcx,r14                                  |
	00007FF6B042E8D6 | 48 8B D8                            | mov rbx,rax                                  |
	00007FF6B042E8D9 | FF 52 78                            | call qword ptr ds:[rdx+78]                   |
	00007FF6B042E8DC | 48 8B 13                            | mov rdx,qword ptr ds:[rbx]                   |
	00007FF6B042E8DF | 48 8B CB                            | mov rcx,rbx                                  |
	00007FF6B042E8E2 | 48 8B F8                            | mov rdi,rax                                  |
	00007FF6B042E8E5 | FF 52 10                            | call qword ptr ds:[rdx+10]                   |
	00007FF6B042E8E8 | 48 8B 17                            | mov rdx,qword ptr ds:[rdi]                   |
	00007FF6B042E8EB | 48 8B CF                            | mov rcx,rdi                                  |
	00007FF6B042E8EE | 0F 28 F0                            | movaps xmm6,xmm0                             |
	00007FF6B042E8F1 | FF 52 18                            | call qword ptr ds:[rdx+18]                   |
	00007FF6B042E8F4 | 0F 2F C6                            | comiss xmm0,xmm6                             |
	00007FF6B042E8F7 | 76 10                               | jbe gw2-64.7FF6B042E909                      |
	00007FF6B042E8F9 | 48 8B 8E 80 00 00 00                | mov rcx,qword ptr ds:[rsi+80]                |
	00007FF6B042E900 | E8 CB CE FF FF                      | call gw2-64.7FF6B042B7D0                     |
	00007FF6B042E905 | 85 C0                               | test eax,eax                                 |
	00007FF6B042E907 | 75 02                               | jne gw2-64.7FF6B042E90B                      |
	00007FF6B042E909 | 33 ED                               | xor ebp,ebp                                  |
	00007FF6B042E90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]                |
	00007FF6B042E912 | 45 85 FF                            | test r15d,r15d                               |
	00007FF6B042E915 | 75 33                               | jne gw2-64.7FF6B042E94A                      |
	...
	00007FF7DE1CE912 | 45 85 FF                            | test r15d,r15d                               |
	00007FF7DE1CE915 | 75 33                               | jne gw2-64.7FF7DE1CE94A                      |
	00007FF7DE1CE917 | 85 DB                               | test ebx,ebx                                 |
	00007FF7DE1CE919 | 74 15                               | je gw2-64.7FF7DE1CE930                       | <<
	00007FF7DE1CE91B | E8 F0 7A FE FF                      | call gw2-64.7FF7DE1B6410                     |
	00007FF7DE1CE920 | 48 8B C8                            | mov rcx,rax                                  |
	00007FF7DE1CE923 | 48 8B 10                            | mov rdx,qword ptr ds:[rax]                   |
	00007FF7DE1CE926 | FF 92 90 00 00 00                   | call qword ptr ds:[rdx+90]                   |
	00007FF7DE1CE92C | 85 C0                               | test eax,eax                                 |
	00007FF7DE1CE92E | 75 1A                               | jne gw2-64.7FF7DE1CE94A                      |
	00007FF7DE1CE930 | 85 ED                               | test ebp,ebp                                 |
	00007FF7DE1CE932 | 75 16                               | jne gw2-64.7FF7DE1CE94A                      |
	00007FF7DE1CE934 | 48 8D 8E A0 00 00 00                | lea rcx,qword ptr ds:[rsi+A0]                |
	00007FF7DE1CE93B | 41 0F 28 C8                         | movaps xmm1,xmm8                             |
	00007FF7DE1CE93F | C7 01 CD CC 4C 3D                   | mov dword ptr ds:[rcx],3D4CCCCD              |
	00007FF7DE1CE945 | E9 DF 00 00 00                      | jmp gw2-64.7FF7DE1CEA29                      |
	=====================================================================================================*/
	memScan(
		"\xC7\x01\xCD\xCC\x4C\x3D\xE9",
		"xxxxxxx",
		&ptr
	);
	return ptr;
}

bool TR_SetHPChars(int mode)
{
	/*====================================================================================================
	disabled: 00007FF7DE1CE90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]
	disabled: 00007FF7DE1CE912 | 45 85 FF                            | test r15d,r15d
	disabled: 00007FF7DE1CE915 | 75 33                               | jne gw2-64.7FF6B042E94A
	enabled:  00007FF7DE1CE90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]
	enabled:  00007FF7DE1CE912 | 45 85 FF                            | test r15d,r15d
	enabled:  00007FF7DE1CE915 | 75 33                               | jne gw2-64.7FF6B042E94A
	=======================================================================================================
	00007FF7DE1CE905 | 85 C0                               | test eax,eax                                 |
	00007FF7DE1CE907 | 75 02                               | jne gw2-64.7FF7DE1CE90B                      |
	00007FF7DE1CE909 | 33 ED                               | xor ebp,ebp                                  |
	00007FF7DE1CE90B | 8B 9C 24 E0 00 00 00                | mov ebx,dword ptr ss:[rsp+E0]                |
	00007FF7DE1CE912 | 45 85 FF                            | test r15d,r15d                               |
	00007FF7DE1CE915 | 75 33                               | jne gw2-64.7FF7DE1CE94A                      |
	00007FF7DE1CE917 | 85 DB                               | test ebx,ebx                                 |
	00007FF7DE1CE919 | 74 15                               | je gw2-64.7FF7DE1CE930                       |
	00007FF7DE1CE91B | E8 F0 7A FE FF                      | call gw2-64.7FF7DE1B6410                     |
	=====================================================================================================*/
	static const intptr_t offset = -0x31;
	static const UCHAR orig[8] = { 0xF8 ,0x00 ,0x00 ,0x00 ,0x45 ,0x85 ,0xFF ,0x75 };
	static const UCHAR patch[8] = { 0x00 ,0x00 ,0x00 ,0x00 ,0x45 ,0x85 ,0xFF ,0xEB };
	return memPatch(TR_GetHPBarPtr(), offset, orig, patch, sizeof(orig), mode);
}

bool TR_SetHPCharsBright(int mode)
{
	/*====================================================================================================
	disabled: 00007FF7DE1CE9BF | 85 C0                               | test eax,eax
	enabled:  00007FF7DE1CE9BF | 85 C8                               | test eax,ecx
	=======================================================================================================
	00007FF7DE1CE9BB | 41 FF 50 70                         | call qword ptr ds:[r8+70]                    |
	00007FF7DE1CE9BF | 85 C0                               | test eax,eax                                 |
	00007FF7DE1CE9C1 | 74 14                               | je gw2-64.7FF7DE1CE9D7                       |
	00007FF7DE1CE9C3 | F3 0F 59 35 9D A2 B2 00             | mulss xmm6,dword ptr ds:[7FF7DECF8C68]       | 7FF7DECF8C68:"333?"
	00007FF7DE1CE9CB | 85 ED                               | test ebp,ebp                                 |
	00007FF7DE1CE9CD | 74 08                               | je gw2-64.7FF7DE1CE9D7                       |
	00007FF7DE1CE9CF | F3 0F 59 35 BD D7 AC 00             | mulss xmm6,dword ptr ds:[7FF7DEC9C194]       |
	=====================================================================================================*/
	static const intptr_t offset = 0x78;
	static const UCHAR orig[3] = { 0x85 ,0xC0 ,0x74 };
	static const UCHAR patch[3] = { 0x85 ,0xC8 ,0x74 };
	return memPatch(TR_GetHPBarPtr(), offset, orig, patch, sizeof(orig), mode);
}

#ifdef __cplusplus
}
#endif