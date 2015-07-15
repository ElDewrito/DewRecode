#pragma once
#include "ModuleCamera.hpp"
#include "ModuleGame.hpp"
#include "ModuleGraphics.hpp"
#include "ModuleInput.hpp"
#include "ModulePlayer.hpp"
#include "ModuleServer.hpp"
#include "ModuleTime.hpp"

#include "Patches/Armor.hpp"
#include "Patches/ContentItems.hpp"
#include "Patches/Core.hpp"
#include "Patches/Input.hpp"
#include "Patches/Network.hpp"
#include "Patches/PlayerUid.hpp"
#include "Patches/Scoreboard.hpp"
#include "Patches/UI.hpp"
#include "Patches/VirtualKeyboard.hpp"

namespace Modules
{
	class ModuleMain : public ModuleBase
	{
	public:
		ModuleCamera Camera;
		ModuleGame Game;
		ModuleGraphics Graphics;
		ModuleInput Input;
		ModulePlayer Player;
		ModuleServer Server;
		ModuleTime Time;

		PatchModuleArmor ArmorPatches;
		PatchModuleContentItems ContentItemPatches;
		PatchModuleCore CorePatches;
		PatchModuleInput InputPatches;
		PatchModuleNetwork NetworkPatches;
		PatchModulePlayerUid PlayerUidPatches;
		PatchModuleScoreboard ScoreboardPatches;
		PatchModuleUI UIPatches;
		PatchModuleVirtualKeyboard VirtualKeyboardPatches;

		ModuleMain();
	};
}
