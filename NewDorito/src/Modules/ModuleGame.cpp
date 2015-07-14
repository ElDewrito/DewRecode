#include "ModuleGame.hpp"

namespace
{
	bool CommandGameTest(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		returnInfo = "Test command: passed!";
		return true;
	}
}

namespace Modules
{
	ModuleGame::ModuleGame() : ModuleBase("Game")
	{
		VarLanguageID = AddVariableInt("LanguageID", "languageid", "The language index to use", eCommandFlagsArchived, 0);
		VarLanguageID->ValueIntMin = 0;
		VarLanguageID->ValueIntMax = 11;

		VarSkipLauncher = AddVariableInt("SkipLauncher", "launcher", "Skip requiring the launcher", eCommandFlagsArchived, 0);
		VarSkipLauncher->ValueIntMin = 0;
		VarSkipLauncher->ValueIntMax = 1;

		AddCommand("Test", "test_cmd2", "Prints something", eCommandFlagsNone, CommandGameTest);
	}
}
