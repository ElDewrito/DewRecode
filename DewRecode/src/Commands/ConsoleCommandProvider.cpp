#include "ConsoleCommandProvider.hpp"

namespace Console
{
	ConsoleCommandProvider::ConsoleCommandProvider(std::shared_ptr<UI::ConsoleWindow> consoleWindow)
	{
		this->consoleWindow = consoleWindow;
	}

	std::vector<Command> ConsoleCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
		};

		return commands;
	}

	void ConsoleCommandProvider::RegisterCallbacks(IEngine* engine)
	{
	}


}