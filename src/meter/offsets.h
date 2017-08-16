#pragma once

// The CN build ID at the time of latest patch
#define CN_BUILD_ID	80250

// Offset macro to accomodate for CN update delay
#define OFFSET_FOR(x)	((buildId() == 0 || g_state.cn_build_id_override > 0 && g_state.cn_build_id_override < buildId()) ? ##x##_CURR : ##x##_PREV)

// Mumble link name.
#define MUMBLE_LINK "MumbleLink"

// UI CTX offsets
#define OFF_UCTX_SCALE 0x54

// CContext
// CharClient::CContext* m_charContext;
// Main game context, saved in the TLS (thread-local-storage)
// Offset of the CharClientContext can be found right below the assert
// "!IsPlayer() || GetPlayer()"
/*================================================================================================
00007FF6C2490BE2 | 48 8D 15 17 E2 E3 00                | lea rdx,qword ptr ds:[7FF6C32CEE00]     | 7FF6C32CEE00:"..\\..\\..\\Game\\Char\\Cli\\ChCliCharacter.cpp"
00007FF6C2490BE9 | 48 8D 0D 20 E4 E3 00                | lea rcx,qword ptr ds:[7FF6C32CF010]     | 7FF6C32CF010:"!IsPlayer() || GetPlayer()"
00007FF6C2490BF0 | 41 B8 8A 07 00 00                   | mov r8d,78A                             |
00007FF6C2490BF6 | E8 E5 EF DD FF                      | call gw2-64.7FF6C226FBE0                |
00007FF6C2490BFB | E8 10 34 DC FF                      | call gw2-64.7FF6C2254010                |
00007FF6C2490C00 | 48 8B A8 90 00 00 00                | mov rbp,qword ptr ds:[rax+90]           |		<== THIS
================================================================================================*/
#define OFF_TLS_SLOT 0x8
#define OFF_CTX_CCTX 0x90

//
// Client Context offsets (g_client_ctx)
// CharClient::CContext
// ANet::Array<CharClient::CCharacter*> m_characterArray;
// "m_characterArray.Count() <= agentId || !m_characterArray[agentId]"
#define OFF_CCTX_CHARS 0x60
// ANet::Array<CharClient::CPlayer*> m_playerArray;
// "m_playerArray.Count() <= playerId || !m_playerArray[playerId]"
#define OFF_CCTX_PLAYS 0x80					// charctxPlayerArray
// CharClient::CCharacter* m_controlledCharacter;
// "!m_controlledCharacter || m_controlledCharacter->IsFinalized()"
#define OFF_CCTX_CONTROLLED_CHAR 0x98
// consecutive members: m_controlledPlayer = m_controlledCharacter + 8
// CharClient::CCharacter* m_controlledPlayer;
// "player == m_controlledPlayer"
// Can also be retrieved from the VT "CharClientContext()->GetControlledPlayer()"
#define OFF_CCTX_CONTROLLED_PLAYER 0xA0

// squadContext offsets
#define OFF_SQUAD_MEMBER_ARR 0x90
#define OFF_SQUAD_GROUP_NUM 0x10C
#define OFF_SQUAD_GROUP_ARR 0x110
#define SQUAD_ARR_ELEM_SIZE 0x1c
#define OFF_SQUAD_ELEM_UUID 0x0
#define OFF_SQUAD_ELEM_SUBGROUP 0x14

// "wmAgent->GetAgent() != agent"
#define OFF_WMAGENT_VT_GET_AGENT 0x60
// search for "guildTagLogoFrame" and go the 2nd "agent" assert going up
/*===================================================================================
00007FF6C27C4E0F | 48 8D 15 EA 02 BD 00 | lea rdx, qword ptr ds : [7FF6C3395100]	| 7FF6C3395100 : "..\\..\\..\\Game\\Ui\\Widgets\\AgentStatus\\AsName.cpp"
00007FF6C27C4E16 | 48 8D 0D 57 E1 AF 00 | lea rcx, qword ptr ds : [7FF6C32C2F74]	| 7FF6C32C2F74 : "agent"
00007FF6C27C4E1D | 41 B8 FB 00 00 00	| mov r8d, FB								|
00007FF6C27C4E23 | E8 B8 AD AA FF		| call gw2 - 64.7FF6C226FBE0				|
00007FF6C27C4E28 | 49 8B CF				| mov rcx, r15								|
00007FF6C27C4E2B | E8 B0 1B 8B 00		| call gw2 - 64.7FF6C30769E0				|
00007FF6C27C4E30 | 48 85 C0				| test rax, rax								|
00007FF6C27C4E33 | 74 1C				| je gw2 - 64.7FF6C27C4E51					|
00007FF6C27C4E35 | 48 8B 10				| mov rdx, qword ptr ds : [rax]				|
00007FF6C27C4E38 | 48 8B C8				| mov rcx, rax								|
00007FF6C27C4E3B | FF 52 70				| call qword ptr ds : [rdx + 70]			|		<== THIS
===================================================================================*/
#define OFF_WMAGENT_VT_GET_CODED_NAME 0x70

