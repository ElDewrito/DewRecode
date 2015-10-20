#define _WINSOCK_DEPRECATED_NO_WARNINGS // TODO: investigate using WSAEventSelect() instead of using this define

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <thread>

#include "ServerCommandProvider.hpp"
#include "../ElDorito.hpp"

#include <ElDorito/Blam/BlamNetwork.hpp>
#include "../CommandContexts/RemoteConsoleContext.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace
{
	int GetNumPlayers();
}

namespace Server
{
	std::vector<Command> ServerCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Server", "Announce", "announce", "Announces this server to the master servers", eCommandFlagsMustBeHosting, BIND_COMMAND(this, &ServerCommandProvider::CommandAnnounce)),
			Command::CreateCommand("Server", "Unannounce", "unannounce", "Notifies the master servers to remove this server", eCommandFlagsMustBeHosting, BIND_COMMAND(this, &ServerCommandProvider::CommandUnannounce)),
			Command::CreateCommand("Server", "AnnounceStats", "announcestats", "Announces the players stats to the masters at the end of the game", eCommandFlagsNone, BIND_COMMAND(this, &ServerCommandProvider::CommandAnnounceStats)),
			Command::CreateCommand("Server", "Connect", "connect", "Begins establishing a connection to a server", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &ServerCommandProvider::CommandConnect), { "host:port The server info to connect to", "password(string) The password for the server" }),
			Command::CreateCommand("Server", "KickPlayer", "kick", "Kicks a player from the game (host only)", eCommandFlagsMustBeHosting, BIND_COMMAND(this, &ServerCommandProvider::CommandKickPlayer), { "playername/UID The name or UID of the player to kick" }),
			Command::CreateCommand("Server", "ListPlayers", "list", "Lists players in the game (currently host only)", eCommandFlagsMustBeHosting, BIND_COMMAND(this, &ServerCommandProvider::CommandListPlayers))
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

		VarName = manager->Add(Command::CreateVariableString("Server", "Name", "server_name", "The name of the server", eCommandFlagsArchived, "HaloOnline Server"));

		VarMaxPlayers = manager->Add(Command::CreateVariableInt("Server", "MaxPlayers", "maxplayers", "Maximum number of connected players", static_cast<CommandFlags>(eCommandFlagsDontUpdateInitial | eCommandFlagsArchived | eCommandFlagsRunOnMainMenu), 16, BIND_COMMAND(this, &ServerCommandProvider::VariableMaxPlayersUpdate)));
		VarMaxPlayers->ValueIntMin = 1;
		VarMaxPlayers->ValueIntMax = 16;

		VarMode = manager->Add(Command::CreateVariableInt("Server", "Mode", "mode", "Changes the game mode for the server. 0 = Xbox Live (Open Party); 1 = Xbox Live (Friends Only); 2 = Xbox Live (Invite Only); 3 = Online; 4 = Offline;", eCommandFlagsDontUpdateInitial, 4, BIND_COMMAND(this, &ServerCommandProvider::VariableModeUpdate)));
		VarMode->ValueIntMin = 0;
		VarMode->ValueIntMax = 4;

		VarLobbyType = manager->Add(Command::CreateVariableInt("Server", "LobbyType", "lobbytype", "Changes the lobby type for the server. 0 = Campaign; 1 = Matchmaking; 2 = Multiplayer; 3 = Forge; 4 = Theater;", eCommandFlagsDontUpdateInitial, 2, BIND_COMMAND(this, &ServerCommandProvider::VariableLobbyTypeUpdate)));
		VarLobbyType->ValueIntMin = 0;
		VarLobbyType->ValueIntMax = 4;

		VarPassword = manager->Add(Command::CreateVariableString("Server", "Password", "password", "The server password", eCommandFlagsArchived, ""));

		VarPort = manager->Add(Command::CreateVariableInt("Server", "Port", "server_port", "The port number for the HTTP info server, the game uses a different one", eCommandFlagsArchived, 11784));
		VarPort->ValueIntMin = 1;
		VarPort->ValueIntMax = 0xFFFF;

		VarShouldAnnounce = manager->Add(Command::CreateVariableInt("Server", "ShouldAnnounce", "should_announce", "Controls whether the server will be announced to the master servers", eCommandFlagsArchived, 1, BIND_COMMAND(this, &ServerCommandProvider::VariableShouldAnnounceUpdate)));
		VarShouldAnnounce->ValueIntMin = 0;
		VarShouldAnnounce->ValueIntMax = 1;

		VarRconPort = manager->Add(Command::CreateVariableInt("Server", "RconPort", "rcon_port", "The port number for the remote console", eCommandFlagsArchived, 2448));
		VarRconPort->ValueIntMin = 1;
		VarRconPort->ValueIntMax = 0xFFFF;

		VarRconPassword = manager->Add(Command::CreateVariableString("Server", "RconPassword", "rcon_password", "The password required for rcon connections", eCommandFlagsArchived, ""));
	}

	void ServerCommandProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnEvent("Core", "Game.End", BIND_CALLBACK(this, &ServerCommandProvider::CallbackEndGame));
		engine->OnEvent("Core", "Server.Start", BIND_CALLBACK(this, &ServerCommandProvider::CallbackInfoServerStart));
		engine->OnEvent("Core", "Server.Stop", BIND_CALLBACK(this, &ServerCommandProvider::CallbackInfoServerStop));
		engine->OnEvent("Core", "Engine.FirstTick", BIND_CALLBACK(this, &ServerCommandProvider::CallbackRemoteConsoleStart));
		engine->OnEvent("Core", "Server.PongReceived", BIND_CALLBACK(this, &ServerCommandProvider::CallbackPongReceived));
		engine->OnEvent("Core", "Server.LifeCycleStateChanged", BIND_CALLBACK(this, &ServerCommandProvider::CallbackLifeCycleStateChanged));

		engine->OnWndProc(BIND_WNDPROC(this, &ServerCommandProvider::WndProc));
	}

	void ServerCommandProvider::CallbackEndGame(void* param)
	{
		AnnounceStats();
	}

	void ServerCommandProvider::CallbackInfoServerStart(void* param)
	{
		if (infoServerRunning)
			return;

		auto& dorito = ElDorito::Instance();

		if (dorito.Engine.GetGameHWND() == 0)
			return;

		infoSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		SOCKADDR_IN bindAddr;
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		unsigned long serverPort = VarPort->ValueInt;

		unsigned long port = serverPort;

		if (port == Pointer(0x1860454).Read<uint32_t>()) // make sure port isn't the same as game port
			port++;

		bindAddr.sin_port = htons((u_short)port);

		// open our listener socket
		while (bind(infoSocket, (PSOCKADDR)&bindAddr, sizeof(bindAddr)) != 0)
		{
			port++;
			if (port == Pointer(0x1860454).Read<uint32_t>()) // make sure port isn't the same as game port
				port++;

			bindAddr.sin_port = htons((u_short)port);
			if (port > (serverPort + 10))
			{
				dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::CallbackInfoServerStart", "Failed to create socket for info server, no ports available?");
				return;
			}
		}
		VarPort->ValueInt = port;

		dorito.Logger.Log(LogSeverity::Debug, "ServerCommandProvider::CallbackInfoServerStart", "Created info socket on port %d", port);

		auto err1 = dorito.Utils.UPnPForwardPort(true, port, port, "DewritoInfoServer");
		uint32_t gamePort = Pointer(0x1860454).Read<uint32_t>();
		auto err2 = dorito.Utils.UPnPForwardPort(false, gamePort, gamePort, "DewritoGameServer");

		if (err1.ErrorType != UPnPErrorType::None)
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::CallbackInfoServerStart", "Failed to open info server port via UPnP!");

		if (err2.ErrorType != UPnPErrorType::None)
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::CallbackInfoServerStart", "Failed to open game server port via UPnP!");

		WSAAsyncSelect(infoSocket, dorito.Engine.GetGameHWND(), WM_INFOSERVER, FD_ACCEPT | FD_CLOSE);
		listen(infoSocket, 5);
		infoServerRunning = true;
	}

	void ServerCommandProvider::CallbackInfoServerStop(void* param)
	{
		if (!infoServerRunning)
			return;

		closesocket(infoSocket);
		int istrue = 1;
		setsockopt(infoSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&istrue, sizeof(int));

		if (VarShouldAnnounce->ValueInt == 1)
			UnannounceServer();

		infoServerRunning = false;
		lastAnnounce = 0;
	}

	void ServerCommandProvider::CallbackRemoteConsoleStart(void* param)
	{
		if (rconServerRunning)
			return;

		auto& dorito = ElDorito::Instance();

		if (dorito.Engine.GetGameHWND() == 0)
			return;

		rconSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		SOCKADDR_IN bindAddr;
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		//bindAddr.sin_port = htons(2448);

		unsigned long rconPort = VarRconPort->ValueInt;

		unsigned long port = rconPort;

		if (port == Pointer(0x1860454).Read<uint32_t>()) // make sure port isn't the same as game port
			port++;

		bindAddr.sin_port = htons((u_short)port);

		// open our listener socket
		while (bind(rconSocket, (PSOCKADDR)&bindAddr, sizeof(bindAddr)) != 0)
		{
			port++;
			if (port == Pointer(0x1860454).Read<uint32_t>()) // make sure port isn't the same as game port
				port++;

			bindAddr.sin_port = htons((u_short)port);
			if (port > (rconPort + 10))
			{
				dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::CallbackRemoteConsoleStart", "Failed to create socket for rcon server, no ports available?");
				return;
			}
		}
		VarRconPort->ValueInt = port;

		dorito.Logger.Log(LogSeverity::Debug, "ServerCommandProvider::CallbackRemoteConsoleStart", "Created rcon socket on port %d", port);

		WSAAsyncSelect(rconSocket, dorito.Engine.GetGameHWND(), WM_RCON, FD_ACCEPT | FD_CLOSE);
		listen(rconSocket, 5);
		rconServerRunning = true;
	}

	void ServerCommandProvider::announceServerThread()
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;
		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(announceEndpoints, "announce");
		if (announceEndpoints.size() <= 0)
			ss << "No announce endpoints found." << std::endl;

		for (auto server : announceEndpoints)
		{
			HttpRequest req;
			try
			{
				req = dorito.Utils.HttpSendRequest(dorito.Utils.WidenString(server + "?port=" + VarPort->ValueString), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(dorito.Engine.GetDoritoVersionString()), L"", L"", L"", NULL, 0);
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server " << server << " (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...) // TODO: find out what exception is being caused
			{
				ss << "Exception during master server announce request to " << server << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			std::wstring expected = L"HTTP/1.1 200 OK";
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server announce response from " << server << std::endl << std::endl;
				continue;
			}

			auto respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server announce response from " << server << std::endl << std::endl;
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
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::announceServerThread", errors);
	}

	bool ServerCommandProvider::CommandAnnounce(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		AnnounceServer();
		return true;
	}

	void ServerCommandProvider::AnnounceServer()
	{
		// TODO1: check if they're in an online game and hosting
		std::thread thread(&ServerCommandProvider::announceServerThread, this);
		thread.detach();
	}

	void ServerCommandProvider::unannounceServerThread()
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;
		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(announceEndpoints, "announce");
		if (announceEndpoints.size() <= 0)
			ss << "No announce endpoints found." << std::endl;

		for (auto server : announceEndpoints)
		{
			HttpRequest req;
			try
			{
				req = dorito.Utils.HttpSendRequest(dorito.Utils.WidenString(server + "?port=" + VarPort->ValueString + "&shutdown=true"), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(dorito.Engine.GetDoritoVersionString()), L"", L"", L"", NULL, 0);
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server " << server << " (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...)
			{
				ss << "Exception during master server unannounce request to " << server << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			std::wstring expected = L"HTTP/1.1 200 OK";
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server unannounce response from " << server << std::endl << std::endl;
				continue;
			}

			auto respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server unannounce response from " << server << std::endl << std::endl;
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
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::unannounceServerThread", errors);
	}

	bool ServerCommandProvider::CommandUnannounce(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		AnnounceServer();
		return true;
	}

	void ServerCommandProvider::UnannounceServer()
	{
		// TODO1: check if they're in an online game and hosting
		std::thread thread(&ServerCommandProvider::unannounceServerThread, this);
		thread.detach();
	}

	void ServerCommandProvider::announceStatsThread()
	{
		std::stringstream ss;
		std::vector<std::string> statsEndpoints;
		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(statsEndpoints, "stats");
		if (statsEndpoints.size() <= 0)
			ss << "No stats endpoints found." << std::endl;

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
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::announceStatsThread", ss.str());
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
			dorito.Logger.Log(LogSeverity::Error, "ServerCommandProvider::announceStatsThread", errors);
	}

	bool ServerCommandProvider::CommandAnnounceStats(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		// TODO1: check if they're in an online game
		AnnounceStats();
		return true;
	}

	void ServerCommandProvider::AnnounceStats()
	{
		std::thread thread(&ServerCommandProvider::announceStatsThread, this);
		thread.detach();
	}

	bool ServerCommandProvider::CommandConnect(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		// TODO: move this into a thread so that non-responding hosts don't lag the game
		if (Arguments.size() <= 0)
		{
			context.WriteOutput("Invalid arguments.");
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
			context.WriteOutput(ret);
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
			context.WriteOutput("Unable to lookup " + address + ": No records found.");
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
			context.WriteOutput("Unable to connect to server. (error: " + std::to_string((int)req.Error) + "/" + std::to_string(req.LastError) + "/" + std::to_string(GetLastError()) + ")");
			return false;
		}

		// make sure the server replied with 200 OK
		std::wstring expected = L"HTTP/1.1 200 OK";
		if (req.ResponseHeader.length() < expected.length())
		{
			context.WriteOutput("Invalid server query response.");
			return false;
		}

		auto respHdr = req.ResponseHeader.substr(0, expected.length());
		if (respHdr.compare(expected))
		{
			context.WriteOutput("Invalid server query header response.");
			return false;
		}

		// parse the json response
		std::string resp = std::string(req.ResponseBody.begin(), req.ResponseBody.end());

		rapidjson::Document json;
		if (json.Parse<0>(resp.c_str()).HasParseError() || !json.IsObject())
		{
			context.WriteOutput("Invalid server query JSON response.");
			return false;
		}

		// make sure the json has all the members we need
		if (!json.HasMember("gameVersion") || !json["gameVersion"].IsString() ||
			!json.HasMember("eldewritoVersion") || !json["eldewritoVersion"].IsString() ||
			!json.HasMember("port") || !json["port"].IsNumber())
		{
			context.WriteOutput("Server query JSON response is missing data.");
			return false;
		}

		if (!json.HasMember("xnkid") || !json["xnkid"].IsString() ||
			!json.HasMember("xnaddr") || !json["xnaddr"].IsString())
		{
			context.WriteOutput("Incorrect password specified.");
			return false;
		}

		std::string gameVer = json["gameVersion"].GetString();
		std::string edVer = json["eldewritoVersion"].GetString();

		std::string ourGameVer((char*)Pointer(0x199C0F0));
		std::string ourEdVer = Utils::Version::GetVersionString();
		if (ourGameVer.compare(gameVer))
		{
			context.WriteOutput("Server is running a different game version.");
			return false;
		}

		if (ourEdVer.compare(edVer))
		{
			context.WriteOutput("Server is running a different ElDewrito version.");
			return false;
		}

		std::string xnkid = json["xnkid"].GetString();
		std::string xnaddr = json["xnaddr"].GetString();
		if (xnkid.length() != (0x10 * 2) || xnaddr.length() != (0x10 * 2))
		{
			context.WriteOutput("Server query XNet info is invalid.");
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

		context.WriteOutput("Attempting connection to " + address + "...");
		return true;
	}

	void ServerCommandProvider::Connect()
	{
		// TODO: move stuff from CommandConnect into here
	}

	bool ServerCommandProvider::CommandKickPlayer(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		if (Arguments.size() <= 0)
		{
			context.WriteOutput("Invalid arguments.");
			return false;
		}

		auto kickPlayerName = Arguments[0];

		auto retVal = KickPlayer(kickPlayerName);
		switch (retVal)
		{
		case KickPlayerReturnCode::NoSession:
			context.WriteOutput("No session found, are you hosting a game?");
			break;
		case KickPlayerReturnCode::NotHost:
			context.WriteOutput("You must be hosting a game to use this command.");
			break;
		case KickPlayerReturnCode::KickFailed:
			context.WriteOutput("Failed to kick player " + kickPlayerName);
			break;
		case KickPlayerReturnCode::NotFound:
			context.WriteOutput("Player " + kickPlayerName + " not found in game?");
			break;
		case KickPlayerReturnCode::Success:
			context.WriteOutput("Issued kick request for player " + kickPlayerName + ".");
			break;
		}

		return retVal == KickPlayerReturnCode::Success;
	}

	KickPlayerReturnCode ServerCommandProvider::KickPlayer(const std::string& playerName)
	{
		auto& dorito = ElDorito::Instance();

		auto* session = dorito.Engine.GetActiveNetworkSession();
		if (!session || !session->IsEstablished())
			return KickPlayerReturnCode::NoSession;

		if (!session->IsHost())
			return KickPlayerReturnCode::NotHost;

		auto membership = &session->MembershipInfo;
		for (auto peerIdx = membership->FindFirstPeer(); peerIdx >= 0; peerIdx = membership->FindNextPeer(peerIdx))
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
			if (playerIdx == -1)
				continue;

			auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

			std::stringstream uidStream;
			uidStream << std::hex << player->Uid;
			auto uidString = uidStream.str();

			if (!dorito.Utils.Trim(dorito.Utils.ThinString(player->DisplayName)).compare(playerName) || !uidString.compare(playerName))
			{
				auto retVal = KickPlayer(peerIdx);
				if (retVal)
					return KickPlayerReturnCode::Success;
				else
					return KickPlayerReturnCode::KickFailed;
			}
		}

		return KickPlayerReturnCode::NotFound;
	}

	bool ServerCommandProvider::KickPlayer(int peerIdx)
	{
		typedef bool(__cdecl *Network_squad_session_boot_playerPtr)(int playerIdx, int reason);
		auto Network_squad_session_boot_player = reinterpret_cast<Network_squad_session_boot_playerPtr>(0x437D60);

		return Network_squad_session_boot_player(peerIdx, 4);
	}

	bool ServerCommandProvider::CommandListPlayers(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		std::stringstream ss;
		auto* session = dorito.Engine.GetActiveNetworkSession();
		if (!session || !session->IsEstablished())
		{
			context.WriteOutput("No session found, are you in a game?");
			return false;
		}

		context.WriteOutput(ListPlayers());
		return true;
	}

	std::string ServerCommandProvider::ListPlayers()
	{
		auto& dorito = ElDorito::Instance();

		std::stringstream ss;
		auto* session = dorito.Engine.GetActiveNetworkSession();
		if (!session || !session->IsEstablished())
		{
			return "No session found, are you in a game?";
		}

		int peerIdx = session->MembershipInfo.FindFirstPeer();
		while (peerIdx != -1)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
			if (playerIdx != -1)
			{
				auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

				std::string name = dorito.Utils.ThinString(player->DisplayName);
				ss << std::dec << "[" << playerIdx << "]: \"" << name << "\" (uid: " << std::hex << player->Uid << ")" << std::endl;
			}

			peerIdx = session->MembershipInfo.FindNextPeer(peerIdx);
		}

		return ss.str();
	}

	bool ServerCommandProvider::VariableModeUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto retVal = SetLobbyMode((Blam::ServerLobbyMode)VarMode->ValueInt);
		if (retVal)
		{
			context.WriteOutput("Changed game mode to " + Blam::ServerLobbyModeNames[VarMode->ValueInt]);
			return true;
		}

		context.WriteOutput("Server mode change failed, are you at the main menu?");
		return false;
	}

	bool ServerCommandProvider::SetLobbyMode(Blam::ServerLobbyMode mode)
	{
		typedef bool(__cdecl *Lobby_SetNetworkModePtr)(int mode);
		auto Lobby_SetNetworkMode = reinterpret_cast<Lobby_SetNetworkModePtr>(0xA7F950);
		return Lobby_SetNetworkMode((int)mode);
	}

	bool ServerCommandProvider::VariableLobbyTypeUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		auto retVal = SetLobbyType((Blam::ServerLobbyType)VarLobbyType->ValueInt);
		if (retVal)
		{
			context.WriteOutput("Changed lobby type to " + Blam::ServerLobbyTypeNames[VarLobbyType->ValueInt]);
			return true;
		}

		context.WriteOutput("Lobby type change failed, are you in a lobby?");
		return false;
	}

	bool ServerCommandProvider::SetLobbyType(Blam::ServerLobbyType type)
	{
		typedef bool(__cdecl *Lobby_SetLobbyTypePtr)(int type);
		auto Lobby_SetLobbyType = reinterpret_cast<Lobby_SetLobbyTypePtr>(0xA7EE70);
		return Lobby_SetLobbyType((int)type);
	}

	bool ServerCommandProvider::VariableCountdownUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
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

	bool ServerCommandProvider::VariableMaxPlayersUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		if (!SetMaxPlayers(VarMaxPlayers->ValueInt))
		{
			context.WriteOutput("Failed to update max player count, are you hosting a lobby?");
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

	bool ServerCommandProvider::VariableShouldAnnounceUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		SetShouldAnnounce(VarShouldAnnounce->ValueInt == 1); // shouldn't return false since range is checked by CommandManager

		//if (!VarShouldAnnounce->ValueInt)
			//TODO1: UnannounceServer();
		return true;
	}

	void ServerCommandProvider::SetShouldAnnounce(bool shouldAnnounce)
	{
		VarShouldAnnounce->ValueInt = shouldAnnounce ? 1 : 0;
	}

	bool ServerCommandProvider::CommandPing(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		if (Arguments.size() > 1)
		{
			context.WriteOutput("Invalid arguments");
			return false;
		}

		std::string addr = "";
		if (Arguments.size() > 0)
			addr = Arguments[0];
		
		return Ping(addr, context);
	}

	std::vector<ICommandContext*> pingContexts;

	bool ServerCommandProvider::Ping(const std::string& address, ICommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		auto session = dorito.Engine.GetActiveNetworkSession();
		if (!session)
		{
			context.WriteOutput("No session available");
			return false;
		}

		// If an IP address was passed, send to that address
		if (!address.empty())
		{
			std::string host;
			std::string port = "11774";
			auto seperatorIdx = address.find(":");
			if (seperatorIdx == std::string::npos)
				host = address;
			else
			{
				host = address.substr(0, seperatorIdx);
				if (address.length() > seperatorIdx + 1)
					port = address.substr(seperatorIdx + 1);
			}

			struct in_addr inAddr;
			if (!inet_pton(AF_INET, host.c_str(), &inAddr))
			{
				context.WriteOutput("Invalid IPv4 address");
				return false;
			}

			pingContexts.push_back(&context);
			auto pingId = (pingContexts.size() - 1) + 0xF00D;

			auto blamAddress = Blam::Network::NetworkAddress::FromInAddr(inAddr.S_un.S_addr, std::stoi(port));
			if (!session->Gateway->Ping(blamAddress, pingId))
			{
				context.WriteOutput("Failed to send ping packet");
				return false;
			}
			return true;
		}

		// Otherwise, send to the host
		if (!session->IsEstablished())
		{
			context.WriteOutput("You are not in a game. Use \"ping <ip>\" instead.");
			return false;
		}
		if (session->IsHost())
		{
			context.WriteOutput("You can't ping yourself. Use \"ping <ip>\" to ping someone else.");
			return false;
		}
		auto membership = &session->MembershipInfo;
		auto channelIndex = membership->PeerChannels[membership->HostPeerIndex].ChannelIndex;
		if (channelIndex == -1)
		{
			context.WriteOutput("You are not connected to a game. Use \"ping <ip>\" instead.");
			return false;
		}

		pingContexts.push_back(&context);
		auto pingId = (pingContexts.size() - 1) + 0xF00D;
		session->Observer->Ping(0, channelIndex, pingId);
		return true;
	}

	void ServerCommandProvider::CallbackPongReceived(void* param)
	{
		auto* tuple = (std::tuple<const Blam::Network::NetworkAddress&, uint32_t, uint16_t, uint32_t>*)param;
		const Blam::Network::NetworkAddress &from = std::get<0>(*tuple);
		uint32_t timestamp = std::get<1>(*tuple);
		uint16_t id = std::get<2>(*tuple);
		uint32_t latency = std::get<3>(*tuple);

		if (id < 0xF00D)
			return; // Only show pings sent by the ping command

		struct in_addr inAddr;
		inAddr.S_un.S_addr = from.ToInAddr();
		char ipStr[INET_ADDRSTRLEN];
		if (!inet_ntop(AF_INET, &inAddr, ipStr, sizeof(ipStr)))
			return;

		// PONG <ip> <timestamp> <latency>
		size_t idx = id - 0xF00D;
		if (pingContexts.size() + 1 < idx)
			return;

		auto& context = pingContexts[idx];
		context->WriteOutput("PONG " + std::string(ipStr) + " " + std::to_string(timestamp) + " " + std::to_string(latency) + "ms");
	}

	void ServerCommandProvider::CallbackLifeCycleStateChanged(void* param)
	{
		auto* state = (Blam::Network::LifeCycleState*)param;
		/*switch (*state)
		{
		case Blam::Network::LifeCycleState::None
			// Switch to global chat on the main menu
			//break;
		case Blam::Network::LifeCycleState::PreGame:
		case Blam::Network::LifeCycleState::InGame:
			// Switch to game chat when joining a game
			//break;
		}*/
	}

	LRESULT ServerCommandProvider::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!infoServerRunning && !rconServerRunning)
			return 0;

		auto& dorito = ElDorito::Instance();

		if (VarShouldAnnounce->ValueInt == 1 && infoServerRunning)
		{
			time_t curTime;
			time(&curTime);
			if (curTime - lastAnnounce > serverContactTimeLimit) // re-announce every "serverContactTimeLimit" seconds
			{
				lastAnnounce = curTime;
				AnnounceServer();
			}
		}

		if (msg != WM_RCON && msg != WM_INFOSERVER)
			return 0;

		if (WSAGETSELECTERROR(lParam))
		{
			closesocket((SOCKET)wParam);
			return 1;
		}

		SOCKET clientSocket;
		int inDataLength;
		char inDataBuffer[1024];
		bool isValidAscii = true;

		switch (WSAGETSELECTEVENT(lParam))
		{
		case FD_ACCEPT:
			// accept the connection and send our motd
			clientSocket = accept((SOCKET)wParam, NULL, NULL);
			WSAAsyncSelect(clientSocket, hwnd, msg, FD_READ | FD_WRITE | FD_CLOSE);
			if (msg == WM_RCON)
			{
				auto* context = new RemoteConsoleContext(clientSocket);
				RconContexts.push_back(context);
			}
			break;
		case FD_READ:
			ZeroMemory(inDataBuffer, sizeof(inDataBuffer));
			inDataLength = recv((SOCKET)wParam, (char*)inDataBuffer, sizeof(inDataBuffer) / sizeof(inDataBuffer[0]), 0);

			// check if text is proper ascii, because sometimes it's not
			for (int i = 0; i < inDataLength; i++)
			{
				char buf = inDataBuffer[i];
				if ((buf < 0x20 || buf > 0x7F) && buf != 0xD && buf != 0xA)
				{
					isValidAscii = false;
					break;
				}
			}

			if (isValidAscii)
			{
				if (msg == WM_RCON)
				{
					RemoteConsoleContext* rcon = 0;
					for (auto* context : RconContexts)
					{
						if (context->ClientSocket != wParam)
							continue;

						rcon = context;
						break;
					}
					if (!rcon)
						return 1;

					rcon->HandleInput(inDataBuffer);
				}
				else if (msg == WM_INFOSERVER)
				{
					std::string mapName((char*)Pointer(0x22AB018)(0x1A4));
					std::wstring mapVariantName((wchar_t*)Pointer(0x1863ACA));
					std::wstring variantName((wchar_t*)Pointer(0x23DAF4C));
					std::string xnkid;
					std::string xnaddr;
					std::string gameVersion((char*)Pointer(0x199C0F0));
					std::string status = "InGame";
					std::string playerName = dorito.PlayerCommands->VarName->ValueString;
					unsigned long maxPlayers = VarMaxPlayers->ValueInt;

					dorito.Utils.BytesToHexString((char*)Pointer(0x2247b80), 0x10, xnkid);
					dorito.Utils.BytesToHexString((char*)Pointer(0x2247b90), 0x10, xnaddr);

					Pointer &gameModePtr = dorito.Engine.GetMainTls(GameGlobals::GameInfo::TLSOffset)[0](GameGlobals::GameInfo::GameMode);
					uint32_t gameMode = gameModePtr.Read<uint32_t>();
					int32_t variantType = Pointer(0x023DAF18).Read<int32_t>();
					if (gameMode == 3)
					{
						if (mapName == "mainmenu")
						{
							status = "InLobby";
							// on mainmenu so we'll have to read other data
							mapName = std::string((char*)Pointer(0x19A5E49));
							variantName = std::wstring((wchar_t*)Pointer(0x179254));
							variantType = Pointer(0x179250).Read<uint32_t>();
						}
						else // TODO: find how to get the variant name/type while it's on the loading screen
							status = "Loading";
					}

					rapidjson::StringBuffer s;
					rapidjson::Writer<rapidjson::StringBuffer> writer(s);
					writer.StartObject();
					writer.Key("name");
					writer.String(VarName->ValueString.c_str());
					writer.Key("port");
					writer.Int(Pointer(0x1860454).Read<uint32_t>());
					writer.Key("hostPlayer");
					writer.String(playerName.c_str());
					writer.Key("map");
					writer.String(dorito.Utils.ThinString(mapVariantName).c_str());
					writer.Key("mapFile");
					writer.String(mapName.c_str());
					writer.Key("variant");
					writer.String(dorito.Utils.ThinString(variantName).c_str());
					if (variantType >= 0 && variantType < (uint32_t)Blam::GameType::Count)
					{
						writer.Key("variantType");
						writer.String(Blam::GameTypeNames[variantType].c_str());
					}
					writer.Key("status");
					writer.String(status.c_str());
					writer.Key("numPlayers");
					writer.Int(GetNumPlayers());

					// TODO: find how to get actual max players from the game, since our variable might be wrong
					writer.Key("maxPlayers");
					writer.Int(maxPlayers);

					bool authenticated = true;
					if (!VarPassword->ValueString.empty())
					{
						std::string authString = "dorito:" + VarPassword->ValueString;
						authString = "Authorization: Basic " + dorito.Utils.Base64Encode((const unsigned char*)authString.c_str(), authString.length()) + "\r\n";
						authenticated = std::string(inDataBuffer).find(authString) != std::string::npos;
					}

					if (authenticated)
					{
						writer.Key("xnkid");
						writer.String(xnkid.c_str());

						writer.Key("xnaddr");
						writer.String(xnaddr.c_str());

						writer.Key("variables");
						writer.StartArray();
						auto cmds = dorito.CommandManager.GetList();
						for (auto cmd : cmds)
						{
							if (!(cmd.Flags & eCommandFlagsReplicated))
								continue;
							writer.StartObject();
							writer.Key("name");
							writer.String(cmd.Name.c_str());
							writer.Key("value");
							writer.String(cmd.ValueString.c_str());
							writer.EndObject();
						}
						writer.EndArray();

						writer.Key("players");
						writer.StartArray();
						uint32_t playerScoresBase = 0x23F1724;
						//uint32_t playerInfoBase = 0x2162DD0;
						uint32_t playerInfoBase = 0x2162E08;
						uint32_t menuPlayerInfoBase = 0x1863B58;
						uint32_t playerStatusBase = 0x2161808;
						for (int i = 0; i < 16; i++)
						{
							uint16_t score = Pointer(playerScoresBase + (1080 * i)).Read<uint16_t>();
							uint16_t kills = Pointer(playerScoresBase + (1080 * i) + 4).Read<uint16_t>();
							uint16_t assists = Pointer(playerScoresBase + (1080 * i) + 6).Read<uint16_t>();
							uint16_t deaths = Pointer(playerScoresBase + (1080 * i) + 8).Read<uint16_t>();

							wchar_t* name = Pointer(playerInfoBase + (5696 * i));
							std::string nameStr = dorito.Utils.ThinString(name);

							wchar_t* menuName = Pointer(menuPlayerInfoBase + (0x1628 * i));
							std::string menuNameStr = dorito.Utils.ThinString(menuName);

							uint32_t ipAddr = Pointer(playerInfoBase + (5696 * i) - 88).Read<uint32_t>();
							uint16_t team = Pointer(playerInfoBase + (5696 * i) + 32).Read<uint16_t>();
							uint16_t num7 = Pointer(playerInfoBase + (5696 * i) + 36).Read<uint16_t>();

							uint8_t alive = Pointer(playerStatusBase + (176 * i)).Read<uint8_t>();

							if (menuNameStr.empty() && nameStr.empty() && ipAddr == 0)
								continue;

							writer.StartObject();
							writer.Key("name");
							writer.String(menuNameStr.c_str());
							writer.Key("score");
							writer.Int(score);
							writer.Key("kills");
							writer.Int(kills);
							writer.Key("assists");
							writer.Int(assists);
							writer.Key("deaths");
							writer.Int(deaths);
							writer.Key("team");
							writer.Int(team);
							writer.Key("isAlive");
							writer.Bool(alive == 1);
							writer.EndObject();
						}
						writer.EndArray();
					}
					else
					{
						writer.Key("passworded");
						writer.Bool(true);
					}

					writer.Key("gameVersion");
					writer.String(gameVersion.c_str());
					writer.Key("eldewritoVersion");
					writer.String(dorito.Engine.GetDoritoVersionString().c_str());
					writer.EndObject();

					std::string replyData = s.GetString();
					std::string reply = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nServer: ElDewrito/" + dorito.Engine.GetDoritoVersionString() + "\r\nContent-Length: " + std::to_string(replyData.length()) + "\r\nConnection: close\r\n\r\n" + replyData;
					send((SOCKET)wParam, reply.c_str(), reply.length(), 0);
				}
			}

			break;
		case FD_CLOSE:
			RemoteConsoleContext* rcon = 0;
			for (auto* context : RconContexts)
			{
				if (context->ClientSocket != wParam)
					continue;

				rcon = context;
				break;
			}

			if (!rcon)
				closesocket((SOCKET)wParam);
			else
				rcon->Disconnect();
			
			break;
		}
		return 1;
	}
}

namespace
{
	int GetNumPlayers()
	{
		void* v2;

		typedef char(__cdecl *sub_454F20Ptr)(void** a1);
		auto sub_454F20 = reinterpret_cast<sub_454F20Ptr>(0x454F20);
		if (!sub_454F20(&v2))
			return 0;

		typedef char*(__thiscall *sub_45C250Ptr)(void* thisPtr);
		auto sub_45C250 = reinterpret_cast<sub_45C250Ptr>(0x45C250);

		return *(DWORD*)(sub_45C250(v2) + 0x10A0);
	}
}