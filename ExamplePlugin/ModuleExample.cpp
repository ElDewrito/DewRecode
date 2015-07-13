#include "ModuleExample.hpp"

bool CommandExampleTest(const std::vector<std::string>& Arguments, std::string& returnInfo)
{
	returnInfo = "Test command: passed!";
	return true;
}

namespace Modules
{
	ModuleExample::ModuleExample() : ModuleBase("Example")
	{
		AddCommand("Test", "test_cmd", "Prints something", eCommandFlagsNone, CommandExampleTest);
	}
}