// Agent Context offsets "ViewAdvanceAgentSelect" (g_agent_ctx)
// Agent::CAgentBase* m_lockedSelection;
#define OFF_ACCTX_AGENT 0x230
// Agent::CAgentBase* m_autoSelection;
#define OFF_ACCTX_AGENT_AUTO 0x50
// Agent::CAgentBase* m_hoverSelection;
#define OFF_ACCTX_AGENT_HOVER 0xF8
// "m_contextMode == CONTEXT_MODE_NULL"
#define OFF_ACCTX_MODE 0x70
// D3DXVECTOR3 m_screenToWorld;
#define OFF_ACCTX_STOW 0x298

// Agent offsets.
// AgentView::CAvAgent
#define OFF_AGENT_ID 0x34			// OFF_AGENT_VT_GETID
#define OFF_AGENT_TYPE 0x38			// OFF_AGENT_VT_GETTYPE
#define OFF_AGENT_GADGET 0x70
#define OFF_AGENT_TRANSFORM 0x40
// Transform offsets.
#define OFF_TRANSFORM_POS 0x30
// Agent::CAgentBase
// AgentCategory GetCategory();
// "agent->GetCategory() == AGENT_CATEGORY_CHAR"
#define OFF_AGENT_VT_GETCAT 0x20
// int GetAgentId();
// "targetAgent && targetAgent->GetAgentId()"
#define OFF_AGENT_VT_GETID 0xc0
// AgentType GetType();
// "agent->GetType() == AGENT_TYPE_CHAR"
// "m_outOfRangeActivationTargetAgent->GetType() == AGENT_TYPE_GADGET_ATTACK_TARGET"
#define OFF_AGENT_VT_GETTYPE 0x140
// void GetPos(D3DXVECTOR4* pPos);
// "75 ?? 4C 8B 07 48 8D 54 24 ?? 48 8B CF 41 FF 90"
/*===============================================================================
00007FF6C30A1EB0 | 48 89 5C 24 08		| mov qword ptr ss : [rsp + 8], rbx		|
00007FF6C30A1EB5 | 57					| push rdi								|
00007FF6C30A1EB6 | 48 83 EC 30			| sub rsp, 30							|
00007FF6C30A1EBA | 48 8B D9				| mov rbx, rcx							|
00007FF6C30A1EBD | 48 8B 89 20 01 00 00 | mov rcx, qword ptr ds : [rcx + 120]	|
00007FF6C30A1EC4 | 48 8B 41 08			| mov rax, qword ptr ds : [rcx + 8]		|
00007FF6C30A1EC8 | 48 83 C1 08			| add rcx, 8							|
00007FF6C30A1ECC | FF 90 E0 00 00 00	| call qword ptr ds : [rax + E0]		|
00007FF6C30A1ED2 | 48 8B C8				| mov rcx, rax							|
00007FF6C30A1ED5 | 48 8B F8				| mov rdi, rax							|
00007FF6C30A1ED8 | 48 8B 10				| mov rdx, qword ptr ds : [rax]			|
00007FF6C30A1EDB | FF 92 00 01 00 00	| call qword ptr ds : [rdx + 100]		|
00007FF6C30A1EE1 | 85 C0				| test eax, eax							|
00007FF6C30A1EE3 | 75 35				| jne gw2 - 64.7FF6C30A1F1A				|
00007FF6C30A1EE5 | 4C 8B 07				| mov r8, qword ptr ds : [rdi]			|
00007FF6C30A1EE8 | 48 8D 54 24 20		| lea rdx, qword ptr ss : [rsp + 20]	|
00007FF6C30A1EED | 48 8B CF				| mov rcx, rdi							|
00007FF6C30A1EF0 | 41 FF 90 20 01 00 00 | call qword ptr ds : [r8 + 120]		|		<== THIS
===============================================================================*/
#define OFF_AGENT_VT_GETPOS 0x120

