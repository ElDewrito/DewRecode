#include "Core.hpp"
#include "../../ElDorito.hpp"
#include <ElDorito/Blam/Tags/Scenario.hpp>

namespace
{
	// TODO: refactor most of the functions below elsewhere, properly interfacing with the game's memory structures
	// TODO: fine-tune the prolog/epilog stuff to be used to wrap compiled function calls in inlined hooks so you don't have to worry about xmm registers getting clobbered etc.

	//__declspec(naked) void DirtyHookProlog()
	//{
	//	__asm
	//	{
	//		pushad
	//		pushf

	//		// save xmm
	//		sub		esp, 4 * 8
	//		movd	[esp + 4 * 0], xmm0
	//		movd	[esp + 4 * 1], xmm1
	//		movd	[esp + 4 * 2], xmm2
	//		movd	[esp + 4 * 3], xmm3
	//		movd	[esp + 4 * 4], xmm4
	//		movd	[esp + 4 * 5], xmm5
	//		movd	[esp + 4 * 6], xmm6
	//		movd	[esp + 4 * 7], xmm7

	//		// save fpu
	//		sub		esp, 108
	//		fnsave	[esp]
	//	}
	//}

	//__declspec(naked) void DirtyHookEpilog()
	//{
	//	__asm
	//	{
	//		// restore fpu
	//		frstor	[esp]
	//		add		esp, 108

	//		// restore xmm
	//		movd	xmm0, [esp + 4 * 0]
	//		movd	xmm1, [esp + 4 * 1]
	//		movd	xmm2, [esp + 4 * 2]
	//		movd	xmm3, [esp + 4 * 3]
	//		movd	xmm4, [esp + 4 * 4]
	//		movd	xmm5, [esp + 4 * 5]
	//		movd	xmm6, [esp + 4 * 6]
	//		movd	xmm7, [esp + 4 * 7]
	//		add		esp, 4 * 8

	//		popf
	//		popad
	//	}
	//}

	void DescopeLocalPlayer()
	{
		Pointer &playerControls = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		playerControls(0x32A).Write<int16_t>(-1);
	}

	uint32_t GetLocalPlayerObjectDatum()
	{
		Pointer &localPlayers = ElDorito::Instance().Engine.GetMainTls(GameGlobals::LocalPlayers::TLSOffset)[0];
		return *(uint32_t*)(localPlayers + GameGlobals::LocalPlayers::Player0ObjectDatumIdx);
	}

	uint32_t GetObjectDataAddress(uint32_t objectDatum)
	{
		uint32_t objectIndex = objectDatum & UINT16_MAX;
		Pointer &objectHeaderPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + objectIndex * 0x10;
		return *(uint32_t*)(objectAddress);
	}

	uint32_t GetLocalPlayerObjectDataAddress()
	{
		return GetObjectDataAddress(GetLocalPlayerObjectDatum());
	}

	__declspec(naked) void FovHook()
	{
		__asm
		{
			// Override the FOV that the memmove before this sets
			mov eax, ds:[0x189D42C]
			mov ds : [0x2301D98], eax
			mov ecx, [edi + 0x18]
			push 0x50CA08
			ret
		}
	}

	int __cdecl DualWieldHook(unsigned short objectIndex)
	{
		auto& dorito = ElDorito::Instance();

		uint32_t index = *(uint32_t*)GetObjectDataAddress(objectIndex);

		typedef char* (*GetTagAddressPtr)(int groupTag, uint32_t index);
		auto GetTagAddress = reinterpret_cast<GetTagAddressPtr>(0x503370);

		char* tagAddr = GetTagAddress(0x70616577, index);

		return ((*(uint32_t*)(tagAddr + 0x1D4) >> 22) & 1) == 1;
	}

	int __cdecl GetEquipmentCountHook(uint16_t playerObjectIndex, uint16_t equipmentIndex)
	{
		auto& dorito = ElDorito::Instance();

		if (equipmentIndex == 0xFFFF)
			return 0;

		// checks if dual wielding and disables equipment use if so
		if (*(uint8_t*)(0x244D33D) == 0)
			return 0;

		return *(uint8_t*)(GetObjectDataAddress(playerObjectIndex) + 0x320 + equipmentIndex);
	}

