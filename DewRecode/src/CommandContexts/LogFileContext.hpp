#pragma once
#include <ElDorito/ElDorito.hpp>

class LogFileContext : public ICommandContext
{
	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal()
	{
		return true;
	}
};