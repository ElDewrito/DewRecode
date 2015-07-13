#include "GameConsole.hpp"
#include <iostream>

void GameConsole::ExecuteCommand(std::string command)
{
	std::cout << "Command: " << command << std::endl;
}

void GameConsole::ExecuteCommands(std::string commands)
{
	std::cout << "Commands: " << commands << std::endl;
}