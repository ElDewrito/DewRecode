#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "ModuleServer.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "../ElDorito.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace
{
	bool VariableServerCountdownUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long seconds = ElDorito::Instance().Modules.Server.VarServerCountdown->ValueInt;

		Pointer(0x553708).Write<uint8_t>((uint8_t)seconds + 0); // player control
		Pointer(0x553738).Write<uint8_t>((uint8_t)seconds + 4); // camera position
		Pointer(0x5521D1).Write<uint8_t>((uint8_t)seconds + 4); // ui timer

		// Fix team notification
		if (seconds == 4)
			Pointer(0x5536F0).Write<uint8_t>(2);
		else
			Pointer(0x5536F0).Write<uint8_t>(3);

		return true;
	}

	bool VariableServerMaxPlayersUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		typedef char(__cdecl *NetworkSquadSessionSetMaximumPlayerCountFunc)(int count);
		auto network_squad_session_set_maximum_player_count = (NetworkSquadSessionSetMaximumPlayerCountFunc)0x439BA0;
		char ret = network_squad_session_set_maximum_player_count(ElDorito::Instance().Modules.Server.VarServerMaxPlayers->ValueInt);
		if (ret == 0)
		{
			returnInfo = "Failed to update max player count, are you hosting a lobby?";
			return false;
		}

		return true;
	}

	// retrieves master server endpoints from dewrito.json
	void GetEndpoints(std::vector<std::string>& destVect, std::string endpointType)
	{
		std::ifstream in("dewrito.json", std::ios::in | std::ios::binary);
		if (in && in.is_open())
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			contents.resize((unsigned int)in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();

			rapidjson::Document json;
			if (!json.Parse<0>(contents.c_str()).HasParseError() && json.IsObject())
			{
				if (json.HasMember("masterServers"))
				{
					auto& mastersArray = json["masterServers"];
					for (auto it = mastersArray.Begin(); it != mastersArray.End(); it++)
					{
						destVect.push_back((*it)[endpointType.c_str()].GetString());
					}
				}
			}
		}
	}

	DWORD WINAPI CommandServerAnnounceStats_Thread(LPVOID lpParam)
	{
		std::stringstream ss;
		std::vector<std::string> statsEndpoints;
		auto& dorito = ElDorito::Instance();

		GetEndpoints(statsEndpoints, "stats");

		//typedef int(__cdecl *Game_GetLocalPlayerDatumIdxPtr)(int localPlayerIdx);
		//auto Game_GetLocalPlayerDatumIdx = reinterpret_cast<Game_GetLocalPlayerDatumIdxPtr>(0x589C30);
		//uint16_t playerIdx = (uint16_t)(Game_GetLocalPlayerDatumIdx(0) & 0xFFFF);
		// above wont work since we're on a different thread without the proper TLS data :(

		auto& localPlayers = dorito.Engine.GetMainTls(GameGlobals::LocalPlayers::TLSOffset)[0];
		uint16_t playerIdx = (uint16_t)(localPlayers(GameGlobals::LocalPlayers::Player0DatumIdx).Read<uint32_t>() & 0xFFFF);

		auto& playersGlobal = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];
		int32_t team = playersGlobal(0x54 + GameGlobals::Players::TeamOffset + (playerIdx * GameGlobals::Players::PlayerEntryLength)).Read<int32_t>();

		int16_t score = playersGlobal(0x54 + GameGlobals::Players::ScoreBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		int16_t kills = playersGlobal(0x54 + GameGlobals::Players::KillsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		int16_t deaths = playersGlobal(0x54 + GameGlobals::Players::DeathsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		// unsure about assists
		int16_t assists = playersGlobal(0x54 + GameGlobals::Players::AssistsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();

		// TODO: get an ID for this match
		int32_t gameId = 0x1337BEEF;
		
		// build our stats announcement
		rapidjson::StringBuffer statsBuff;
		rapidjson::Writer<rapidjson::StringBuffer> statsWriter(statsBuff);
		statsWriter.StartObject();
		statsWriter.Key("gameId");
		statsWriter.Int(gameId);
		statsWriter.Key("score");
		statsWriter.Int(score);
		statsWriter.Key("kills");
		statsWriter.Int(kills);
		statsWriter.Key("assists");
		statsWriter.Int(assists);
		statsWriter.Key("deaths");
		statsWriter.Int(deaths);
		statsWriter.Key("team");
		statsWriter.Int(team);
		statsWriter.Key("medals");
		statsWriter.StartArray();

		// TODO: log each medal earned during the game and output them here
		/*statsWriter.String("doublekill");
		statsWriter.String("triplekill");
		statsWriter.String("overkill");
		statsWriter.String("unfreakingbelieveable");*/

		statsWriter.EndArray();
		statsWriter.EndObject();

		std::string statsObject = statsBuff.GetString();
		// todo: look into using JSON Web Tokens (JWT) that use JSON Web Signature (JWS), instead of using our own signature stuff
		std::string statsSignature;
		if (!dorito.Utils.RSACreateSignature(dorito.Modules.PlayerUidPatches.GetFormattedPrivKey(), (void*)statsObject.c_str(), statsObject.length(), statsSignature))
		{
			ss << "Failed to create stats RSA signature!";
			dorito.Logger.Log(LogSeverity::Error, "AnnounceStats", ss.str());
			return 0;
		}

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);
		writer.StartObject();
		writer.Key("statsVersion");
		writer.Int(1);
		writer.Key("stats");
		writer.String(statsObject.c_str()); // write stats object as a string instead of object so that the string matches up exactly with what we signed (also because there's no easy way to append a writer..)
		writer.Key("publicKey");
		writer.String(ElDorito::Instance().Modules.Player.VarPlayerPubKey->ValueString.c_str());
		writer.Key("signature");
		writer.String(statsSignature.c_str());
		writer.EndObject();

		std::string sendObject = s.GetString();

		for (auto server : statsEndpoints)
		{
			HttpRequest req;
			try
			{
				req = ElDorito::Instance().Utils.HttpSendRequest(dorito.Utils.WidenString(server), L"POST", L"ElDewrito/" + dorito.Utils.WidenString(Utils::Version::GetVersionString()), L"", L"", L"Content-Type: application/json\r\n", (void*)sendObject.c_str(), sendObject.length());
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server " << server << " (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...)
			{
				ss << "Exception during master server stats announce request to " << server << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			std::wstring expected = L"HTTP/1.1 200 OK";
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server stats response from " << server << std::endl << std::endl;
				continue;
			}

			auto respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server stats response from " << server << std::endl << std::endl;
				continue;
			}

			// parse the json response
			std::string resp = std::string(req.ResponseBody.begin(), req.ResponseBody.end());
			rapidjson::Document json;
			if (json.Parse<0>(resp.c_str()).HasParseError() || !json.IsObject())
			{
				ss << "Invalid master server JSON response from " << server << std::endl << std::endl;
				continue;
			}

			if (!json.HasMember("result"))
			{
				ss << "Master server JSON response from " << server << " is missing data." << std::endl << std::endl;
				continue;
			}

			auto& result = json["result"];
			if (result["code"].GetInt() != 0)
			{
				ss << "Master server " << server << " returned error code " << result["code"].GetInt() << " (" << result["msg"].GetString() << ")" << std::endl << std::endl;
				continue;
			}
		}

		std::string errors = ss.str();
		if (!errors.empty())
			dorito.Logger.Log(LogSeverity::Error, "AnnounceStats", ss.str());

		return true;
	}

	bool CommandServerAnnounceStats(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		//if (!IsEndGame())
		//	return false;

		auto thread = CreateThread(NULL, 0, CommandServerAnnounceStats_Thread, (LPVOID)&Arguments, 0, NULL);
		returnInfo = "Announcing stats to master servers...";
		return true;
	}

	bool CommandServerConnect(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		// TODO: move this into a thread so that non-responding hosts don't lag the game
		if (Arguments.size() <= 0)
		{
			returnInfo = "Invalid arguments.";
			return false;
		}
		auto& dorito = ElDorito::Instance();

		std::string address = Arguments[0];
		std::string password = "";
		if (Arguments.size() > 1)
			password = Arguments[1];

		uint32_t rawIpaddr = 0;
		int httpPort = 11784;

		size_t portOffset = address.find(':');
		auto host = address;
		if (portOffset != std::string::npos && portOffset + 1 < address.size())
		{
			auto port = address.substr(portOffset + 1);
			httpPort = (uint16_t)std::stoi(port);
			host = address.substr(0, portOffset);
		}

		struct addrinfo* info = NULL;
		INT retval = getaddrinfo(host.c_str(), NULL, NULL, &info);
		if (retval != 0)
		{
			int error = WSAGetLastError();
			std::string ret = "Unable to lookup " + address + " (" + std::to_string(retval) + "): ";
			if (error != 0)
			{
				if (error == WSAHOST_NOT_FOUND)
					ret += "Host not found.";
				else if (error == WSANO_DATA)
					ret += "No data record found.";
				else
					ret += "Function failed with error " + std::to_string(error) + ".";
			}
			else
				ret += "Unknown error.";
			returnInfo = ret;
			return false;
		}

		struct addrinfo *ptr = NULL;
		for (ptr = info; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_family != AF_INET)
				continue; // not ipv4

			rawIpaddr = ntohl(((sockaddr_in*)ptr->ai_addr)->sin_addr.S_un.S_addr);
			break;
		}

		freeaddrinfo(info);

		if (!rawIpaddr)
		{
			returnInfo = "Unable to lookup " + address + ": No records found.";;
			return false;
		}
		
		// query the server
		std::wstring usernameStr = L"";
		std::wstring passwordStr = L"";
		if (!password.empty())
		{
			usernameStr = L"dorito";
			passwordStr = dorito.Utils.WidenString(password);
		}

		HttpRequest req = ElDorito::Instance().Utils.HttpSendRequest(dorito.Utils.WidenString("http://" + host + ":" + std::to_string(httpPort) + "/"), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(Utils::Version::GetVersionString()), usernameStr, passwordStr, L"", NULL, 0);

		if (req.Error != HttpRequestError::None)
		{
			returnInfo = "Unable to connect to server. (error: " + std::to_string((int)req.Error) + "/" + std::to_string(req.LastError) + "/" + std::to_string(GetLastError()) + ")";
			return false;
		}

		// make sure the server replied with 200 OK
		std::wstring expected = L"HTTP/1.1 200 OK";
		if (req.ResponseHeader.length() < expected.length())
		{
			returnInfo = "Invalid server query response.";
			return false;
		}

		auto respHdr = req.ResponseHeader.substr(0, expected.length());
		if (respHdr.compare(expected))
		{
			returnInfo = "Invalid server query header response.";
			return false;
		}

		// parse the json response
		std::string resp = std::string(req.ResponseBody.begin(), req.ResponseBody.end());

		rapidjson::Document json;
		if (json.Parse<0>(resp.c_str()).HasParseError() || !json.IsObject())
		{
			returnInfo = "Invalid server query JSON response.";
			return false;
		}

		// make sure the json has all the members we need
		if (!json.HasMember("gameVersion") || !json["gameVersion"].IsString() ||
			!json.HasMember("eldewritoVersion") || !json["eldewritoVersion"].IsString() ||
			!json.HasMember("port") || !json["port"].IsNumber())
		{
			returnInfo = "Server query JSON response is missing data.";
			return false;
		}

		if (!json.HasMember("xnkid") || !json["xnkid"].IsString() ||
			!json.HasMember("xnaddr") || !json["xnaddr"].IsString())
		{
			returnInfo = "Incorrect password specified.";
			return false;
		}

		std::string gameVer = json["gameVersion"].GetString();
		std::string edVer = json["eldewritoVersion"].GetString();

		std::string ourGameVer((char*)Pointer(0x199C0F0));
		std::string ourEdVer = Utils::Version::GetVersionString();
		if (ourGameVer.compare(gameVer))
		{
			returnInfo = "Server is running a different game version.";
			return false;
		}

		if (ourEdVer.compare(edVer))
		{
			returnInfo = "Server is running a different ElDewrito version.";
			return false;
		}

		std::string xnkid = json["xnkid"].GetString();
		std::string xnaddr = json["xnaddr"].GetString();
		if (xnkid.length() != (0x10 * 2) || xnaddr.length() != (0x10 * 2))
		{
			returnInfo = "Server query XNet info is invalid.";
			return false;
		}
		uint16_t gamePort = (uint16_t)json["port"].GetInt();

		BYTE xnetInfo[0x20];
		dorito.Utils.HexStringToBytes(xnkid, xnetInfo, 0x10);
		dorito.Utils.HexStringToBytes(xnaddr, xnetInfo + 0x10, 0x10);

		// set up our syslink data struct
		auto& server = ElDorito::Instance().Modules.Server;
		memset(server.SyslinkData, 0, 0x176);
		*(uint32_t*)server.SyslinkData = 1;

		memcpy(server.SyslinkData + 0x9E, xnetInfo, 0x20);

		*(uint32_t*)(server.SyslinkData + 0x170) = rawIpaddr;
		*(uint16_t*)(server.SyslinkData + 0x174) = gamePort;

		// set syslink stuff to point at our syslink data
		Pointer(0x228E6D8).Write<uint32_t>(1);
		Pointer(0x228E6DC).Write<uint32_t>((uint32_t)&server.SyslinkData);

		// tell the game to start joining
		Pointer(0x2240BA8).Write<int64_t>(-1);
		Pointer(0x2240BB0).Write<uint32_t>(1);
		Pointer(0x2240BB4).Write(xnetInfo, 0x10);
		Pointer(0x2240BD4).Write(xnetInfo + 0x10, 0x10);
		Pointer(0x2240BE4).Write<uint32_t>(1);

		// send an event
		dorito.Engine.Event("Core", "Game.Joining");

		returnInfo = "Attempting connection to " + address + "...";
		return true;
	}

	void CallbackEndGame(void* param)
	{
		// TODO: check if the user is hosting/joined a game (ie. make sure the game isn't just an offline game)
		// TODO: make sure the game has had 2 or more players during gameplay
		// TODO: get a game/match ID
		// TODO: make sure we haven't announced stats already for this game ID
		// TODO: make Server.AnnounceStats only callable in code, not via the console (once we've finished debugging it etc
		CommandServerAnnounceStats(std::vector<std::string>(), std::string());
	}
}

