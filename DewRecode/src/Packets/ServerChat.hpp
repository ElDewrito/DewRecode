#pragma once

#include <string>
#include <bitset>
#include <memory>
#include <ElDorito/ElDorito.hpp>

namespace Server
{
	namespace Chat
	{
		// Initializes the server chat system.
		void Initialize();

		// Sends a message to a peer as a packet.
		bool SendDirectedServerMessage(const std::string& body, int peer);

		// Sends a message to every peer. Returns true if successful.
		bool SendGlobalMessage(const std::string &body);

		// Sends a message to every player on the local player's team. Returns
		// true if successful.
		bool SendTeamMessage(const std::string &body);

		// Sends a server message to specific peers. Only works if you are
		// host. Returns true if successful.
		bool SendServerMessage(const std::string &body, PeerBitSet peers);

		// Registers a chat handler object.
		void AddHandler(std::shared_ptr<ChatHandler> handler);
	}
}