	__declspec(naked) void SprintInputHook()
	{
		__asm
		{
			mov		ecx, edi
			cmp		byte ptr ds:[0244D33Dh], 0	; zero if dual wielding
			jne		enable						; leave sprint enabled (for now) if not dual wielding
			and		ax, 0FEFFh					; disable by removing the 8th bit indicating no sprint input press
			enable:
			mov		dword ptr ds:[esi + 8], eax
			push	046DFC0h
			ret
		}
	}

	// scope level is an int16 with -1 indicating no scope, 0 indicating first level, 1 indicating second level etc.
	__declspec(naked) void ScopeLevelHook()
	{
		__asm
		{
			mov		word ptr ds:[edi + esi + 32Ah], 0FFFFh	; no scope by default
			cmp		byte ptr ds:[0244D33Dh], 0				; zero if dual wielding
			je		noscope									; prevent scoping when dual wielding
			mov		word ptr ds:[edi + esi + 32Ah], ax		; otherwise use intended scope level
			noscope:
			push	05D50D3h
			ret
		}
	}
	
	// hook @ 0xB553A0
	__declspec(naked) void HostObjectHealthHook()
	{
		__asm
		{
			pushad

			; get tls info
			mov		eax, dword ptr fs:[02Ch]	; tls array address
			mov		eax, dword ptr ds:[eax]		; slot 0 tls address

			; get local player object offset
			mov		ebx, dword ptr ds:[eax + 05Ch]	; local player mappings
			mov		ebx, dword ptr ds:[ebx + 014h]	; local player datum
			and		ebx, 0FFFFh						; local player index
			shl		ebx, 4							; multiply by object entry size of 16 bytes
			add		ebx, 0Ch						; add object header size

			; get local player object data address
			mov		ecx, dword ptr ds:[eax + 0448h]	; object header address
			mov		ecx, dword ptr ds:[ecx + 044h]	; first object address
			add		ecx, ebx						; local player object address
			mov		ecx, [ecx]						; local player object data address

			; check if damaging local player object and descope if so
			cmp		edi, ecx
			jne		orig
		
			; descope local player
			mov		edx, dword ptr ds:[eax + 0C4h]		; player control globals
			mov		word ptr ds:[edx + 032Ah], 0FFFFh	; descope

			orig:
			popad
			ucomiss	xmm1, xmm0
			movss   dword ptr ds:[edi + 100h], xmm1
			push	0B553ABh
			ret
		}
	}

	// hook @ 0xB54B4E
	__declspec(naked) void HostObjectShieldHook()
	{
		__asm
		{
			pushad

			; get tls info
			mov		eax, dword ptr fs:[02Ch]	; tls array address
			mov		eax, dword ptr ds:[eax]		; slot 0 tls address

			; get local player object offset
			mov		ebx, dword ptr ds:[eax + 05Ch]	; local player mappings
			mov		ebx, dword ptr ds:[ebx + 014h]	; local player datum
			and		ebx, 0FFFFh						; local player index
			shl		ebx, 4							; multiply by object entry size of 16 bytes
			add		ebx, 0Ch						; add object header size

			; get local player object data address
			mov		ecx, dword ptr ds:[eax + 0448h]	; object header address
			mov		ecx, dword ptr ds:[ecx + 044h]	; first object address
			add		ecx, ebx						; local player object address
			mov		ecx, [ecx]						; local player object data address

			; check if damaging local player object and descope if so
			cmp		edi, ecx
			jne		orig

			; descope local player
			mov		edx, dword ptr ds:[eax + 0C4h]		; player control globals
			mov		word ptr ds:[edx + 032Ah], 0FFFFh	; descope

			orig:
			popad
			movss   dword ptr ds:[edi + 0FCh], xmm1
			push	0B54B56h
			ret
		}
	}

