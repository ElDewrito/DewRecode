#pragma once
#include "ElDorito.hpp"

class CommandContext
{
public:
	virtual void HandleInput(const std::string& input) = 0;
	virtual void WriteOutput(const std::string& output) = 0;
	virtual bool IsInternal() = 0;
	virtual bool IsChat() = 0;

	virtual int GetPeerIdx() = 0;
	virtual Blam::Network::Session* GetSession() = 0;
};