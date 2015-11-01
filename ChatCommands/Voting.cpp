#include "Voting.hpp"

namespace ChatCommands
{
	const int secsBetweenTallys = 10;

	Voting::Voting(std::shared_ptr<VotingCommandProvider> votingCmds)
	{
		this->votingCmds = votingCmds;

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

		engine->OnTick(BIND_CALLBACK(this, &Voting::OnTick));
		engine->OnEvent("Core", "Server.LifeCycleStateChanged", BIND_CALLBACK(this, &Voting::CallbackLifeCycleStateChanged));
	}

	void Voting::OnTick(const std::chrono::duration<double>& deltaTime)
	{
		if (voteTimeStarted == 0)
			return;

		Server::Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		time_t curTime;
		time(&curTime);
		auto elapsed = curTime - voteTimeStarted;

		if (lastTally == 0)
			lastTally = curTime;

		auto elapsedTally = curTime - lastTally;

		int voteTimeSecs = votingCmds->VarVotingTime->ValueInt;

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

	void Voting::CallbackLifeCycleStateChanged(void* param)
	{
		lifeCycleState = *(Blam::Network::LifeCycleState*)param;
		if (lifeCycleState == Blam::Network::LifeCycleState::PreGame && !nextMap.empty())
		{
			commands->Execute("Game.Map " + nextMap, commands->GetLogFileContext());
			commands->Execute("Game.Start", commands->GetLogFileContext());
			nextMap = "";
		}
	}

	bool Voting::HostMessageReceived(Blam::Network::Session *session, int peer, const Server::Chat::ChatMessage &message)
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
	}

	void Voting::SendVoteText()
	{
		Server::Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		int retCode = 0;
		
		engine->SendChatServerMessage("Map vote started! Type /vote [mapname] to vote for a map.", peers);
	}

	bool Voting::CommandVote(Blam::Network::Session *session, int peer, const std::string& body)
	{
		if (!votingCmds->VarRTVEnabled->ValueInt && !votingCmds->VarEnabled->ValueInt)
			return false;

		Server::Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		if (!voteTimeStarted)
		{
			engine->SendChatDirectedServerMessage("Voting hasn't begun yet, type rtv to vote to Rock the Vote!", peer);
			return true;
		}

		auto voteEndIdx = body.find_first_of(" ");

		if (voteEndIdx == std::string::npos || voteEndIdx+1 >= body.length())
		{
			engine->SendChatDirectedServerMessage("/votemap: Invalid input, type /votemap [mapname] to vote for that map!", peer);
			return true;
		}
		// TODO: check for valid map name / forge map variant

		auto voted = body.substr(voteEndIdx+1);

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

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
					engine->SendChatDirectedServerMessage("You've already voted for this map", peer);
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

	bool Voting::CommandRTV(Blam::Network::Session *session, int peer)
	{
		if (!votingCmds->VarRTVEnabled->ValueInt)
			return false;

		if (voteTimeStarted != 0)
		{
			engine->SendChatDirectedServerMessage("Can't rtv, voting is already in progress!", peer);
			return true;
		}

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

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
				std::string msg = "You have already voted to Rock the Vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)";
				engine->SendChatDirectedServerMessage(msg, peer);
				return true;
			}

		wantsVote.push_back(playerUid);

		std::string announcement = playerName + " wants to rock the vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)";

		Server::Chat::PeerBitSet peers;
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
	bool Voting::CommandUnRTV(Blam::Network::Session *session, int peer)
	{
		if (!votingCmds->VarRTVEnabled->ValueInt)
			return false;

		if (voteTimeStarted != 0)
		{
			engine->SendChatDirectedServerMessage("Can't unrtv, voting is already in progress!", peer);
			return true;
		}

		uint64_t playerUid = 0xBEEFCAFE1337BABE;
		std::string playerName = "SERVER";

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

		engine->SendChatDirectedServerMessage("Your rtv vote has been removed (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)", peer);
		return true;
	}

	int Voting::NumVoted()
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

	int Voting::NumNeeded()
	{
		auto* session = engine->GetActiveNetworkSession();
		if (!session || !session->IsHost())
			return 0;

		int connected = engine->GetNumPlayers();
		int needed = connected / 2;
		if (needed < 1 || needed > 16)
			needed = 1;

		return needed;
	}

	int Voting::NumRemaining()
	{
		return NumNeeded() - NumVoted();
	}
}