	// hook @ 0xB33F13
	__declspec(naked) void ClientObjectHealthHook()
	{
		__asm
		{
			pushad

			; get tls info
			mov		eax, dword ptr fs:[02Ch]	; tls array address
			mov		eax, dword ptr ds:[eax]		; slot 0 tls address

			; get local player object offset
			mov		ebx, dword ptr ds:[eax + 05Ch]	; local player mappings
			mov		ebx, dword ptr ds:[ebx + 014h]	; local player datum
			and		ebx, 0FFFFh						; local player index
			shl		ebx, 4							; multiply by object entry size of 16 bytes
			add		ebx, 0Ch						; add object header size

			; get local player object data address
			mov		ecx, dword ptr ds:[eax + 0448h]	; object header address
			mov		ecx, dword ptr ds:[ecx + 044h]	; first object address
			add		ecx, ebx						; local player object address
			mov		ecx, [ecx]						; local player object data address

			; check if damaging local player object
			cmp		edi, ecx
			jne		orig

			; only descope if shield is decreasing at a rate greater than epsilon
			mov		esi, 03C23D70Ah						; use an epsilon of 0.01f
			movd	xmm7, esi
			movss	xmm6, dword ptr ds:[edi + 100h]		; get original health
			subss	xmm6, xmm0							; get negative health delta (orig - new)
			comiss	xmm6, xmm7							; compare health delta with epsilon
			jb		orig								; skip descope if delta is less than epsilon

			; descope local player
			mov		edx, dword ptr ds:[eax + 0C4h]		; player control globals
			mov		word ptr ds:[edx + 032Ah], 0FFFFh	; descope

			orig:
			popad
			movss   dword ptr ds:[edi + 100h], xmm0
			push	0B33F1Bh
			ret
		}
	}

	// hook @ 0xB329CE
	__declspec(naked) void ClientObjectShieldHook()
	{
		__asm
		{
			pushad

			; get tls info
			mov		eax, dword ptr fs:[02Ch]	; tls array address
			mov		eax, dword ptr ds:[eax]		; slot 0 tls address

			; get local player object offset
			mov		ebx, dword ptr ds:[eax + 05Ch]	; local player mappings
			mov		ebx, dword ptr ds:[ebx + 014h]	; local player datum
			and		ebx, 0FFFFh						; local player index
			shl		ebx, 4							; multiply by object entry size of 16 bytes
			add		ebx, 0Ch						; add object header size

			; get local player object data address
			mov		edx, dword ptr ds:[eax + 0448h]	; object header address
			mov		edx, dword ptr ds:[edx + 044h]	; first object address
			add		edx, ebx						; local player object address
			mov		edx, [edx]						; local player object data address

			; check if damaging local player object
			cmp		ecx, edx
			jne		orig

			; only descope if shield is decreasing at a rate greater than epsilon
			mov		esi, 03C23D70Ah						; use an epsilon of 0.01f
			movd	xmm7, esi
			movss	xmm6, dword ptr ds:[ecx + 0FCh]		; get original shield
			subss	xmm6, xmm0							; get negative shield delta (orig - new)
			comiss	xmm6, xmm7							; compare shield delta with epsilon
			jb		orig								; skip descope if delta is less than epsilon

			; descope local player
			mov		edx, dword ptr ds:[eax + 0C4h]		; player control globals
			mov		word ptr ds:[edx + 032Ah], 0FFFFh	; descope

			orig:
			popad
			movss   dword ptr ds:[ecx + 0FCh], xmm0
			push	0B329D6h
			ret
		}
	}

	void GrenadeLoadoutHookImpl(uint8_t* unit)
	{
		// Based off of 0x8227B48C in H3 non-TU

		// TODO: Clean this up, hardcoded offsets are hacky
		const size_t GrenadeCountOffset = 0x320;
		const size_t ControllingPlayerOffset = 0x198;
		auto grenadeCounts = unit + GrenadeCountOffset; // 0 = frag, 1 = plasma, 2 = spike, 3 = firebomb
		auto playerIndex = *reinterpret_cast<int16_t*>(unit + ControllingPlayerOffset);
		if (playerIndex < 0)
		{
			memset(grenadeCounts, 0, 4);
			return;
		}

		// Get the player's grenade setting (haxhaxhax)
		const size_t DatumArrayPtrOffset = 0x44;
		const size_t PlayerSize = 0x2F08;
		const size_t GrenadeSettingOffset = 0x2DB4;
		auto grenadeSettingPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0][DatumArrayPtrOffset](PlayerSize * playerIndex + GrenadeSettingOffset);
		auto grenadeSetting = grenadeSettingPtr.Read<int16_t>();