// Char offsets.
// CharClient::CCharacter
// ChCliCharacter
// "m_agent && (m_agent->GetAgentId() == character->GetAgentId() || m_masterCharacter == character)"
#define OFF_CHAR_AGENT 0x88
// Search for "character->GetAgent() == m_agent" (first LEA up)
#define OFF_CHAR_SUBCLASS 0x8
// "character->GetAgent() == m_agent"
// "m_agent && (m_agent->GetAgentId() == character->GetAgentId() || m_masterCharacter == character)"
#define OFF_CHAR_VT_GET_AGENT_ID 0x198
// "!IsPlayer() || GetPlayer()"
#define OFF_CHAR_VT_IS_PLAYER 0x450
// "!IsPlayer() || GetPlayer()"
// "m_ownerCharacter->GetPlayer() == CharClientContext()->GetControlledPlayer()"
// "playerId"
#define OFF_CHAR_VT_GET_PLAYER 0x288
#define OFF_CHAR_VT_GET_PLAYER_ID 0x290
#define OFF_CHAR_PLAYER_ID 0x1D8
// bool IsAlive();
// "character->IsAlive() || (character->IsDowned() && character->IsInWater())"
#define OFF_CHAR_VT_IS_ALIVE 0x8
// bool IsDowned();
#define OFF_CHAR_VT_IS_DOWNED 0x350
// "character->IsPlayer() || character->IsMonsterClone()"
#define OFF_CHAR_VT_IS_CLONE 0x3F8
// "IsPlayer() || IsMonster()"
#define OFF_CHAR_VT_IS_MONSTER 0x3D0
// m_combatant
// Looks like combatant is simply char ptr + 0x30?
// i.e. no need to read the addr as 'c->cbptr = c->cptr+0x30;'
// Search for "TextValidateLiteral(m_nameOverride.Ptr())" scroll 1 function down (quite a lot)
// or search  for "character && character->IsFinalized()" (last hit) and scroll up 3 functions
/*=======================================================================================
00007FF6C248E830 | 48 85 C9                            | test rcx,rcx					|		<== OFF_CHAR_VT_GET_CMBTNT (CHAR_VT+offset)
00007FF6C248E833 | 74 05                               | je gw2-64.7FF6C248E83A			|
00007FF6C248E835 | 48 8D 41 30                         | lea rax,qword ptr ds:[rcx+30]	|		<== OFF_CHAR_COMBATANT
00007FF6C248E839 | C3                                  | ret							|
00007FF6C248E83A | 33 C0                               | xor eax,eax					|
00007FF6C248E83C | C3                                  | ret							|
00007FF6C248E83D | CC                                  | int3							|
00007FF6C248E83E | CC                                  | int3							|
00007FF6C248E83F | CC                                  | int3							|
=======================================================================================*/
#define OFF_CHAR_COMBATANT 0x30
// Seems that the offset is the same at 0x110 and 0x118
// Search for "buffModifier" -0x31 and look at the first JMP instruction
// 00007FF65105D475 | 48 FF A2 18 01 00 00				| jmp qword ptr ds:[rdx+118]	|
#define OFF_CHAR_VT_GET_CMBTNT 0x110
// "speciesDef" and look for the offset past the first ASM::ret
//  or 2nd "speciesDef" with "m_kennel", calls the char VT
#define OFF_CHAR_VT_GET_SPECIES_DEF 0x2F0
#define OFF_SPECIES_DEF_ID 0x28
#define OFF_SPECIES_DEF_HASHID 0x18c
// Search for "speciesDef" with "m_kennel" right after
// (1st hit of "m_kennel", 2nd hit of "speciesDef")
// the VT is the 3 call up from the "speciesDef" assert
#define OFF_CHAR_VT_IS_RANGER_PET 0x448

