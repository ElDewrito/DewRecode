#include "ExampleCommandProvider.hpp"

namespace Example
{
	std::vector<Command> ExampleCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Example", "Test", "test_cmd", "Example plugin test command", eCommandFlagsNone, BIND_COMMAND(this, &ExampleCommandProvider::CommandTest))
		};

		return commands;
	}

	bool ExampleCommandProvider::CommandTest(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		context.WriteOutput("ExampleCommandProvider: Test passed!");
		return true;
	}
}