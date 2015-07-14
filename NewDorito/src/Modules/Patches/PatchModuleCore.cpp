#include "PatchModuleCore.hpp"

namespace Modules
{
	PatchModuleCore::PatchModuleCore() : ModuleBase("Patches.Core")
	{
		patches->TogglePatchSet(patches->AddPatchSet("Patches.Core",
		{
			// Enable tag edits
			{ "TagEdits1", 0x501A5B, { 0xEB }, {}, false },
			{ "TagEdits2", 0x502874, { 0x90, 0x90 }, {}, false },
			{ "TagEdits3", 0x5030AA, { 0x90, 0x90 }, {}, false },

			// No --account args patch
			{ "AccountArgs1", 0x83731A, { 0xEB, 0x0E }, {}, false },
			{ "AccountArgs2", 0x8373AD, { 0xEB, 0x03 }, {}, false },

			// Prevent game variant weapons from being overridden
			{ "VariantOverride1", 0x5A315F, { 0xEB }, {}, false },
			{ "VariantOverride2", 0x5A31A4, { 0xEB }, {}, false },
		}));
	}
}
