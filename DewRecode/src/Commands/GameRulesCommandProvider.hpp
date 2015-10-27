#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>
#include "../Patches/GameRulesPatchProvider.hpp"

namespace GameRules
{
	class GameRulesCommandProvider : public CommandProvider
	{
	private:
		std::shared_ptr<GameRulesPatchProvider> gameRulesPatches;

		Command* VarGameSpeed;
		Command* VarSprintEnabled;
		Command* VarSprintUnlimited;

	public:
		explicit GameRulesCommandProvider(std::shared_ptr<GameRulesPatchProvider> gameRulesPatches);

		virtual void RegisterVariables(ICommandManager* manager) override;

		bool VariableGameSpeedUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableSprintEnabledUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableSprintUnlimitedUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
	};
}