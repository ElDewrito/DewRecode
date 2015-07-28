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

		// disable equipment when dual wielding
		if (*(uint8_t*)(0x244D33D) == 0)
			return 0;

		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];
		uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + playerObjectIndex * 0x10;
		uint32_t objectDataAddress = *(uint32_t*)(objectAddress);

		return *(uint8_t*)(objectDataAddress + 0x320 + equipmentIndex);
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
			Hook("GetEquipmentCountHook", 0xB440F0, GetEquipmentCountHook, HookType::Jmp)
		});
	}
}
