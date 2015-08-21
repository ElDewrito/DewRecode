#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include "ServerCommandProvider.hpp"
#include "../ElDorito.hpp"
#include <thread>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace Server
{

	std::vector<Command> ServerCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Server", "Connect", "connect", "Begins establishing a connection to a server", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &ServerCommandProvider::CommandConnect), { "host:port The server info to connect to", "password(string) The password for the server" }),
			Command::CreateCommand("Server", "AnnounceStats", "announcestats", "Announces the players stats to the masters at the end of the game", eCommandFlagsNone, BIND_COMMAND(this, &ServerCommandProvider::CommandAnnounceStats))
		};

		return commands;
	}

	void ServerCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarCheats = manager->Add(Command::CreateVariableInt("Server", "Cheats", "sv_cheats", "Allows/blocks using cheat commands", eCommandFlagsReplicated, 0));
		VarCheats->ValueIntMin = 0;
		VarCheats->ValueIntMax = 1;

		VarCountdown = manager->Add(Command::CreateVariableInt("Server", "Countdown", "countdown", "The number of seconds to wait at the start of the game", eCommandFlagsArchived, 5, BIND_COMMAND(this, &ServerCommandProvider::VariableCountdownUpdate)));
		VarCountdown->ValueIntMin = 0;
		VarCountdown->ValueIntMax = 20;

		VarMaxPlayers = manager->Add(Command::CreateVariableInt("Server", "MaxPlayers", "maxplayers", "Maximum number of connected players", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsRunOnMainMenu), 16, BIND_COMMAND(this, &ServerCommandProvider::VariableMaxPlayersUpdate)));
		VarMaxPlayers->ValueIntMin = 1;
		VarMaxPlayers->ValueIntMax = 16;

		VarMode = manager->Add(Command::CreateVariableInt("Server", "Mode", "mode", "Changes the game mode for the server. 0 = Xbox Live (Open Party); 1 = Xbox Live (Friends Only); 2 = Xbox Live (Invite Only); 3 = Online; 4 = Offline;", eCommandFlagsNone, 4, BIND_COMMAND(this, &ServerCommandProvider::CommandMode)));
		VarMode->ValueIntMin = 0;
		VarMode->ValueIntMax = 4;

		VarPort = manager->Add(Command::CreateVariableInt("Server", "Port", "server_port", "The port number for the HTTP info server, game itself uses a different one", eCommandFlagsArchived, 11784));
		VarPort->ValueIntMin = 1;
		VarPort->ValueIntMax = 0xFFFF;
	}

	void ServerCommandProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnEvent("Core", "Game.End", BIND_CALLBACK(this, &ServerCommandProvider::CallbackEndGame));
	}

	void ServerCommandProvider::CallbackEndGame(void* param)
	{
		AnnounceStats();
	}

	void ServerCommandProvider::announceStatsThread()
	{
		std::stringstream ss;
		std::vector<std::string> statsEndpoints;
		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(statsEndpoints, "stats");

		//typedef int(__cdecl *Game_GetLocalPlayerDatumIdxPtr)(int localPlayerIdx);
		//auto Game_GetLocalPlayerDatumIdx = reinterpret_cast<Game_GetLocalPlayerDatumIdxPtr>(0x589C30);
		//uint16_t playerIdx = (uint16_t)(Game_GetLocalPlayerDatumIdx(0) & 0xFFFF);
		// above wont work since we're on a different thread without the proper TLS data :(

		auto& localPlayers = dorito.Engine.GetMainTls(GameGlobals::LocalPlayers::TLSOffset)[0];
		uint16_t playerIdx = (uint16_t)(localPlayers(GameGlobals::LocalPlayers::Player0DatumIdx).Read<uint32_t>() & 0xFFFF);

		auto* playersGlobal = dorito.Engine.GetArrayGlobal(GameGlobals::Players::TLSOffset);
		auto playerPtr = playersGlobal->GetEntry(playerIdx);
		int32_t team = playerPtr(GameGlobals::Players::TeamOffset).Read<int32_t>();

		auto& playersGlobal2 = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];

		// TODO: find how this fits into the players global
		int16_t score = playersGlobal2(0x54 + GameGlobals::Players::ScoreBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		int16_t kills = playersGlobal2(0x54 + GameGlobals::Players::KillsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		int16_t deaths = playersGlobal2(0x54 + GameGlobals::Players::DeathsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();
		// unsure about assists
		int16_t assists = playersGlobal2(0x54 + GameGlobals::Players::AssistsBase + (playerIdx * GameGlobals::Players::ScoresEntryLength)).Read<int16_t>();

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
		std::string statsSignature;
		if (!dorito.Utils.RSACreateSignature(dorito.PlayerCommands->GetFormattedPrivKey(), (void*)statsObject.c_str(), statsObject.length(), statsSignature))
		{
			ss << "Failed to create stats RSA signature!";
			dorito.Logger.Log(LogSeverity::Error, "AnnounceStats", ss.str());
			return;
		}

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);
		writer.StartObject();
		writer.Key("statsVersion");
		writer.Int(1);
		writer.Key("stats");
		writer.String(statsObject.c_str()); // write stats object as a string instead of object so that the string matches up exactly with what we signed (also because there's no easy way to append a writer..)
		writer.Key("publicKey");
		writer.String(dorito.PlayerCommands->VarPubKey->ValueString.c_str());
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
	}

	bool ServerCommandProvider::CommandAnnounceStats(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		// TODO1: check if they're in an online game
		AnnounceStats();
		return true;
	}

	void ServerCommandProvider::AnnounceStats()
	{
		std::thread thread(&ServerCommandProvider::announceStatsThread, this);
	}

	bool ServerCommandProvider::CommandConnect(const std::vector<std::string>& Arguments, std::string& returnInfo)
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

		struct addrinfo* ptr = NULL;
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
		memset(SyslinkData, 0, 0x176);
		*(uint32_t*)SyslinkData = 1;

		memcpy(SyslinkData + 0x9E, xnetInfo, 0x20);

		*(uint32_t*)(SyslinkData + 0x170) = rawIpaddr;
		*(uint16_t*)(SyslinkData + 0x174) = gamePort;

		// set syslink stuff to point at our syslink data
		Pointer(0x228E6D8).Write<uint32_t>(1);
		Pointer(0x228E6DC).Write<uint32_t>((uint32_t)&SyslinkData);

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

	void ServerCommandProvider::Connect()
	{
		// TODO: move stuff from CommandConnect into here
	}

	bool ServerCommandProvider::CommandMode(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto retVal = SetLobbyMode((Blam::ServerLobbyMode)VarMode->ValueInt);
		if (retVal)
		{
			returnInfo = "Changed game mode to " + Blam::ServerLobbyModeNames[VarMode->ValueInt];
			return true;
		}

		returnInfo = "Server mode change failed, are you at the main menu?";
		return false;
	}

	bool ServerCommandProvider::SetLobbyMode(Blam::ServerLobbyMode mode)
	{
		typedef bool(__cdecl *Lobby_SetNetworkModePtr)(int mode);
		auto Lobby_SetNetworkMode = reinterpret_cast<Lobby_SetNetworkModePtr>(0xA7F950);
		return Lobby_SetNetworkMode((int)mode);
	}

	bool ServerCommandProvider::VariableMaxPlayersUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (!SetMaxPlayers(VarMaxPlayers->ValueInt))
		{
			returnInfo = "Failed to update max player count, are you hosting a lobby?";
			return false;
		}

		return true;
	}

	bool ServerCommandProvider::SetMaxPlayers(int maxPlayers)
	{
		if (maxPlayers < 1 || maxPlayers > 16)
			return false;

		typedef char(__cdecl *Network_squad_session_set_maximum_player_countPtr)(int count);
		auto network_squad_session_set_maximum_player_count = reinterpret_cast<Network_squad_session_set_maximum_player_countPtr>(0x439BA0);
		char ret = network_squad_session_set_maximum_player_count(maxPlayers);
		if (ret == 0)
			return false;

		VarMaxPlayers->ValueInt = maxPlayers;
		return true;
	}

	bool ServerCommandProvider::VariableCountdownUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		SetCountdown(VarCountdown->ValueInt); // shouldn't return false since range is checked by CommandManager

		return true;
	}

	bool ServerCommandProvider::SetCountdown(int seconds)
	{
		if (seconds < 0 || seconds > 20)
			return false;

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
}