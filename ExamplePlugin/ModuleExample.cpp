#include "ModuleExample.hpp"

namespace
{
	bool CommandExampleTest(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		returnInfo = "Test command: passed!";
		return true;
	}
}

namespace Modules
{
	ModuleExample::ModuleExample() : ModuleBase("Example")
	{
		AddCommand("Test", "test_cmd", "Prints something", eCommandFlagsNone, CommandExampleTest);
	}
}
