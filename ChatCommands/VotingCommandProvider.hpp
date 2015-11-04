#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>

namespace ChatCommands
{
	class VotingCommandProvider : public CommandProvider
	{
		std::vector<uint64_t> wantsVote;
		std::map<uint64_t, std::string> mapVotes;
		std::vector<uint64_t> hasVoted;

		time_t voteTimeStarted = 0;
		time_t lastTally = 0;
		//bool inGame = false;
		Blam::Network::LifeCycleState lifeCycleState;
		bool votedDuringThisState = false;

		IEngine* engine;
		IUtils* utils;
		ICommandManager* commands;

		std::string nextMap;

	public:
		VotingCommandProvider();

		Command* VarEnabled;
		Command* VarRTVEnabled;
		Command* VarRTVPercent;
		Command* VarVotingTime;

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		void OnTick(const std::chrono::duration<double>& deltaTime);
		void CallbackLifeCycleStateChanged(void* param);

		int NumVoted();
		int NumNeeded();
		int NumRemaining();

		void SendVoteText();

		bool CommandRTV(const std::vector<std::string>& Arguments, CommandContext& context);
		bool CommandUnRTV(const std::vector<std::string>& Arguments, CommandContext& context);
		bool CommandVote(const std::vector<std::string>& Arguments, CommandContext& context);
		bool CommandCancel(const std::vector<std::string>& Arguments, CommandContext& context);
	};
}
