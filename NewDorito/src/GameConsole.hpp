#pragma once
#include "../include/ElDorito/ElDorito.hpp"

class GameConsole : public IConsole002, public IConsole001
{
public:
	void ExecuteCommand(std::string command);
	void ExecuteCommands(std::string commands);
};