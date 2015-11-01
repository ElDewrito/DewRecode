#include "RockTheVote.hpp"

namespace ChatCommands
{
	RockTheVote::RockTheVote()
	{

	}

	bool RockTheVote::HostMessageReceived(Blam::Network::Session *session, int peer, const Server::Chat::ChatMessage &message)
	{
		std::string body = std::string(message.Body);
		if (body.length() < 3 || body.substr(body.length() - 3, 3) != "rtv")
			return false;

		int retCode = 0;
		IUtils* utils = reinterpret_cast<IUtils*>(CreateInterface(UTILS_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create utils interface");

		IEngine* engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create engine interface");

		int playerIdx = session->MembershipInfo.GetPeerPlayer(peer);
		if (playerIdx == -1)
			return false;

		auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];
		for (auto uid : wantsVote)
			if (uid == player->Uid)
			{
				std::string msg = "You have already voted to Rock the Vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)";
				engine->SendChatDirectedServerMessage(msg, peer);
				return true;
			}

		wantsVote.push_back(player->Uid);

		std::string announcement = utils->ThinString(player->DisplayName) + " wants to rock the vote (" + std::to_string(NumVoted()) + " votes, " + std::to_string(NumNeeded()) + " required)";

		Server::Chat::PeerBitSet peers;
		for (int i = 0; i < 17; i++)
			peers[i] = 1;

		engine->SendChatServerMessage(announcement, peers);

		if (NumRemaining() == 0)
		{
			// TODO: begin map vote
			engine->SendChatServerMessage("RockTheVote::NumRemaining() == 0! Beginning vote!", peers);
		}
		return true;
	}

	int RockTheVote::NumVoted()
	{
		int retCode = 0;
		IEngine* engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create engine interface");

		auto* session = engine->GetActiveNetworkSession();
		if (!session || !session->IsHost())
			return 0;

		int votes = 0;

		auto membership = &session->MembershipInfo;
		for (auto peerIdx = membership->FindFirstPeer(); peerIdx >= 0; peerIdx = membership->FindNextPeer(peerIdx))
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
			if (playerIdx == -1)
				continue;

			auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];
			for (auto uid : wantsVote)
				if (uid == player->Uid)
				{
					votes++;
					break;
				}
		}
		return votes;
	}

	int RockTheVote::NumNeeded()
	{
		int retCode = 0;
		IEngine* engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
		if (retCode != 0)
			throw std::runtime_error("Failed to create engine interface");

		auto* session = engine->GetActiveNetworkSession();
		if (!session || !session->IsHost())
			return 0;

		int connected = engine->GetNumPlayers();
		int needed = connected / 2;
		if (needed < 1 || needed > 16)
			needed = 1;

		return needed;
	}

	int RockTheVote::NumRemaining()
	{
		return NumNeeded() - NumVoted();
	}
}