#include "PlayerCommandProvider.hpp"
#include "../ElDorito.hpp"
#include "../Patches/ArmorPatchProvider.hpp"
#include "../Patches/PlayerPatchProvider.hpp"

#include <ElDorito/Blam/BlamNetwork.hpp>

namespace Player
{
	PlayerCommandProvider::PlayerCommandProvider(std::shared_ptr<Armor::ArmorPatchProvider> armorPatches, std::shared_ptr<PlayerPatchProvider> playerPatches)
	{
		this->armorPatches = armorPatches;
		this->playerPatches = playerPatches;
	}

	std::vector<Command> PlayerCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Player", "PrintUid", "uid", "Prints the players UID", eCommandFlagsNone, BIND_COMMAND(this, &PlayerCommandProvider::CommandPrintUid))
		};

		return commands;
	}

	void PlayerCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarArmorAccessory = manager->Add(Command::CreateVariableString("Player", "Armor.Accessory", "armor_accessory", "Armor ID for player accessory", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorArms = manager->Add(Command::CreateVariableString("Player", "Armor.Arms", "armor_arms", "Armor ID for player arms", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorChest = manager->Add(Command::CreateVariableString("Player", "Armor.Chest", "armor_chest", "Armor ID for player chest", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorHelmet = manager->Add(Command::CreateVariableString("Player", "Armor.Helmet", "armor_helmet", "Armor ID for player helmet", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorLegs = manager->Add(Command::CreateVariableString("Player", "Armor.Legs", "armor_legs", "Armor ID for player legs", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorPelvis = manager->Add(Command::CreateVariableString("Player", "Armor.Pelvis", "armor_pelvis", "Armor ID for player pelvis", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarArmorShoulders = manager->Add(Command::CreateVariableString("Player", "Armor.Shoulders", "armor_shoulders", "Armor ID for player shoulders", eCommandFlagsArchived, "", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));

		VarColorsPrimary = manager->Add(Command::CreateVariableString("Player", "Colors.Primary", "colors_primary", "The primary colors hex value", eCommandFlagsArchived, "#000000", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarColorsSecondary = manager->Add(Command::CreateVariableString("Player", "Colors.Secondary", "colors_secondary", "The secondary colors hex value", eCommandFlagsArchived, "#000000", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarColorsVisor = manager->Add(Command::CreateVariableString("Player", "Colors.Visor", "colors_visor", "The visor colors hex value", eCommandFlagsArchived, "#000000", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarColorsLights = manager->Add(Command::CreateVariableString("Player", "Colors.Lights", "colors_lights", "The lights colors hex value", eCommandFlagsArchived, "#000000", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));
		VarColorsHolo = manager->Add(Command::CreateVariableString("Player", "Colors.Holo", "colors_holo", "The holo colors hex value", eCommandFlagsArchived, "#000000", BIND_COMMAND(this, &PlayerCommandProvider::VariableArmorUpdate)));

		VarName = manager->Add(Command::CreateVariableString("Player", "Name", "name", "The players ingame name", eCommandFlagsArchived, "Jasper", BIND_COMMAND(this, &PlayerCommandProvider::VariableNameUpdate)));
		// hack to add a small notice before Player.PrivKey in the cfg file
		manager->Add(Command::CreateVariableString("Player", "PrivKeyNote", "priv_key_note", "", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsHidden), "The PrivKey below is used to keep your stats safe. Treat it like a password and don't share it with anyone!"));
		VarPrivKey = manager->Add(Command::CreateVariableString("Player", "PrivKey", "player_privkey", "The players unique stats private key", static_cast<CommandFlags>(eCommandFlagsOmitValueInList | eCommandFlagsArchived | eCommandFlagsInternal), ""));
		VarPubKey = manager->Add(Command::CreateVariableString("Player", "PubKey", "player_pubkey", "The players unique stats public key", static_cast<CommandFlags>(eCommandFlagsOmitValueInList | eCommandFlagsArchived), ""));
		memset(this->UserName, 0, sizeof(wchar_t) * 17);

		char* defaultNames[49] = {
			"Donut", "Penguin", "Stumpy", "Whicker", "Shadow", "Howard", "Wilshire",
			"Darling", "Disco", "Jack", "The Bear", "Sneak", "The Big ", "Whisp",
			"Wheezy", "Crazy", "Goat", "Pirate", "Saucy", "Hambone", "Butcher",
			"Walla Walla", "Snake", "Caboose", "Sleepy", "Killer", "Stompy",
			"Mopey", "Dopey", "Wease", "Ghost", "Dasher", "Grumpy", "Hollywood",
			"Tooth", "Noodle", "King", "Cupid", "Prancer", "Pyong", "Jasper",
			"Fish", "Moose", "Banana", "Peanut", "Code", "Upvote", "Commit"
		};

		srand((unsigned int)time(0));
		manager->SetVariable(VarName, std::string(defaultNames[rand() % 41]), std::string());
	}

	bool PlayerCommandProvider::CommandPrintUid(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		bool uidIsValid = Pointer(0x19AB728).Read<bool>();
		if (!uidIsValid)
		{
			context.WriteOutput("0x0");
			return true;
		}

		uint64_t uid = Pointer(0x19AB730).Read<uint64_t>();
		std::string uidStr;
		ElDorito::Instance().Utils.BytesToHexString(&uid, sizeof(uint64_t), uidStr);
		context.WriteOutput("0x" + uidStr);
		return true;
	}

	bool PlayerCommandProvider::VariableArmorUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		armorPatches->SignalRefreshUIPlayerArmor();
		return true;
	}

	bool PlayerCommandProvider::VariableNameUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		auto* session = dorito.Engine.GetActiveNetworkSession();
		if (session && session->IsEstablished())
		{
			context.WriteOutput("You can only change your name when you're disconnected.");
			return false;
		}

		std::wstring nameStr = dorito.Utils.WidenString(VarName->ValueString);
		wcsncpy_s(UserName, nameStr.c_str(), 16);
		UserName[15] = 0;
		std::string actualName = dorito.Utils.ThinString(UserName);
		dorito.Engine.Event("Core", "Player.ChangeName", VarName);

		return true;
	}

	std::string PlayerCommandProvider::GetFormattedPrivKey()
	{
		playerPatches->EnsureValidUid();
		auto& dorito = ElDorito::Instance();
		return dorito.Utils.RSAReformatKey(dorito.PlayerCommands->VarPrivKey->ValueString, true);
	}
}