// Attitude m_attitudeTowardControlled;
// "m_attitudeTowardControlled < Content::AFFINITY_ATTITUDES"
#define OFF_CHAR_ATTITUDE 0xA8
// CharClient::CBreakBar* m_breakBar;
// "m_breakBar" (only works as offset from gadget)
// search for "comboDef" and scroll down till "data"
// the VT is the 2nd function up, first offset is the breakbar
// "40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 BA 01 00 00 00"
/*===================================================================================
00007FF6C2494120 | 40 53				| push rbx									|
00007FF6C2494122 | 48 83 EC 20			| sub rsp, 20								|
00007FF6C2494126 | 48 8B D9				| mov rbx, rcx								|
00007FF6C2494129 | 48 8B 89 B0 00 00 00 | mov rcx, qword ptr ds : [rcx + B0]		|		<== THIS
00007FF6C2494130 | 48 85 C9				| test rcx, rcx								|
00007FF6C2494133 | 74 32				| je gw2 - 64.7FF6C2494167					|
00007FF6C2494135 | 48 8B 01				| mov rax, qword ptr ds : [rcx]				|
00007FF6C2494138 | BA 01 00 00 00		| mov edx, 1								|
00007FF6C249413D | FF 10				| call qword ptr ds : [rax]					|
00007FF6C249413F | 48 8D 8B C8 00 00 00 | lea rcx, qword ptr ds : [rbx + C8]		|
00007FF6C2494146 | 4C 8D 44 24 30		| lea r8, qword ptr ss : [rsp + 30]			|
00007FF6C249414B | 48 8D 15 26 2D 13 00 | lea rdx, qword ptr ds : [7FF6C25C6E78]	|
===================================================================================*/
#define OFF_CHAR_BREAKBAR 0xB0
// CharClient::CHealth* m_health;
// "m_health"
#define OFF_CHAR_HP 0x358
// CharClient::CInventory* m_inventory;
// "m_inventory" (first hit in ChCliCharacter::)
#define OFF_CHAR_INVENTORY 0x360
// CharClient::CCoreStats* m_coreStats;
// "m_coreStats" (first hit in ChCliCharacter::)
#define OFF_CHAR_STATS 0x2F8
// "TextValidateLiteral(m_nameOverride.Ptr())"
#define OFF_CHAR_NAME 0x1B8
// m_profession
#define OFF_CHAR_PROFESSION_CURR 0x450
#define OFF_CHAR_PROFESSION_PREV 0x440
#define OFF_CHAR_PROFESSION OFFSET_FOR(OFF_CHAR_PROFESSION)
#define OFF_PROFESSION_STANCE 0x40			// profStance
#define OFF_PROFESSION_ENERGY_CUR 0x50		// profEnergy
#define OFF_PROFESSION_ENERGY_MAX 0x54		// profEnergyMax
// Search for "TextValidateLiteral(m_name.Ptr())" 1 function up
/*================================================================================================
00007FF6C24987AF | CC                                  | int3                                    |
00007FF6C24987B0 | 48 8B 41 20                         | mov rax,qword ptr ds:[rcx+20]           |
00007FF6C24987B4 | 48 85 C0                            | test rax,rax                            |
00007FF6C24987B7 | 74 0E                               | je gw2-64.7FF6C24987C7                  |
00007FF6C24987B9 | 8B 88 50 01 00 00                   | mov ecx,dword ptr ds:[rax+150]          |		<== THIS
00007FF6C24987BF | C0 E9 04                            | shr cl,4                                |
00007FF6C24987C2 | F6 C1 01                            | test cl,1                               |
00007FF6C24987C5 | 75 02                               | jne gw2-64.7FF6C24987C9                 |
00007FF6C24987C7 | 33 C0                               | xor eax,eax                             |
00007FF6C24987C9 | C3                                  | ret                                     |
================================================================================================*/
#define OFF_CHAR_FLAGS 0x160
// CharClient::CEndurance* m_endurance;
// "!m_endurance"
#define OFF_CHAR_ENDURANCE 0x340
#define OFF_ENDURANCE_CUR 0x8				// int m_maxValue; endCurrent
#define OFF_ENDURANCE_MAX 0xC				// int m_currentValue; endMax
// "!m_skillbar"
#define OFF_CHAR_SKILLBAR_CURR 0x460
#define OFF_CHAR_SKILLBAR_PREV 0x450
#define OFF_CHAR_SKILLBAR OFFSET_FOR(OFF_CHAR_SKILLBAR)

