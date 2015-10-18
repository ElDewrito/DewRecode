#pragma once
#include "ElDorito.hpp"

class ICommandContext
{
public:
	virtual void HandleInput(const std::string& input) = 0;
	virtual void WriteOutput(const std::string& output) = 0;
	virtual bool IsInternal() = 0;
};