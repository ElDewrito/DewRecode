#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>
#include "VotingCommandProvider.hpp"
namespace ChatCommands
{
	class Voting : public Chat::ChatHandler
	{
		std::vector<uint64_t> wantsVote;
		std::map<uint64_t, std::string> mapVotes;
		std::vector<uint64_t> hasVoted;

		time_t voteTimeStarted = 0;
		time_t lastTally = 0;
		//bool inGame = false;
		Blam::Network::LifeCycleState lifeCycleState;

		IEngine* engine;
		IUtils* utils;
		ICommandManager* commands;

		std::string nextMap;

		std::shared_ptr<VotingCommandProvider> votingCmds;

	public:
		Voting(std::shared_ptr<VotingCommandProvider> votingCmds);

		void OnTick(const std::chrono::duration<double>& deltaTime);
		void CallbackLifeCycleStateChanged(void* param);

		int NumVoted();
		int NumNeeded();
		int NumRemaining();

		void SendVoteText();
		bool CommandRTV(Blam::Network::Session *session, int peer);
		bool CommandUnRTV(Blam::Network::Session *session, int peer);
		bool CommandVote(Blam::Network::Session *session, int peer, const std::string& body);

		virtual bool HostMessageReceived(Blam::Network::Session *session, int peer, const Chat::ChatMessage &message) override;
	};
}