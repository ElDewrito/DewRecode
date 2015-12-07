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

		std::vector<const char*> armorNames;
		for (auto kvp : armors)
			armorNames.push_back(kvp.first);

		static int helmetCurrent = 0;
		ImGui::Combo("helmet", &helmetCurrent, &armorNames[0], armorNames.size(), 4);
		static int chestCurrent = 0;
		ImGui::Combo("chest", &chestCurrent, &armorNames[0], armorNames.size(), 4);
		static int shouldersCurrent = 0;
		ImGui::Combo("shoulders", &shouldersCurrent, &armorNames[0], armorNames.size(), 4);
		static int armsCurrent = 0;
		ImGui::Combo("arms", &armsCurrent, &armorNames[0], armorNames.size(), 4);
		static int legsCurrent = 0;
		ImGui::Combo("legs", &legsCurrent, &armorNames[0], armorNames.size(), 4);
		static float primary[3] = { 1.0f, 0.0f, 0.2f };
		ImGui::ColorEdit3("primary color", primary);
		static float secondary[3] = { 1.0f, 0.0f, 0.2f };
		ImGui::ColorEdit3("secondary color", secondary);
		static float visor[3] = { 1.0f, 0.0f, 0.2f };
		ImGui::ColorEdit3("visor color", visor);
		static float lights[3] = { 1.0f, 0.0f, 0.2f };
		ImGui::ColorEdit3("lights color", lights);
		static float holo[3] = { 1.0f, 0.0f, 0.2f };
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
}