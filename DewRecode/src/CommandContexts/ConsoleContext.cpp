#include "ConsoleContext.hpp"
#include "../ElDorito.hpp"
#include "../UI/UserInterface.hpp"

// todo: this

void ConsoleContext::HandleInput(const std::string& input)
{
	ElDorito::Instance().CommandManager.Execute(input, *this);
}

void ConsoleContext::WriteOutput(const std::string& output)
{
	ElDorito::Instance().UserInterface.WriteToConsole(output);
}