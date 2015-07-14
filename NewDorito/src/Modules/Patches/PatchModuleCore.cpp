#include "PatchModuleCore.hpp"

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
		}));

		/*
		// Level load patch
		Patch::NopFill(Pointer::Base(0x2D26DF), 5);
		*/
	}
}
