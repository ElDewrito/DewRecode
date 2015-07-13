#pragma once
#include <string>

class IConsole001
{
public:
	virtual void ExecuteCommand(std::string command) = 0;
};

class IConsole002
{
public:
	virtual void ExecuteCommand(std::string command) = 0;
	virtual void ExecuteCommands(std::string commands) = 0;
};

#define CONSOLE_INTERFACE_VERSION001 "GameConsole001"
#define CONSOLE_INTERFACE_VERSION002 "GameConsole002"