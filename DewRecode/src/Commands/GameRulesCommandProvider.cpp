#include "GameRulesCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace GameRules
{
	GameRulesCommandProvider::GameRulesCommandProvider(std::shared_ptr<GameRulesPatchProvider> gameRulesPatches)
	{
		this->gameRulesPatches = gameRulesPatches;
	}

	void GameRulesCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarGameSpeed = manager->Add(Command::CreateVariableFloat("GameRules", "GameSpeed", "game_speed", "The game's speed", (CommandFlags)(eCommandFlagsCheat | eCommandFlagsReplicated | eCommandFlagsDontUpdateInitial), 1.0f, BIND_COMMAND(this, &GameRulesCommandProvider::VariableGameSpeedUpdate)));
		VarGameSpeed->ValueFloatMin = 0.0f;
		VarGameSpeed->ValueFloatMax = 10.0f;

		VarSprintEnabled = manager->Add(Command::CreateVariableInt("GameRules", "SprintEnabled", "sprint_enabled", "Controls whether sprint is enabled on the server", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 1, BIND_COMMAND(this, &GameRulesCommandProvider::VariableSprintEnabledUpdate)));
		VarSprintEnabled->ValueIntMin = 0;
		VarSprintEnabled->ValueIntMax = 1;

		VarSprintUnlimited = manager->Add(Command::CreateVariableInt("GameRules", "SprintUnlimited", "sprint_unlimited", "Controls whether unlimited sprint is enabled on the server", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 0, BIND_COMMAND(this, &GameRulesCommandProvider::VariableSprintUnlimitedUpdate)));
		VarSprintUnlimited->ValueIntMin = 0;
		VarSprintUnlimited->ValueIntMax = 1;
	}

	bool GameRulesCommandProvider::VariableGameSpeedUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		auto speed = VarGameSpeed->ValueFloat;
		Pointer &gameTimeGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Time::TLSOffset)[0];
		gameTimeGlobalsPtr(GameGlobals::Time::GameSpeedIndex).Write(speed);

		context.WriteOutput("Game speed set to " + std::to_string(speed));
		return true;
	}

	bool GameRulesCommandProvider::VariableSprintEnabledUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		this->gameRulesPatches->SprintEnable(VarSprintEnabled->ValueInt != 0);

		return true;
	}

	bool GameRulesCommandProvider::VariableSprintUnlimitedUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		this->gameRulesPatches->SprintUnlimited(VarSprintUnlimited->ValueInt != 0);

		return true;
	}
}