// charID
// Search for "sourceCharacter->IsFinalized()" and scroll 2 functions down
/*================================================================================================
00007FF6C248FE4F | CC                                  | int3                                    |
00007FF6C248FE50 | 40 53                               | push rbx                                |
00007FF6C248FE52 | 48 83 EC 20                         | sub rsp,20                              |
00007FF6C248FE56 | 48 8B D9                            | mov rbx,rcx                             |
00007FF6C248FE59 | E8 A2 03 1D 00                      | call gw2-64.7FF6C2660200                |
00007FF6C248FE5E | 41 B9 01 00 00 00                   | mov r9d,1                               |
00007FF6C248FE64 | 48 8B 50 08                         | mov rdx,qword ptr ds:[rax+8]            |
00007FF6C248FE68 | 48 8D 48 08                         | lea rcx,qword ptr ds:[rax+8]            |
00007FF6C248FE6C | 4C 63 42 04                         | movsxd r8,dword ptr ds:[rdx+4]          |
00007FF6C248FE70 | 48 8B 93 88 00 00 00                | mov rdx,qword ptr ds:[rbx+88]           |		<== THIS
00007FF6C248FE77 | 49 03 C8                            | add rcx,r8                              |
================================================================================================*/
#define OFF_CHAR_ID 0x88

// Gadget object offsets.
#define OFF_GADGET_HP 0x1E0
#define OFF_GADGET_WBOSS 0x50

// World boss offsets.
#define OFF_WBOSS_HP 0x210

// Health offsets.
// CharClient::CHealth
#define OFF_HP_MAX 0x10
#define OFF_HP_VAL 0xC

// Breakbar offsets.
// float
#define OFF_BREAKBAR_STATE 0x40
#define OFF_BREAKBAR_VALUE 0x44

// Player offsets.
// Search for "charClient" 2nd function down
/*================================================================================================
00007FF6C24654EF | CC                                  | int3                                    |
00007FF6C24654F0 | 48 8B 89 A0 00 00 00                | mov rcx,qword ptr ds:[rcx+A0]           |
00007FF6C24654F7 | 48 85 C9                            | test rcx,rcx                            |
00007FF6C24654FA | 74 07                               | je gw2-64.7FF6C2465503                  |
00007FF6C24654FC | 48 8B 01                            | mov rax,qword ptr ds:[rcx]              |
00007FF6C24654FF | 48 FF 60 20                         | jmp qword ptr ds:[rax+20]               |		<== THIS
00007FF6C2465503 | 33 C0                               | xor eax,eax                             |
00007FF6C2465505 | C3                                  | ret                                     |
================================================================================================*/
#define OFF_PLAYER_VT_GET_CHAR 0x20
#define OFF_PLAYER_CHAR 0x20
#define OFF_PLAYER_NAME 0x68				// "TextValidateLiteral(m_name.Ptr())"
#define OFF_PLAYER_VT_GET_NAME 0x78			// "StrCmp(newName, m_character->GetPlayer()->GetName()) != 0"
// "m_character->GetPlayer()->GetSpecializationMgr() == specMgr"
#define OFF_PLAYER_VT_GET_SPECMGR 0x2b0

// Spec / Traif offset.
#define OFF_SPECMGR_SPECS 0x40
#define OFF_SPECMGR_TRAITS 0x60
#define OFF_SPEC_TYPE 0x28		// specType
#define OFF_SPEC_HASHID 0x2c	// hashId for CodedTextFromHashId
#define OFF_TRAIT_TYPE 0x28		// traitType
#define OFF_TRAIT_HASHID 0x64	// hashId for CodedTextFromHashId


// Stats offsets.
#define OFF_STATS_BASE 0xAC
#define OFF_STATS_PROFESSION 0x264
#define OFF_STATS_RACE 0x33					// BYTE;
#define OFF_STATS_GENDER 0x35				// BYTE;
#define OFF_STATS_LEVEL	0x1ec				// int m_level;
#define OFF_STATS_SCALED_LEVEL 0x21c		// int m_scaledLevel;


