#pragma once
#include "ModuleGame.hpp"
#include "Patches/PatchModuleNetwork.hpp"
#include "Patches/PatchModuleUI.hpp"

namespace Modules
{
	class ModuleMain : public ModuleBase
	{
	public:
		ModuleGame Game;
		PatchModuleNetwork NetworkPatches;
		PatchModuleUI UIPatches;

		ModuleMain();
	};
}
