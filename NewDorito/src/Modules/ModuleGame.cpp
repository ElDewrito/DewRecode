#include "ModuleGame.hpp"

bool CommandGameTest(const std::vector<std::string>& Arguments, std::string& returnInfo)
{
	returnInfo = "Test command: passed!";
	return true;
}

namespace Modules
{
	ModuleGame::ModuleGame() : ModuleBase("Game")
	{
		AddCommand("Test", "test_cmd2", "Prints something", eCommandFlagsNone, CommandGameTest);
	}
}