// ChCliInventory
// ANet::Array<Agent::ItCliItem*>
// "m_inventorySlots[slotIndex] == NULL"
#define OFF_INVENTORY_BAG 0xC8
#define OFF_INVENTORY_BANK 0xA8
// // "m_sharedSlots[slotIndex] == NULL"
#define OFF_INVENTORY_SHARED 0x128


// ChCliInventory::m_equipmentSlots
// uintptr_t[] array
// search for "m_equipmentSlots[equipSlot] == NULL"
// look for   "cmp qword ptr [rsi + 0x140], 0"	(140 being the offset)
// to break on Unequip() search for
// "location.equipSlot < arrsize(m_equipmentSlots)"
// look for   "cmp qword ptr ds:[rdi+rcx*8+140],rbx" (140 being the offset)
#define OFF_INVENTORY_EQUIPMENT 0x140


// Combatant buffs
#define OFF_CMBTNT_BUFFBAR 0x88		// cmbtntBuffBar
// Search for "itemSlotIndex" under "bankBag" assert look for the array modifier
/*================================================================================================
00007FF6C257B9A5 | 66 66 66 0F 1F 84 00 00 00 00 00    | nop word ptr ds:[rax+rax]               |
00007FF6C257B9B0 | 8B 17                               | mov edx,dword ptr ds:[rdi]              |
00007FF6C257B9B2 | 48 8B 43 20                         | mov rax,qword ptr ds:[rbx+20]           |
00007FF6C257B9B6 | 48 8D 0C 52                         | lea rcx,qword ptr ds:[rdx+rdx*2]        |
00007FF6C257B9BA | 83 7C C8 10 00                      | cmp dword ptr ds:[rax+rcx*8+10],0       |		<== THIS
================================================================================================*/
#define OFF_BUFFBAR_BUFF_ARR 0x20
// Search for "No valid case for switch variable 'buffModifier->GetFormulaType()'"
// All offsets can be found 2 functions below the assert
/*================================================================================================
00007FF6C251D79F | CC                                  | int3                                    |
00007FF6C251D7A0 | 48 89 5C 24 08                      | mov qword ptr ss:[rsp+8],rbx            |		<== OFF_BUFF_EF_TYPE
00007FF6C251D7A5 | 48 89 74 24 10                      | mov qword ptr ss:[rsp+10],rsi           |		<== OFF_BUFF_SKILL_DEF
00007FF6C251D7AA | 48 89 7C 24 18                      | mov qword ptr ss:[rsp+18],rdi           |		<== OFF_BUFF_BUFF_ID
00007FF6C251D7AF | 41 56                               | push r14                                |
00007FF6C251D7B1 | 48 83 EC 20                         | sub rsp,20                              |
00007FF6C251D7B5 | 49 8B 01                            | mov rax,qword ptr ds:[r9]               |
00007FF6C251D7B8 | 4C 8B F1                            | mov r14,rcx                             |
00007FF6C251D7BB | 48 83 C1 50                         | add rcx,50                              |
00007FF6C251D7BF | 44 8B 50 28                         | mov r10d,dword ptr ds:[rax+28]          |
00007FF6C251D7C3 | 48 8D 05 86 4E DC 00                | lea rax,qword ptr ds:[7FF6C32E2650]     |
00007FF6C251D7CA | 49 8B F1                            | mov rsi,r9                              |
00007FF6C251D7CD | 44 89 51 B8                         | mov dword ptr ds:[rcx-48],r10d          |
00007FF6C251D7D1 | 48 89 41 B0                         | mov qword ptr ds:[rcx-50],rax           |
00007FF6C251D7D5 | 33 C0                               | xor eax,eax                             |
00007FF6C251D7D7 | 48 89 41 C0                         | mov qword ptr ds:[rcx-40],rax           |		<== OFF_BUFF_DURATION
00007FF6C251D7DB | 48 89 41 D8                         | mov qword ptr ds:[rcx-28],rax           |		<== OFF_BUFF_SRC_AGENT
00007FF6C251D7DF | 48 89 41 E0                         | mov qword ptr ds:[rcx-20],rax           |
00007FF6C251D7E3 | 48 89 41 E8                         | mov qword ptr ds:[rcx-18],rax           |
00007FF6C251D7E7 | 49 8B F8                            | mov rdi,r8                              |
00007FF6C251D7EA | 48 8B DA                            | mov rbx,rdx                             |
00007FF6C251D7ED | E8 FE 97 A8 00                      | call gw2-64.7FF6C2FA6FF0                |
00007FF6C251D7F2 | 49 8D 4E 28                         | lea rcx,qword ptr ds:[r14+28]           |
00007FF6C251D7F6 | 48 8B D3                            | mov rdx,rbx                             |
00007FF6C251D7F9 | E8 F2 BA 03 00                      | call gw2-64.7FF6C25592F0                |
00007FF6C251D7FE | 49 89 7E 20                         | mov qword ptr ds:[r14+20],rdi           |
00007FF6C251D802 | 48 8B 06                            | mov rax,qword ptr ds:[rsi]              |
00007FF6C251D805 | 49 89 46 10                         | mov qword ptr ds:[r14+10],rax           |
00007FF6C251D809 | 8B 44 24 50                         | mov eax,dword ptr ss:[rsp+50]           |
00007FF6C251D80D | 41 89 46 18                         | mov dword ptr ds:[r14+18],eax           |		<== OFF_BUFF_BUFF_ID
00007FF6C251D811 | 8B 44 24 58                         | mov eax,dword ptr ss:[rsp+58]           |
00007FF6C251D815 | 41 89 46 40                         | mov dword ptr ds:[r14+40],eax           |
00007FF6C251D819 | E8 42 45 D4 FF                      | call gw2-64.7FF6C2261D60                |
00007FF6C251D81E | 48 8B 5C 24 30                      | mov rbx,qword ptr ss:[rsp+30]           |
00007FF6C251D823 | 48 8B 74 24 38                      | mov rsi,qword ptr ss:[rsp+38]           |
00007FF6C251D828 | 48 8B 7C 24 40                      | mov rdi,qword ptr ss:[rsp+40]           |
00007FF6C251D82D | 41 89 46 44                         | mov dword ptr ds:[r14+44],eax           |
00007FF6C251D831 | 8B 44 24 60                         | mov eax,dword ptr ss:[rsp+60]           |
00007FF6C251D835 | 41 89 46 48                         | mov dword ptr ds:[r14+48],eax           |
00007FF6C251D839 | 8B 44 24 68                         | mov eax,dword ptr ss:[rsp+68]           |
00007FF6C251D83D | 41 89 46 4C                         | mov dword ptr ds:[r14+4C],eax           |		<== OFF_BUFF_ACTIVE
00007FF6C251D841 | 49 8B C6                            | mov rax,r14                             |
00007FF6C251D844 | 48 83 C4 20                         | add rsp,20                              |
00007FF6C251D848 | 41 5E                               | pop r14                                 |
00007FF6C251D84A | C3                                  | ret                                     |
================================================================================================*/
#define OFF_BUFF_EF_TYPE 0x8
#define OFF_BUFF_SKILL_DEF 0x10	
#define OFF_BUFF_BUFF_ID 0x18
#define OFF_BUFF_SRC_AGENT 0x28
#define OFF_BUFF_DURATION 0x40
#define OFF_BUFF_ACTIVE 0x4c


