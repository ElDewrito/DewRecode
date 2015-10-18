#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../Patches/GameRulesPatchProvider.hpp"

namespace GameRules
{
	class GameRulesCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<GameRulesPatchProvider> gameRulesPatches;

		Command* VarGameSpeed;
		Command* VarSprintEnabled;
		Command* VarSprintUnlimited;

	public:
		explicit GameRulesCommandProvider(std::shared_ptr<GameRulesPatchProvider> gameRulesPatches);

		virtual void RegisterVariables(ICommandManager* manager) override;

		bool VariableGameSpeedUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableSprintEnabledUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableSprintUnlimitedUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
	};
}