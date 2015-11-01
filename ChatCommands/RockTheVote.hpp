#pragma once
#include <ElDorito/ElDorito.hpp>

namespace ChatCommands
{
	class RockTheVote : public Server::Chat::ChatHandler
	{
		std::vector<uint64_t> wantsVote;
	public:
		RockTheVote();

		int NumVoted();
		int NumNeeded();
		int NumRemaining();

		virtual bool HostMessageReceived(Blam::Network::Session *session, int peer, const Server::Chat::ChatMessage &message) override;
	};
}