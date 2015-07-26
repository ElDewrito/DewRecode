#pragma once
#include "ModuleCamera.hpp"
#include "ModuleConsole.hpp"
#include "ModuleForge.hpp"
#include "ModuleGame.hpp"
#include "ModuleGraphics.hpp"
#include "ModuleInput.hpp"
#include "ModulePlayer.hpp"
#include "ModuleServer.hpp"
#include "ModuleTime.hpp"
#include "ModuleDebug.hpp"

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
		ModuleGame Game; // game has to be created before all others
		ModuleCamera Camera;
		ModuleConsole Console;
		ModuleForge Forge;
		ModuleGraphics Graphics;
		ModuleInput Input;
		ModulePlayer Player;
		ModuleServer Server;
		ModuleTime Time;
		ModuleDebug Debug;

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
