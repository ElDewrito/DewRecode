#include "PlayerSettingsWindow.hpp"
#include "../../ElDorito.hpp"

namespace UI
{
	std::map<const char*, const char*> armors = {
		{ "Air Assault", "air_assault" },
		{ "Stealth", "stealth" },
		{ "Renegade", "renegade" },
		{ "Nihard", "nihard" },
		{ "Gladiator", "gladiator" },
		{ "Mac", "mac" },
		{ "Shark", "shark" },
		{ "Juggernaut", "juggernaut" },
		{ "Dutch", "dutch" },
		{ "Chameleon", "chameleon" },
		{ "Halberd", "halberd" },
		{ "Cyclops", "cyclops" },
		{ "Scanner", "scanner" },
		{ "Mercenary", "mercenary" },
		{ "Hoplite", "hoplite" },
		{ "Ballista", "ballista" },
		{ "Strider", "strider" },
		{ "Demo", "demo" },
		{ "Orbital", "orbital" },
		{ "Spectrum", "spectrum" },
		{ "Gungnir", "gungnir" },
		{ "Hammerhead", "hammerhead" },
		{ "Omni", "omni" },
		{ "Oracle", "oracle" },
		{ "Silverback", "silverback" },
		{ "Widow Maker", "widow_maker" }
	};

	PlayerSettingsWindow::PlayerSettingsWindow()
	{
	}

	void PlayerSettingsWindow::Draw()
	{
		if (!isVisible)
			return;

		//ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin("Player Settings", &isVisible))
		{
			ImGui::End();
			return;
		}

		ImGui::InputText("player name", username, IM_ARRAYSIZE(username));

		ImGui::Combo("helmet", &helmetCurrent, &armorNames[0], armorNames.size(), 4);
		ImGui::Combo("chest", &chestCurrent, &armorNames[0], armorNames.size(), 4);
		ImGui::Combo("shoulders", &shouldersCurrent, &armorNames[0], armorNames.size(), 4);
		ImGui::Combo("arms", &armsCurrent, &armorNames[0], armorNames.size(), 4);
		ImGui::Combo("legs", &legsCurrent, &armorNames[0], armorNames.size(), 4);
		ImGui::ColorEdit3("primary color", primary);
		ImGui::ColorEdit3("secondary color", secondary);
		ImGui::ColorEdit3("visor color", visor);
		ImGui::ColorEdit3("lights color", lights);
		ImGui::ColorEdit3("holo color", holo);

		if (ImGui::Button("Save"))
		{
			auto& dorito = ElDorito::Instance();

			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarName, std::string(username), std::string(), true);

			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarColorsPrimary, dorito.Utils.ColorToHex(primary), std::string(), true);
			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarColorsSecondary, dorito.Utils.ColorToHex(secondary), std::string(), true);
			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarColorsVisor, dorito.Utils.ColorToHex(visor), std::string(), true);
			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarColorsLights, dorito.Utils.ColorToHex(lights), std::string(), true);
			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarColorsHolo, dorito.Utils.ColorToHex(holo), std::string(), true);

			if (helmetCurrent > -1)
			{
				const char* helmet = armorNames[helmetCurrent];
				bool foundHelmet = false;
				for (auto kvp : armors)
				{
					if (kvp.first == std::string(helmet))
					{
						foundHelmet = true;
						helmet = kvp.second;
					}
				}
				if (foundHelmet)
					dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarArmorHelmet, std::string(helmet), std::string(), true);
			}

			if (chestCurrent > -1)
			{
				const char* value = armorNames[chestCurrent];
				const char* foundVal = nullptr;
				for (auto kvp : armors)
					if (kvp.first == std::string(value))
						foundVal = kvp.second;

				if (foundVal)
					dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarArmorChest, std::string(foundVal), std::string(), true);
			}

			if (shouldersCurrent > -1)
			{
				const char* value = armorNames[shouldersCurrent];
				const char* foundVal = nullptr;
				for (auto kvp : armors)
					if (kvp.first == std::string(value))
						foundVal = kvp.second;

				if (foundVal)
					dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarArmorShoulders, std::string(foundVal), std::string(), true);
			}

			if (armsCurrent > -1)
			{
				const char* value = armorNames[armsCurrent];
				const char* foundVal = nullptr;
				for (auto kvp : armors)
					if (kvp.first == std::string(value))
						foundVal = kvp.second;

				if (foundVal)
					dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarArmorArms, std::string(foundVal), std::string(), true);
			}

			if (legsCurrent > -1)
			{
				const char* value = armorNames[legsCurrent];
				const char* foundVal = nullptr;
				for (auto kvp : armors)
					if (kvp.first == std::string(value))
						foundVal = kvp.second;

				if (foundVal)
					dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarArmorLegs, std::string(foundVal), std::string(), true);
			}
		}

		ImGui::End();
	}

	void PlayerSettingsWindow::SetPlayerData() {
		auto& dorito = ElDorito::Instance();
		//Store username
		ZeroMemory(username, 256);
		strcpy_s(username, 256, dorito.PlayerCommands->VarName->ValueString.c_str());

		//Store armor information
		for (auto kvp = armors.begin(); kvp != armors.end(); ++kvp) {
			armorNames.push_back(kvp->first);

			if (!dorito.PlayerCommands->VarArmorHelmet->ValueString.compare(kvp->second))
				helmetCurrent = std::distance(armors.begin(), kvp);

			if (!dorito.PlayerCommands->VarArmorChest->ValueString.compare(kvp->second))
				chestCurrent = std::distance(armors.begin(), kvp);

			if (!dorito.PlayerCommands->VarArmorShoulders->ValueString.compare(kvp->second))
				shouldersCurrent = std::distance(armors.begin(), kvp);

			if (!dorito.PlayerCommands->VarArmorArms->ValueString.compare(kvp->second))
				armsCurrent = std::distance(armors.begin(), kvp);

			if (!dorito.PlayerCommands->VarArmorLegs->ValueString.compare(kvp->second))
				legsCurrent = std::distance(armors.begin(), kvp);
		}

		//Armor color information
		primary = dorito.Utils.HexToColor(dorito.PlayerCommands->VarColorsPrimary->ValueString);
		secondary = dorito.Utils.HexToColor(dorito.PlayerCommands->VarColorsSecondary->ValueString);
		visor = dorito.Utils.HexToColor(dorito.PlayerCommands->VarColorsVisor->ValueString);
		lights = dorito.Utils.HexToColor(dorito.PlayerCommands->VarColorsLights->ValueString);
		holo = dorito.Utils.HexToColor(dorito.PlayerCommands->VarColorsHolo->ValueString);
	}
}