namespace Modules
{
	ModuleServer::ModuleServer() : ModuleBase("Server")
	{
		engine->OnEvent("Core", "Game.End", CallbackEndGame);
		// TODO: move [Port, Announce, Unannounce] to ServerPlugin once HttpRequest is exposed via interface

		VarServerCountdown = AddVariableInt("Countdown", "countdown", "The number of seconds to wait at the start of the game", eCommandFlagsArchived, 5, VariableServerCountdownUpdate);
		VarServerCountdown->ValueIntMin = 0;
		VarServerCountdown->ValueIntMax = 20;

		VarServerMaxPlayers = AddVariableInt("MaxPlayers", "maxplayers", "Maximum number of connected players", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsRunOnMainMenu), 16, VariableServerMaxPlayersUpdate);
		VarServerMaxPlayers->ValueIntMin = 1;
		VarServerMaxPlayers->ValueIntMax = 16;

		VarServerCheats = AddVariableInt("Cheats", "sv_cheats", "Allows/blocks using cheat commands", eCommandFlagsReplicated, 0);
		VarServerCheats->ValueIntMin = 0;
		VarServerCheats->ValueIntMax = 1;

		AddCommand("Connect", "connect", "Begins establishing a connection to a server", eCommandFlagsRunOnMainMenu, CommandServerConnect, { "host:port The server info to connect to", "password(string) The password for the server" });

		AddCommand("AnnounceStats", "announcestats", "Announces the players stats to the masters at the end of the game", eCommandFlagsNone, CommandServerAnnounceStats);
	}
}