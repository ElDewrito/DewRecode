#include "ChatCommandContext.hpp"
#include "../ElDorito.hpp"

ChatCommandContext::ChatCommandContext(Blam::Network::Session* session, int peer)
{
	this->session = session;
	this->peer = peer;
}

void ChatCommandContext::HandleInput(const std::string& input)
{
}

void ChatCommandContext::WriteOutput(const std::string& output)
{
	ElDorito::Instance().Engine.SendChatDirectedServerMessage(output, peer);
}