// Skill definitions
#define OFF_SKILLDEF_EFFECT 0x28
#define OFF_SKILLDEF_INFO 0x60
#define OFF_SKILLDEF_STACK_TYPE 0xc
#define OFF_SKILLDEF_HASHID 0x34	// hashId for CodedTextFromHashId


// ItCliItem::
// m_itemDef
#define OFF_EQITEM_ITEM_DEF 0x40
// pointer back to char's inventory
#define OFF_EQITEM_INV 0x40
// returns item def
// "item->GetSkinDef() != skinDef"
#define OFF_EQITEM_VT_GET_SKIN 0x60
// returns upgradeDef
#define OFF_EQITEM_VT_GET_UPGRADES 0x210
// Infusions offset from the equipItem (after calling GetUpgrades)
#define OFF_EQITEM_INFUSE_ARR 0xB8
// Infusions offset from the GetUpgrades retval
#define OFF_UPGRADE_DEF_INFUS_ARR 0x18
// call GetUpgrades first
#define OFF_EQITEM_UPGRADES_ARR 0x28
// Upgrades are not in an array
// they are part of the equipItem
#define OFF_EQITEM_UPGRADE_ARMOR 0xC0
#define OFF_EQITEM_UPGRADE_WEP1 0xC8
#define OFF_EQITEM_UPGRADE_WEP2 0xD0
// statDef
#define OFF_EQITEM_STATS_ARMOR 0xA0
#define OFF_EQITEM_STATS_WEP 0xA8


