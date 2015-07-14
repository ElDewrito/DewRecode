#pragma once
#include "ModuleGame.hpp"
#include "ModulePlayer.hpp"
#include "Patches/PatchModuleArmor.hpp"
#include "Patches/PatchModuleContentItems.hpp"
#include "Patches/PatchModuleCore.hpp"
#include "Patches/PatchModuleNetwork.hpp"
#include "Patches/PatchModuleUI.hpp"

namespace Modules
{
	class ModuleMain : public ModuleBase
	{
	public:
		ModuleGame Game;
		ModulePlayer Player;

		PatchModuleArmor ArmorPatches;
		PatchModuleContentItems ContentItemPatches;
		PatchModuleCore CorePatches;
		PatchModuleNetwork NetworkPatches;
		PatchModuleUI UIPatches;

		ModuleMain();
	};
}
