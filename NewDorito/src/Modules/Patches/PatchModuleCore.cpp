#include "PatchModuleCore.hpp"
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
}
namespace Modules
{
	PatchModuleCore::PatchModuleCore() : ModuleBase("Patches.Core")
	{
		patches->TogglePatchSet(patches->AddPatchSet("Patches.Core",
		{
			// Enable tag edits
			Patch("TagEdits1", 0x501A5B, { 0xEB }),
			Patch("TagEdits2", 0x502874, true, 2),//{ 0x90, 0x90 }),
			Patch("TagEdits3", 0x5030AA, true, 2),//{ 0x90, 0x90 }),

			// No --account args patch
			Patch("AccountArgs1", 0x83731A, { 0xEB, 0x0E }),
			Patch("AccountArgs2", 0x8373AD, { 0xEB, 0x03 }),

			// Prevent game variant weapons from being overridden
			Patch("VariantOverride1", 0x5A315F, { 0xEB }),
			Patch("VariantOverride2", 0x5A31A4, { 0xEB }),

			// Level load patch (?)
			Patch("LevelLoad", 0x6D26DF, true, 5),

			/* TODO: find out if one of these patches is breaking game prefs
			// Remove preferences.dat hash check
			Patch("PrefsDatHashCheck", 0x50C99A, true, 6),

			// Patch to allow spawning AI through effects
			Patch("SpawnAIWithEffects", 0x1433321, true, 2),

			// Prevent FOV from being overridden when the game loads
			Patch("FOVOverride1", 0x65FA79, true, 10),
			Patch("FOVOverride2", 0x65FA86, true, 5)
		},
		{
			Hook("FOVHook", 0x50CA02, FovHook, HookType::Jmp)*/
		}));
	}
}
