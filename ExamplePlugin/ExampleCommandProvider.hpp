#pragma once
#include <ElDorito/ElDorito.hpp>

namespace Example
{
	class ExampleCommandProvider : public CommandProvider
	{
	public:
		virtual std::vector<Command> GetCommands() override;

		bool CommandTest(const std::vector<std::string>& Arguments, CommandContext& context);
	};
}
