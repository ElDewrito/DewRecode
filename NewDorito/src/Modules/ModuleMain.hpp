#pragma once
#include "ModuleGame.hpp"
#include "Patches/PatchModuleCore.hpp"
#include "Patches/PatchModuleNetwork.hpp"
#include "Patches/PatchModuleUI.hpp"

namespace Modules
{
	class ModuleMain : public ModuleBase
	{
	public:
		ModuleGame Game;
		PatchModuleCore CorePatches;
		PatchModuleNetwork NetworkPatches;
		PatchModuleUI UIPatches;

		ModuleMain();
	};
}