		// Get the current scenario tag
		auto scenario = Blam::Tags::GetCurrentScenario();

		// If the setting is none (2) or the scenario has invalid starting
		// profile data, set the grenade counts to 0 and return
		if (grenadeSetting == 2 || !scenario->StartingProfile)
		{
			memset(grenadeCounts, 0, 4);
			return;
		}

		// Load the grenade counts from the scenario tag
		auto profile = &scenario->StartingProfile[0];
		grenadeCounts[0] = profile->FragGrenades;
		grenadeCounts[1] = profile->PlasmaGrenades;
		grenadeCounts[2] = profile->SpikeGrenades;
		grenadeCounts[3] = profile->FirebombGrenades;
	}

	__declspec(naked) void GrenadeLoadoutHook()
	{
		__asm
		{
			push edi // Unit object data
			call GrenadeLoadoutHookImpl
			add esp, 4
			push 0x5A32C7
			ret
		}
	}

	void EquipmentHookImpl(unsigned short playerIndex, unsigned short equipmentIndex)
	{
		BYTE unkData[0x40];
		BYTE b69Data[0x48];

		auto& dorito = ElDorito::Instance();

		Pointer& playerHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];
		uint32_t playerStructAddress = playerHeaderPtr(0x44).Read<uint32_t>() + playerIndex * GameGlobals::Players::PlayerEntryLength;
		uint32_t playerObjectDatum = *(uint32_t*)(playerStructAddress + 0x30);

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + equipmentIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)objectAddress;
		uint32_t index = *(uint32_t*)objectDataAddress;

		memset(b69Data, 0, 0x48);
		*(uint32_t*)(b69Data) = 0x3D; // object type?
		*(uint32_t*)(b69Data + 4) = equipmentIndex;

		// add equipment to the player, also removes the object from gameworld
		typedef char(__cdecl* Objects_AttachPtr)(int a1, void* a2);
		auto Objects_Attach = reinterpret_cast<Objects_AttachPtr>(0xB69C50);
		Objects_Attach(playerObjectDatum, b69Data); // 82182C48

		// prints text to the HUD, taken from HO's code
		{
			typedef int(__thiscall* sub_589680Ptr)(void* thisPtr, int a2);
			auto sub_589680 = reinterpret_cast<sub_589680Ptr>(0x589680);
			sub_589680(&unkData, playerIndex);

			typedef BOOL(__thiscall* sub_589770Ptr)(void* thisPtr);
			auto sub_589770 = reinterpret_cast<sub_589770Ptr>(0x589770);
			while ((unsigned __int8)sub_589770(&unkData))
			{
				typedef int(__thiscall* sub_589760Ptr)(void* thisPtr);
				auto sub_589760 = reinterpret_cast<sub_589760Ptr>(0x589760);
				int v9 = sub_589760(&unkData);

				typedef int(__cdecl* sub_A95850Ptr)(unsigned int a1, short a2);
				auto sub_A95850 = reinterpret_cast<sub_A95850Ptr>(0xA95850);
				sub_A95850(v9, index);
			}
		}

		// unsure what these do, taken from HO code
		{
			typedef int(__cdecl *sub_B887B0Ptr)(unsigned short a1, unsigned short a2);
			auto sub_B887B0 = reinterpret_cast<sub_B887B0Ptr>(0xB887B0);
			sub_B887B0(playerIndex, index); // sub_82437A08

			typedef void(_cdecl *sub_4B31C0Ptr)(unsigned short a1, unsigned short a2);
			auto sub_4B31C0 = reinterpret_cast<sub_4B31C0Ptr>(0x4B31C0);
			sub_4B31C0(playerObjectDatum, index); // sub_8249A1A0
		}

		// called by powerup pickup func, deletes the item but also crashes the game when used with equipment
		// not needed since Objects_Attach removes it from the game world
		/*
		typedef int(__cdecl *Objects_DeletePtr)(int objectIndex);
		auto Objects_Delete = reinterpret_cast<Objects_DeletePtr>(0xB57090);
		Objects_Delete(equipmentIndex);*/

	}

	__declspec(naked) void EquipmentHook()
	{
		__asm
		{
			mov edx, 0x531D70
			call edx
			mov esi, [ebp+8]
			push edi
			push esi
			test al, al
			jz DoEquipmentHook
			mov edx, 0x4B4A20
			call edx
			add esp, 8
			push 0x5397D8
			ret
		DoEquipmentHook:
			call EquipmentHookImpl
			add esp, 8
			push 0x5397D8
			ret
		}
	}
}
namespace Modules
{
	PatchModuleCore::PatchModuleCore() : ModuleBase("Patches.Core")
	{
		AddModulePatches(
		{
			// Enable tag edits
			Patch("TagEdits1", 0x501A5B, { 0xEB }),
			Patch("TagEdits2", 0x502874, 0x90, 2),
			Patch("TagEdits3", 0x5030AA, 0x90, 2),

			// No --account args patch
			Patch("AccountArgs1", 0x83731A, { 0xEB, 0x0E }),
			Patch("AccountArgs2", 0x8373AD, { 0xEB, 0x03 }),

			// Prevent game variant weapons from being overridden
			Patch("VariantOverride1", 0x5A315F, { 0xEB }),
			Patch("VariantOverride2", 0x5A31A4, { 0xEB }),

			// Level load patch (?)
			Patch("LevelLoad", 0x6D26DF, 0x90, 5),

			// Adds the FMOD WASAPI output fix from FMODEx 4.44.56, which stops weird popping sound at startup
			// TODO: maybe find a way to update HO's FMOD, HO is using 4.26.6 which is ancient
			Patch("FmodWasapiFix", 0x140DA75, { 0x2 }),

			// skips some check and allows you to throw equipment
			// not exactly sure what the skipped check is checking
			// my guess is it checks the loadout for the currently equipped equipment, if it's not in the loadout then it returns false
			Patch("EquipmentThrowHookFix", 0xB86CFF, 0x90, 6)

			/* TODO: find out if one of these patches is breaking game prefs
			// Remove preferences.dat hash check
			Patch("PrefsDatHashCheck", 0x50C99A, 0x90, 6),

			// Patch to allow spawning AI through effects
			Patch("SpawnAIWithEffects", 0x1433321, 0x90, 2),

			// Prevent FOV from being overridden when the game loads
			Patch("FOVOverride1", 0x65FA79, 0x90, 10),
			Patch("FOVOverride2", 0x65FA86, 0x90, 5)*/
		},
		{
			/*Hook("FOVHook", 0x50CA02, FovHook, HookType::Jmp)*/

			Hook("DualWieldHook", 0xB61550, DualWieldHook, HookType::Jmp),
			Hook("GetEquipmentCountHook", 0xB440F0, GetEquipmentCountHook, HookType::Jmp),
			Hook("SprintInputHook", 0x46DFBB, SprintInputHook, HookType::Jmp),
			Hook("ScopeLevelHook", 0x5D50CB, ScopeLevelHook, HookType::Jmp),

			Hook("HostObjectHealthHook", 0xB553A0, HostObjectHealthHook, HookType::Jmp),
			Hook("HostObjectShieldHook", 0xB54B4E, HostObjectShieldHook, HookType::Jmp),
			Hook("ClientObjectHealthHook", 0xB33F13, ClientObjectHealthHook, HookType::Jmp),
			Hook("ClientObjectShieldHook", 0xB329CE, ClientObjectShieldHook, HookType::Jmp),

			Hook("GrenadeLoadoutHook", 0x5A3267, GrenadeLoadoutHook, HookType::Jmp),
			Hook("EquipmentHook", 0x539888, EquipmentHook, HookType::JmpIfNotEqual)
		});
	}
}
