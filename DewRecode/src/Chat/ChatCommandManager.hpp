#pragma once
#include <ElDorito/ElDorito.hpp>

namespace Chat
{
	class ChatCommandManager : public ChatHandler
	{
		virtual bool HostMessageReceived(Blam::Network::Session *session, int peer, const Chat::ChatMessage &message) override;
	};
}