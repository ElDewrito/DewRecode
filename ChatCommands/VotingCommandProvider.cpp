#include "VotingCommandProvider.hpp"

namespace ChatCommands
{
	const int secsBetweenTallys = 10;

	VotingCommandProvider::VotingCommandProvider()
	{
		int retCode = 0;
		engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create engine interface");

		utils = reinterpret_cast<IUtils*>(CreateInterface(UTILS_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create utils interface");

		commands = reinterpret_cast<ICommandManager*>(CreateInterface(COMMANDMANAGER_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create command manager interface");

	}

	std::vector<Command> VotingCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Voting", "RTV", "rtv", "Votes to Rock the Vote", eCommandFlagsChatCommand, BIND_COMMAND(this, &VotingCommandProvider::CommandRTV)),
			Command::CreateCommand("Voting", "UnRTV", "unrtv", "Unvotes to Rock the Vote", eCommandFlagsChatCommand, BIND_COMMAND(this, &VotingCommandProvider::CommandUnRTV)),
			Command::CreateCommand("Voting", "Vote", "vote", "Votes for a map", eCommandFlagsChatCommand, BIND_COMMAND(this, &VotingCommandProvider::CommandVote))
		};

		return commands;
	}

	void VotingCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarEnabled = manager->Add(Command::CreateVariableInt("Voting", "Enabled", "voting_enabled", "End of game map voting", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 0));
		VarEnabled->ValueIntMin = 0;
		VarEnabled->ValueIntMax = 1;

		VarRTVEnabled = manager->Add(Command::CreateVariableInt("Voting", "RTVEnabled", "voting_rtvenabled", "Midgame map voting", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 0));
		VarRTVEnabled->ValueIntMin = 0;
		VarRTVEnabled->ValueIntMax = 1;

		VarRTVPercent = manager->Add(Command::CreateVariableInt("Voting", "RTVPercent", "voting_rtvpercent", "The percent of connected players needed for RTV to trigger", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 75));
		VarRTVPercent->ValueIntMin = 0;
		VarRTVPercent->ValueIntMax = 100;

		VarVotingTime = manager->Add(Command::CreateVariableInt("Voting", "VotingTime", "voting_time", "How many seconds voting should last for", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 60));		
	}

	void VotingCommandProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnTick(BIND_CALLBACK(this, &VotingCommandProvider::OnTick));
		engine->OnEvent("Core", "Server.LifeCycleStateChanged", BIND_CALLBACK(this, &VotingCommandProvider::CallbackLifeCycleStateChanged));
	}

	void VotingCommandProvider::OnTick(const std::chrono::duration<double>& deltaTime)
	{
		if (voteTimeStarted == 0)
			return;

		Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		time_t curTime;
		time(&curTime);
		auto elapsed = curTime - voteTimeStarted;

		if (lastTally == 0)
			lastTally = curTime;

		auto elapsedTally = curTime - lastTally;

		int voteTimeSecs = VarVotingTime->ValueInt;

		if (elapsed < voteTimeSecs && !(elapsedTally >= secsBetweenTallys))
			return;

		if (!nextMap.empty())
		{
			if (elapsed >= voteTimeSecs + 5)
			{
				if (lifeCycleState == Blam::Network::LifeCycleState::InGame || lifeCycleState == Blam::Network::LifeCycleState::StartGame)
					commands->Execute("Game.Stop", commands->GetLogFileContext()); // return to lobby and let the life cycle handler change the map
				else
				{
					commands->Execute("Game.Map " + nextMap, commands->GetLogFileContext());
					commands->Execute("Game.Start", commands->GetLogFileContext());
					nextMap = "";
				}

				voteTimeStarted = 0;
			}
			return;
		}

		std::map<std::string, int> totals;
		for (auto v : mapVotes)
		{
			bool exists = false;
			for (auto m : totals)
				if (m.first == v.second)
				{
					exists = true;
					break;
				}

			if (exists)
				totals[v.second]++;
			else
				totals.insert({ v.second, 1 });
		}

		if (elapsedTally >= secsBetweenTallys)
		{
			engine->SendChatServerMessage("Voting ends in " + std::to_string(voteTimeSecs - elapsed) + " seconds, current votes:", peers);

			for (auto v : totals)
				engine->SendChatServerMessage(v.first + " - " + std::to_string(v.second), peers);

			engine->SendChatServerMessage("Type /vote [mapname] to vote for the next map!", peers);

			lastTally = curTime;
		}

		if (elapsed < voteTimeSecs)
			return;

		std::string winner = "";
		int count = 0;
		for (auto v : totals)
			if (v.second > count)
			{
				winner = v.first;
				count = v.second;
			}

		if (winner == "")
		{
			engine->SendChatServerMessage("Voting ended, map extended.", peers);
			voteTimeStarted = 0;
		}
		else
		{
			engine->SendChatServerMessage("Voting ended! Changing map to " + winner + " in 5 seconds...", peers);
			nextMap = winner;
		}
	}

