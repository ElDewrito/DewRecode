#pragma once
#include <ElDorito/ElDorito.hpp>

namespace ChatCommands
{
	class VotingCommandProvider : public CommandProvider
	{
	public:
		Command* VarEnabled;
		Command* VarRTVEnabled;
		Command* VarRTVPercent;
		Command* VarVotingTime;

		virtual void RegisterVariables(ICommandManager* manager) override;
	};
}
