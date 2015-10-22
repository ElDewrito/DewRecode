#pragma once
#include <ElDorito/ElDorito.hpp>

class ConsoleContext : public ICommandContext
{
public:
	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal()
	{
		return false;
	}
};