	void VotingCommandProvider::CallbackLifeCycleStateChanged(void* param)
	{
		lifeCycleState = *(Blam::Network::LifeCycleState*)param;
		if (lifeCycleState == Blam::Network::LifeCycleState::PreGame && !nextMap.empty())
		{
			commands->Execute("Game.Map " + nextMap, commands->GetLogFileContext());
			commands->Execute("Game.Start", commands->GetLogFileContext());
			nextMap = "";
		}
	}

	/*bool VotingCommandProvider::HostMessageReceived(Blam::Network::Session *session, int peer, const Chat::ChatMessage &message)
	{
		std::string body = std::string(message.Body);
		if ((body.length() >= 3 && body.substr(0, 3) == "rtv") ||
			(body.length() >= 4 && body.substr(1, 3) == "rtv") ||
			(body.length() >= 11 && body.substr(body.length() - 11, 11) == "rockthevote"))
			return CommandRTV(session, peer);

		else if ((body.length() >= 5 && body.substr(0, 5) == "unrtv") ||
			(body.length() >= 6 && body.substr(1, 5) == "unrtv") ||
			(body.length() >= 13 && body.substr(body.length() - 13, 13) == "unrockthevote"))
			return CommandUnRTV(session, peer);

		else if ((body.length() >= 4 && body.substr(0, 4) == "vote") ||
			(body.length() >= 5 && body.substr(1, 4) == "vote"))
			return CommandVote(session, peer, body);

		return false;
	}*/

	void VotingCommandProvider::SendVoteText()
	{
		Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		int retCode = 0;

		engine->SendChatServerMessage("Map vote started! Type /vote [mapname] to vote for a map.", peers);
	}

	bool VotingCommandProvider::CommandVote(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (!VarRTVEnabled->ValueInt && !VarEnabled->ValueInt)
			return false;

		Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		if (!voteTimeStarted)
		{
			context.WriteOutput("Voting hasn't begun yet, type rtv to vote to Rock the Vote!");
			return true;
		}

		if (Arguments.size() <= 0)
		{
			context.WriteOutput("/vote: Invalid input, type /vote [mapname] to vote for that map!");
			return true;
		}

		// TODO: check for valid map name / forge map variant

		auto voted = Arguments[0];

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

		int peer = context.GetPeerIdx();
		auto* session = context.GetSession();

		if (!engine->IsDedicated() || peer != session->MembershipInfo.LocalPeerIndex)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peer);
			if (playerIdx == -1)
				return true;
			auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

