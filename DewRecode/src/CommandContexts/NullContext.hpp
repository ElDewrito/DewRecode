#pragma once
#include <ElDorito/ElDorito.hpp>

class NullContext : public CommandContext
{
public:
	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal()
	{
		return false;
	}
};