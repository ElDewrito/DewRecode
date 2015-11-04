#pragma once
#include <ElDorito/ElDorito.hpp>

class ChatCommandContext : public CommandContext
{
	Blam::Network::Session* session;
	int peer;

public:
	ChatCommandContext(Blam::Network::Session* session, int peer);

	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);
	bool IsInternal() { return false; }
	bool IsChat() { return true; }

	int GetPeerIdx() { return peer; }
	Blam::Network::Session* GetSession() { return session; }
};