			playerUid = player->Uid;
			playerName = utils->ThinString(player->DisplayName);
		}

		bool hasVotedPreviously = false;
		for (auto v : mapVotes)
		{
			if (v.first == playerUid)
			{
				hasVotedPreviously = true;
				if (v.second == voted)
				{
					context.WriteOutput("You've already voted for this map.");
					return true;
				}
				break;
			}
		}

		if (!hasVotedPreviously)
			mapVotes.insert({ playerUid, voted });
		else
			mapVotes[playerUid] = voted;

		if (!hasVotedPreviously)
			engine->SendChatServerMessage(playerName + " voted for " + voted + " (/vote " + voted + ")", peers);
		else
			engine->SendChatServerMessage(playerName + " changed their vote to " + voted + " (/vote " + voted + ")", peers);

		engine->SendChatServerMessage("Current votes:", peers);
		std::map<std::string, int> totals;
		for (auto v : mapVotes)
		{
			bool exists = false;
			for (auto m : totals)
				if (m.first == v.second)
				{
					exists = true;
					break;
				}

			if (exists)
				totals[v.second]++;
			else
				totals.insert({ v.second, 1 });
		}
		for (auto v : totals)
			engine->SendChatServerMessage(v.first + " - " + std::to_string(v.second), peers);

		time(&lastTally);

		return true;
	}

	bool VotingCommandProvider::CommandRTV(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (!VarRTVEnabled->ValueInt)
			return false;

		if (voteTimeStarted != 0)
		{
			context.WriteOutput("Can't rtv, voting is already in progress!");
			return true;
		}

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

		int peer = context.GetPeerIdx();
		auto* session = context.GetSession();

		if (!engine->IsDedicated() || peer != session->MembershipInfo.LocalPeerIndex)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peer);
			if (playerIdx == -1)
				return true;
			auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

			playerUid = player->Uid;
			playerName = utils->ThinString(player->DisplayName);
		}

		for (auto uid : wantsVote)
			if (uid == playerUid)
			{
				context.WriteOutput("You have already voted to Rock the Vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)");
				return true;
			}

		wantsVote.push_back(playerUid);

		std::string announcement = playerName + " wants to rock the vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)";

		Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		engine->SendChatServerMessage(announcement, peers);

		if (NumRemaining() <= 0)
		{
			engine->SendChatServerMessage("Rock the Vote passed! Starting map vote...", peers);

			time(&voteTimeStarted);
			lastTally = 0;
			wantsVote.clear();
			mapVotes.clear();
			hasVoted.clear();
			nextMap.clear();

			SendVoteText();
		}

		return true;
	}
	bool VotingCommandProvider::CommandUnRTV(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (!VarRTVEnabled->ValueInt)
			return false;

		if (voteTimeStarted != 0)
		{
			context.WriteOutput("Can't unrtv, voting is already in progress!");
			return true;
		}

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

		int peer = context.GetPeerIdx();
		auto* session = context.GetSession();

		if (!engine->IsDedicated() || peer != session->MembershipInfo.LocalPeerIndex)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peer);
			if (playerIdx == -1)
				return true;
			auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

			playerUid = player->Uid;
			playerName = utils->ThinString(player->DisplayName);
		}

		int idx = -1;
		for (size_t i = 0; i < wantsVote.size(); i++)
		{
			if (wantsVote[i] == playerUid)
			{
				idx = i;
				break;
			}
		}

		if (idx != -1)
			wantsVote.erase(wantsVote.begin() + idx);

		context.WriteOutput("Your rtv vote has been removed (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)");
		return true;
	}

	int VotingCommandProvider::NumVoted()
	{
		auto* session = engine->GetActiveNetworkSession();
		if (!session || !session->IsHost())
			return 0;

		int votes = 0;

		auto membership = &session->MembershipInfo;
		for (auto peerIdx = membership->FindFirstPeer(); peerIdx >= 0; peerIdx = membership->FindNextPeer(peerIdx))
		{
			uint64_t playerUid = 0xBEEFCAFE1337BABE;
			if (!engine->IsDedicated() || peerIdx != session->MembershipInfo.LocalPeerIndex)
			{
				int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
				if (playerIdx == -1)
					continue;

				auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];
				playerUid = player->Uid;
			}

			for (auto uid : wantsVote)
				if (uid == playerUid)
				{
					votes++;
					break;
				}
		}
		return votes;
	}

	int VotingCommandProvider::NumNeeded()
	{
		auto* session = engine->GetActiveNetworkSession();
		if (!session || !session->IsHost())
			return 0;

		float connected = (float)engine->GetNumPlayers();
		float multi = (float)VarRTVPercent->ValueInt / 100.f;
		int needed = (int)(connected * multi);

		if (needed < 1 || needed > 16)
			needed = 1;

		return needed;
	}

	int VotingCommandProvider::NumRemaining()
	{
		return NumNeeded() - NumVoted();
	}
}