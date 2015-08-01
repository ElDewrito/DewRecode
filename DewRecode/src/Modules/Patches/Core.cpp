#include "Core.hpp"
#include "../../ElDorito.hpp"
namespace
{
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

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + objectIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)objectAddress;
		uint32_t index = *(uint32_t*)objectDataAddress;

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

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + playerObjectIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)(objectAddress);

		return *(uint8_t*)(objectDataAddress + 0x320 + equipmentIndex);
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
	
	__declspec(naked) void LocalPlayerDamageHook()
	{
		__asm
		{
			; descope when taking damage
			push	eax
			mov     eax, dword ptr fs:[02Ch]			; get tls array address
			mov		eax, dword ptr ds:[eax]				; get slot 0 tls address
			mov		eax, dword ptr ds:[eax + 0C4h]		; get player control globals address
			mov		word ptr ds:[eax + 32Ah], 0FFFFh	; reset local player 0 scope level
			pop		eax

			; decrease player object damage
			movss	dword ptr ds:[edi + 100h], xmm1
			push	0B553ABh
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
			Hook("LocalPlayerDamageHook", 0xB553A3, LocalPlayerDamageHook, HookType::Jmp)
		});
	}
}