// itemDef offsets.
#define OFF_ITEM_DEF_ID 0x28
#define OFF_ITEM_DEF_TYPE 0x2C
#define OFF_ITEM_DEF_WEAPON 0x30
#define OFF_ITEM_DEF_RARITY 0x60
#define OFF_ITEM_DEF_ITEMTYPE 0xA0
#define OFF_ITEM_VT_GET_ITEM_DEF 0x20
#define OFF_ITEM_VT_GET_ITEM_ID 0x40
#define OFF_ITEM_HASHID	0x80				// hashId for CodedTextFromHashId
#define OFF_ITEM_UPG_HASHID 0xA8			// returns the suffix (i.e. "of Scholar")
#define OFF_ITEM_TYPE_HASHID 0x58			// return itemName (i.e. Staff/Sword/etc)

// SkinDef offset
#define OFF_SKIN_HASHID 0x58

// Stats Ptr offset
#define OFF_STAT_ID 0x28
#define OFF_STAT_DATA 0x40
#define OFF_STAT_HASHID 0x2c				// hashId for CodedTextFromHashId

// Item rarity constants
#define ITEM_RARITY_NONE 1					// white
#define ITEM_RARITY_FINE 2					// blue
#define ITEM_RARITY_MASTER 3				// green
#define ITEM_RARITY_RARE 4					// yellow
#define ITEM_RARITY_EXOTIC 5				// orange
#define ITEM_RARITY_ASCENDED 6				// pink
#define ITEM_RARITY_LEGENDARY 7				// purple

// Item type constants
#define ITEM_TYPE_ARMOR 0					// "skin->GetType() == Content::ITEM_TYPE_ARMOR"
#define ITEM_TYPE_WEAPON 0x12				// "itemDef->GetItemType() == Content::ITEM_TYPE_WEAPON"

// Weapon type constants
#define OFF_WEP_TYPE 0xC

// WorldView::CContext offsets
// void GetMetrics(int one, D3DXVECTOR3* camPos, D3DXVECTOR3* lookAt, D3DXVECTOR3* upVec, float* fov);
// search for "IsCameraAvailable()"
#define OFF_WVCTX_VT_GETMETRICS 0x78
#define OFF_WVCTX_STATUS 0x58

#define OFF_CAM_CUR_ZOOM 0x48
#define OFF_CAM_MIN_ZOOM 0x38
#define OFF_CAM_MAX_ZOOM 0x3c


//
// Contact ctx, GetContact(ctx, pptr)
// Search for "profession < arrsize(m_professionLookup)"
// 1st call from start of function is GetContactCtx()
// 2nd call from start of function is the GetContact() offset
//00007FF6591B20F0 | 48 89 5C 24 10			| mov qword ptr ss : [rsp + 10], rbx	|
//00007FF6591B20F5 | 56 | push rsi			|										|
//00007FF6591B20F6 | 48 83 EC 20			| sub rsp, 20							|
//00007FF6591B20FA | 48 8B DA				| mov rbx, rdx							|
//00007FF6591B20FD | E8 EE C3 1B 00			| call gw2 - 64.7FF65936E4F0			|	<== GetContactCtx()
//00007FF6591B2102 | 48 8B D3				| mov rdx, rbx							|
//00007FF6591B2105 | 4C 8B 00				| mov r8, qword ptr ds : [rax]			|
//00007FF6591B2108 | 48 8B C8				| mov rcx, rax							|
//00007FF6591B210B | 41 FF 90 38 02 00 00	| call qword ptr ds : [r8 + 238]		|	<== OFFSET
#define OFF_CONTACT_CTX_GET_CONTACT 0x238
#define OFF_CONTACT_UUID 0x18
#define OFF_CONTACT_ISPARTYSQUAD 0x84
