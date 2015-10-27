#pragma once
#include "ElDorito.hpp"

class CommandProvider
{
public:
	virtual std::vector<Command> GetCommands() { return std::vector<Command>(); }
	virtual void RegisterVariables(ICommandManager* manager) { return; }
	virtual void RegisterCallbacks(IEngine* engine) { return; }
};