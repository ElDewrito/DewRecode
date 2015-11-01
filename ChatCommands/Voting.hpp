#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>
#include "VotingCommandProvider.hpp"
namespace ChatCommands
{
	class Voting : public Server::Chat::ChatHandler
	{
		std::vector<uint64_t> wantsVote;
		std::map<uint64_t, std::string> mapVotes;
		std::vector<uint64_t> hasVoted;

		time_t voteTimeStarted = 0;
		time_t lastTally = 0;

		IEngine* engine;
		IUtils* utils;
		ICommandManager* commands;

		std::shared_ptr<VotingCommandProvider> votingCmds;

	public:
		Voting(std::shared_ptr<VotingCommandProvider> votingCmds);

		void OnTick(const std::chrono::duration<double>& deltaTime);

		int NumVoted();
		int NumNeeded();
		int NumRemaining();

		void SendVoteText();
		bool CommandRTV(Blam::Network::Session *session, int peer);
		bool CommandUnRTV(Blam::Network::Session *session, int peer);
		bool CommandVote(Blam::Network::Session *session, int peer, const std::string& body);

		virtual bool HostMessageReceived(Blam::Network::Session *session, int peer, const Server::Chat::ChatMessage &message) override;
	};
}