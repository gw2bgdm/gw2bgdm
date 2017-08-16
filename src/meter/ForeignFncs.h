#pragma once
#include <stdint.h>
#pragma warning (push)
#pragma warning (disable: 4201)
#include "dxsdk/d3dx9math.h"
#pragma warning (pop)

/***		GW2 hacklib excerpt start			***/
/*** https://bitbucket.org/rafzi/hacklib_gw2	***/
#ifdef __cplusplus
#include <type_traits> // std::conditional
#pragma warning( push )  
#pragma warning( disable : 4127 )  
namespace ANet
{
	template <typename T>
	struct Array {
		T *m_basePtr;
		uint32_t m_capacity;
		uint32_t m_count;
	};

	template <typename T>
	struct Dictionary {
		uint32_t m_capacity;
		uint32_t m_count;
		T *m_basePtr;
	};

	template <typename T, bool IsArray = true>
	class Collection : private std::conditional<IsArray, Array<T>, Dictionary<T>>::type {
	public:
		Collection<T> &operator= (const Collection<T> &a) {
			if (this != &a) {
				m_basePtr = a.m_basePtr;
				m_capacity = a.m_capacity;
				m_count = a.m_count;
			}
			return *this;
		}
		T &operator[] (uint32_t index) {
			if (IsArray && index < Count()) {
				return m_basePtr[index];
			}
			else if (index < Capacity()) {
				return m_basePtr[index];
			}
			throw STATUS_ARRAY_BOUNDS_EXCEEDED;
		}
		bool IsValid() {
			return !!m_basePtr;
		}
		uint32_t Count() {
			return m_count;
		}
		uint32_t Capacity() {
			return m_capacity;
		}
		uintptr_t Data() {
			return (uintptr_t)m_basePtr;
		}
	};
};
#pragma warning( pop )  
#endif	// __cplusplus
/*** GW2 hacklib exceprt end ***/

#ifdef __cplusplus
extern "C" {
#endif

#define BGDM_EXCEPTION(msg) ExceptHandler(msg, GetExceptionCode(), GetExceptionInformation(), __FILE__, __FUNCTION__, __LINE__)
DWORD ExceptHandler(const char *msg, DWORD code, EXCEPTION_POINTERS *ep, const char *file, const char *func, int line);

struct Array;
typedef struct __Spec Spec;
typedef struct __EquipItems EquipItems;
typedef void(__fastcall *cbDecodeText_t)(uint8_t*, wchar_t*);

uint8_t* CodedTextFromHashId(uint32_t hashId, uint32_t a2);
bool DecodeText(uint8_t* codedText, cbDecodeText_t cbDecodeText, uintptr_t ctx);
uint32_t HashIdFromPtr(int type, uintptr_t ptr);

bool WV_GetMetrics(uintptr_t wvctxptr, int one, D3DXVECTOR3* camPos, D3DXVECTOR3* lookAt, D3DXVECTOR3* upVec, float* fovy);
bool Ag_GetPos(uintptr_t aptr, D3DXVECTOR4* outPos);
uintptr_t Ag_GetWmAgemt(uintptr_t aptr);
bool WmAgent_GetCodedName(uintptr_t aptr, cbDecodeText_t cbDecodeText, uintptr_t ctx);
uintptr_t Ch_GetCombatant(uintptr_t cptr);
bool Ch_GetSpeciesDef(uintptr_t cptr, uintptr_t *pSpeciesDef);
int  Ch_GetProfession(uintptr_t cptr);
uintptr_t Ag_GetChar(uintptr_t aptr, struct Array* ca);
bool Ag_Validate(uint32_t id, uintptr_t aptr, uintptr_t wmptr);
bool Ch_Validate(uintptr_t cptr, struct Array* ca);
uint32_t Ch_GetPlayerId(uintptr_t cptr);
bool Ch_IsPlayer(uintptr_t cptr);
bool Ch_IsAlive(uintptr_t cptr);
bool Ch_IsDowned(uintptr_t cptr);
bool Ch_IsClone(uintptr_t cptr);
bool Pl_HasEliteSpec(uintptr_t pptr);
bool Pl_GetSpec(uintptr_t pptr, Spec *outSpec);
bool Ch_GetInventory(uintptr_t cptr, EquipItems *equipItems);
uint32_t EqItem_GetUpgrades(uintptr_t eqptr, struct Array *outArr);
uintptr_t GetCtxContact(uintptr_t fncGetCtxContact);
uintptr_t GetCtxSquad(uintptr_t fncGetCtxContact);
uintptr_t CtxContact_GetContact(uintptr_t pctx, uintptr_t pptr);
uintptr_t CtxContact_GetContactSelf(uintptr_t pctx);
const wchar_t * Contact_GetAccountName(uintptr_t ctptr);
uint32_t Pl_GetSquadSubgroup(uintptr_t squadptr, uintptr_t pctx);
bool Pl_IsInPartyOrSquad(uintptr_t pptr, uintptr_t pctx, uintptr_t fncGetCtxContact);

uintptr_t TR_GetHPBarPtr();
bool TR_SetHPChars(int mode);
bool TR_SetHPCharsBright(int mode);

#ifdef __cplusplus
}
#endif