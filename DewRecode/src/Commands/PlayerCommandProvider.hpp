#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>
#include "../Patches/PlayerPatchProvider.hpp"

namespace Armor
{
	class ArmorPatchProvider;
}

namespace Player
{
	class PlayerCommandProvider : public CommandProvider
	{
	private:
		std::shared_ptr<Armor::ArmorPatchProvider> armorPatches;
		std::shared_ptr<PlayerPatchProvider> playerPatches;

	public:
		Command* VarArmorAccessory;
		Command* VarArmorArms;
		Command* VarArmorChest;
		Command* VarArmorHelmet;
		Command* VarArmorLegs;
		Command* VarArmorPelvis;
		Command* VarArmorShoulders;

		Command* VarColorsPrimary;
		Command* VarColorsSecondary;
		Command* VarColorsVisor;
		Command* VarColorsLights;
		Command* VarColorsHolo;

		Command* VarName;
		Command* VarPrivKey;
		Command* VarPubKey;

		wchar_t UserName[17];

		explicit PlayerCommandProvider(std::shared_ptr<Armor::ArmorPatchProvider> armorPatches, std::shared_ptr<PlayerPatchProvider> playerPatches);

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;

		bool CommandPrintUid(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableArmorUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableNameUpdate(const std::vector<std::string>& Arguments, CommandContext& context);

		std::string GetFormattedPrivKey();
	};
}