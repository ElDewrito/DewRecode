#pragma once
#include <ElDorito/ElDorito.hpp>

class NullContext : public ICommandContext
{
	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal()
	{
		return false;
	}
};