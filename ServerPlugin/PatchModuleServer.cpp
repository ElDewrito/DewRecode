#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <libwebsockets.h>
#include "PatchModuleServer.hpp"
#include <iostream>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <ElDorito/Blam/BlamTypes.hpp>
#include <ElDorito/Blam/BlamNetwork.hpp>

Modules::PatchModuleServer ServerPatches;
IUtils* PublicUtils;
IEngine* Engine;
IDebugLog* Logger;
ICommands* Commands;

namespace
{
	LRESULT __stdcall PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ServerPatches.WndProc(hWnd, msg, wParam, lParam);
	}

	static int callback_http(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
	{
		return 0;
	}

	static int callback_dew_rcon(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
	{
		//auto& console = GameConsole::Instance();
		switch (reason) {
		case LWS_CALLBACK_ESTABLISHED: // just log message that someone is connecting

			//console.consoleQueue.pushLineFromGameToUI("Websocket Connection Established!");
			break;
		case LWS_CALLBACK_RECEIVE: {
			//console.consoleQueue.pushLineFromGameToUI("received data:" + std::string((char *)in));

			std::string send = Commands->Execute(std::string((char *)in), true);

			unsigned char *sendChar = (unsigned char*)malloc(LWS_SEND_BUFFER_PRE_PADDING + send.length() + LWS_SEND_BUFFER_POST_PADDING);

			for (size_t i = 0; i < send.length(); i++) {
				sendChar[LWS_SEND_BUFFER_PRE_PADDING + i] = send.c_str()[i];
			}

			libwebsocket_write(wsi, &sendChar[LWS_SEND_BUFFER_PRE_PADDING], send.length(), LWS_WRITE_TEXT);

			// release memory back into the wild
			free(sendChar);
			break;
		}
		default:
			break;
		}

		return 0;
	}

	static struct libwebsocket_protocols protocols[] = {
		/* first protocol must always be HTTP handler */
		{
			"http-only",   // name
			callback_http, // callback
			0              // per_session_data_size
		},
		{
			"dew-rcon",			// protocol name - very important!
			callback_dew_rcon,	// callback
			0					// we don't use any per session data
		},
		{
			NULL, NULL, 0   /* End of list */
		}
	};

	DWORD WINAPI StartRconWebSocketServer(LPVOID)
	{
		// server url will be http://localhost:11776
		struct libwebsocket_context *context = nullptr;
		// create libwebsocket context representing this server
		int opts = 0;
		char interface_name[128] = "";
		const char * interfacez = NULL;

		//TODO: implement https/ssl later
		const char *cert_path = "/libwebsockets-test-server.pem";
		const char *key_path = "/libwebsockets-test-server.key.pem";

		//if (!use_ssl)
		cert_path = key_path = NULL;


		int port = ServerPatches.VarRconWSPort->ValueInt;
		int maxPort = port + 10;
		while (context == nullptr && maxPort > port)
		{
			context = libwebsocket_create_context(port, interfacez, protocols,
				libwebsocket_internal_extensions,
				cert_path, key_path, NULL, -1, -1, opts);
			port++;
		}

		if (context == nullptr)
		{
			//console.consoleQueue.pushLineFromGameToUI("libwebsocket init failed\n");
			return -1;
		}


		// infinite loop, to end this server send SIGTERM. (CTRL+C)
		while (1) {
			libwebsocket_service(context, 50);
			// libwebsocket_service will process all waiting events with their
			// callback functions and then wait 50 ms.
			// (this is a single threaded webserver and this will keep our server
			// from generating load while there are not requests to process)
		}

		libwebsocket_context_destroy(context);

		return 0;
	}

	void CallbackRemoteConsoleStart(void* param)
	{
		ServerPatches.RemoteConsoleStart();

		auto thread = CreateThread(NULL, 0, StartRconWebSocketServer, 0, 0, NULL);
	}

	void CallbackInfoServerStart(void* param)
	{
		ServerPatches.InfoServerStart();
	}

	void CallbackInfoServerStop(void* param)
	{
		ServerPatches.InfoServerStop();
	}

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

	bool CommandServerKickPlayer(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() <= 0)
		{
			returnInfo = "Invalid arguments";
			return false;
		}

		std::string kickPlayerName = Arguments[0];

		auto* session = Blam::Network::GetActiveSession();
		if (!session || !session->IsEstablished())
		{
			returnInfo = "No session found, are you hosting a game?";
			return false;
		}

		if (!session->IsHost())
		{
			returnInfo = "You must be hosting a game to use this command";
			return false;
		}

		int peerIdx = session->MembershipInfo.FindFirstPeer();
		while (peerIdx != -1)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
			if (playerIdx != -1)
			{
				auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

				std::stringstream uidStream;
				uidStream << std::hex << player->Uid;
				auto uidString = uidStream.str();

				if (!PublicUtils->Trim(PublicUtils->ThinString(player->DisplayName)).compare(kickPlayerName) || !uidString.compare(kickPlayerName))
				{
					typedef bool(__cdecl *Network_squad_session_boot_playerPtr)(int playerIdx, int reason);
					auto Network_squad_session_boot_player = reinterpret_cast<Network_squad_session_boot_playerPtr>(0x437D60);

					if (Network_squad_session_boot_player(peerIdx, 4))
					{
						returnInfo = "Issued kick request for player " + kickPlayerName + " (peer: " + std::to_string(peerIdx) + " player: " + std::to_string(playerIdx) + ")";
						return true;
					}
					else
					{
						returnInfo = "Failed to kick player " + kickPlayerName;
						return false;
					}
				}
			}

			peerIdx = session->MembershipInfo.FindNextPeer(peerIdx);
		}

		returnInfo = "Player " + kickPlayerName + " not found in game?";
		return false;
	}

	bool CommandServerListPlayers(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		std::stringstream ss;

		// TODO: check if player is in a lobby
		// TODO: find an addr where we can find this data in clients memory
		// so people could use it to find peoples UIDs and report them for cheating etc

		auto* session = Blam::Network::GetActiveSession();
		if (!session || !session->IsEstablished())
		{
			returnInfo = "No session found, are you hosting a game?";
			return false;
		}

		if (!session->IsHost())
		{
			returnInfo = "You must be hosting a game to use this command";
			return false;
		}

		int peerIdx = session->MembershipInfo.FindFirstPeer();
		while (peerIdx != -1)
		{
			int playerIdx = session->MembershipInfo.GetPeerPlayer(peerIdx);
			if (playerIdx != -1)
			{
				auto* player = &session->MembershipInfo.PlayerSessions[playerIdx];

				std::string name = PublicUtils->ThinString(player->DisplayName);
				ss << std::dec << "(" << peerIdx << "/" << playerIdx << "): " << name << " (uid: 0x" << std::hex << player->Uid << ")" << std::endl;
			}

			peerIdx = session->MembershipInfo.FindNextPeer(peerIdx);
		}

		returnInfo = ss.str();
		return true;
	}

	bool VariableServerShouldAnnounceUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (!ServerPatches.VarServerShouldAnnounce->ValueInt) // if we're setting Server.ShouldAnnounce to false unannounce ourselves too
			ServerPatches.Unannounce();

		return true;
	}

	// retrieves master server endpoints from dewrito.json
	// TODO1: move this to be part of IEngine?
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


	DWORD WINAPI CommandServerAnnounce_Thread(LPVOID lpParam)
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;

		GetEndpoints(announceEndpoints, "announce");

		for (auto server : announceEndpoints)
		{
			HttpRequest req;
			try
			{
				req = PublicUtils->HttpSendRequest(PublicUtils->WidenString(server + "?port=" + ServerPatches.VarServerPort->ValueString), L"GET", L"ElDewrito/" + PublicUtils->WidenString(Engine->GetDoritoVersionString()), L"", L"", L"", NULL, 0);
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
			Logger->Log(LogSeverity::Error, "Announce", ss.str());

		return true;
	}

	DWORD WINAPI CommandServerUnannounce_Thread(LPVOID lpParam)
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;

		GetEndpoints(announceEndpoints, "announce");

		for (auto server : announceEndpoints)
		{
			HttpRequest req;
			try
			{
				req = PublicUtils->HttpSendRequest(PublicUtils->WidenString(server + "?port=" + ServerPatches.VarServerPort->ValueString + "&shutdown=true"), L"GET", L"ElDewrito/" + PublicUtils->WidenString(Engine->GetDoritoVersionString()), L"", L"", L"", NULL, 0);
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
			Logger->Log(LogSeverity::Error, "Unannounce", ss.str());

		return true;
	}

	bool CommandServerAnnounce(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		// TODO2: 
		//if (!Patches::Network::IsInfoSocketOpen())
		//	return false;

		auto thread = CreateThread(NULL, 0, CommandServerAnnounce_Thread, (LPVOID)&Arguments, 0, NULL);
		returnInfo = "Announcing to master servers...";
		return true;
	}

	bool CommandServerUnannounce(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		// TODO2: 
		//if (!Patches::Network::IsInfoSocketOpen())
		//	return false;

		auto thread = CreateThread(NULL, 0, CommandServerUnannounce_Thread, (LPVOID)&Arguments, 0, NULL);
		returnInfo = "Unannouncing to master servers...";
		return true;
	}
}

namespace Modules
{
	PatchModuleServer::PatchModuleServer() : ModuleBase("Server")
	{
		PublicUtils = utils;
		Engine = engine;
		Logger = logger;
		Commands = commands;

		engine->OnWndProc(PluginWndProc);
		engine->OnEvent("Core", "Engine.FirstTick", CallbackRemoteConsoleStart);
		engine->OnEvent("Core", "Server.Start", CallbackInfoServerStart);
		engine->OnEvent("Core", "Server.Stop", CallbackInfoServerStop);

		VarServerName = AddVariableString("Name", "server_name", "The name of the server", eCommandFlagsArchived, "HaloOnline Server");
		VarServerPassword = AddVariableString("Password", "password", "The server password", eCommandFlagsArchived, "");

		VarServerShouldAnnounce = AddVariableInt("ShouldAnnounce", "should_announce", "Controls whether the server will be announced to the master servers", eCommandFlagsArchived, 1, VariableServerShouldAnnounceUpdate);
		VarServerShouldAnnounce->ValueIntMin = 0;
		VarServerShouldAnnounce->ValueIntMax = 1;

		AddCommand("KickPlayer", "kick", "Kicks a player from the game (host only)", eCommandFlagsMustBeHosting, CommandServerKickPlayer, { "playername/UID The name or UID of the player to kick" });
		AddCommand("ListPlayers", "list", "Lists players in the game (currently host only)", eCommandFlagsMustBeHosting, CommandServerListPlayers);

		VarServerPort = AddVariableInt("Port", "server_port", "The port number the HTTP server runs on, game uses different one", eCommandFlagsArchived, 11784);
		VarServerPort->ValueIntMin = 1;
		VarServerPort->ValueIntMax = 0xFFFF;

		VarRconWSPort = AddVariableInt("RconPort", "rcon_port", "The port number for the RCON/WebSockets server", eCommandFlagsArchived, 11764);
		VarRconWSPort->ValueIntMin = 1;
		VarRconWSPort->ValueIntMax = 0xFFFF;

		AddCommand("Announce", "announce", "Announces this server to the master servers", eCommandFlagsMustBeHosting, CommandServerAnnounce);
		AddCommand("Unannounce", "unannounce", "Notifies the master servers to remove this server", eCommandFlagsMustBeHosting, CommandServerUnannounce);
	}

	void PatchModuleServer::Announce()
	{
		commands->Execute("Server.Announce"); // until we move announcement stuff to this plugin
	}

	void PatchModuleServer::Unannounce()
	{
		commands->Execute("Server.Unannounce"); // until we move announcement stuff to this plugin
	}

	LRESULT PatchModuleServer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!infoSocketOpen)
			return 0;

		if (VarServerShouldAnnounce->ValueInt == 1)
		{
			time_t curTime;
			time(&curTime);
			if (curTime - lastAnnounce > serverContactTimeLimit) // re-announce every "serverContactTimeLimit" seconds
			{
				lastAnnounce = curTime;
				Announce();
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
			WSAAsyncSelect(clientSocket, hWnd, msg, FD_READ | FD_WRITE | FD_CLOSE);
			if (msg == WM_RCON)
			{
				std::string motd = "ElDewrito " + engine->GetDoritoVersionString() + " Remote Console\r\n";
				send(clientSocket, motd.c_str(), motd.length(), 0);
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
					auto ret = commands->Execute(inDataBuffer, true);
					if (ret.length() > 0)
					{
						utils->ReplaceString(ret, "\n", "\r\n");
						ret = ret + "\r\n";
						send((SOCKET)wParam, ret.c_str(), ret.length(), 0);
					}
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
					std::string playerName;
					unsigned long maxPlayers;

					// TODO: check return values of these? should be fine though
					commands->GetVariableString("Player.Name", playerName);
					commands->GetVariableInt("Server.MaxPlayers", maxPlayers);

					utils->BytesToHexString((char*)Pointer(0x2247b80), 0x10, xnkid);
					utils->BytesToHexString((char*)Pointer(0x2247b90), 0x10, xnaddr);

					Pointer &gameModePtr = engine->GetMainTls(GameGlobals::GameInfo::TLSOffset)[0](GameGlobals::GameInfo::GameMode);
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
					writer.String(VarServerName->ValueString.c_str());
					writer.Key("port");
					writer.Int(Pointer(0x1860454).Read<uint32_t>());
					writer.Key("hostPlayer");
					writer.String(playerName.c_str());
					writer.Key("map");
					writer.String(utils->ThinString(mapVariantName).c_str());
					writer.Key("mapFile");
					writer.String(mapName.c_str());
					writer.Key("variant");
					writer.String(utils->ThinString(variantName).c_str());
					if (variantType >= 0 && variantType < Blam::GameTypeCount)
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
					if (!VarServerPassword->ValueString.empty())
					{
						std::string authString = "dorito:" + VarServerPassword->ValueString;
						authString = "Authorization: Basic " + utils->Base64Encode((const unsigned char*)authString.c_str(), authString.length()) + "\r\n";
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
						auto cmds = commands->GetList();
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
							std::string nameStr = utils->ThinString(name);

							wchar_t* menuName = Pointer(menuPlayerInfoBase + (0x1628 * i));
							std::string menuNameStr = utils->ThinString(menuName);

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
					writer.String(engine->GetDoritoVersionString().c_str());
					writer.EndObject();

					std::string replyData = s.GetString();
					std::string reply = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nServer: ElDewrito/" + engine->GetDoritoVersionString() + "\r\nContent-Length: " + std::to_string(replyData.length()) + "\r\nConnection: close\r\n\r\n" + replyData;
					send((SOCKET)wParam, reply.c_str(), reply.length(), 0);
				}
			}

			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			break;
		}
		return 1;
	}

	void PatchModuleServer::RemoteConsoleStart()
	{
		if (rconSocketOpen)
			return;

		if (engine->GetGameHWND() == 0)
			return;

		rconSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		SOCKADDR_IN bindAddr;
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		bindAddr.sin_port = htons(2448);

		// open our listener socket
		bind(rconSocket, (PSOCKADDR)&bindAddr, sizeof(bindAddr));
		WSAAsyncSelect(rconSocket, engine->GetGameHWND(), WM_RCON, FD_ACCEPT | FD_CLOSE);
		listen(rconSocket, 5);
		rconSocketOpen = true;
	}

	void PatchModuleServer::InfoServerStart()
	{
		if (infoSocketOpen)
			return;

		if (engine->GetGameHWND() == 0)
			return;

		infoSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		SOCKADDR_IN bindAddr;
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);

		unsigned long serverPort = 0;
		if (!commands->GetVariableInt("Server.Port", serverPort))
		{
			logger->Log(LogSeverity::Error, "ServerPlugin", "Failed to get Server.Port variable?");
			return;
		}

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
				logger->Log(LogSeverity::Error, "ServerPlugin", "Failed to create socket for info server, no ports available?");
				return;
			}
		}
		commands->SetVariable("Server.Port", std::to_string(port), std::string());

		auto err1 = utils->UPnPForwardPort(true, port, port, "DewritoInfoServer");
		uint32_t gamePort = Pointer(0x1860454).Read<uint32_t>();
		auto err2 = utils->UPnPForwardPort(false, gamePort, gamePort, "DewritoGameServer");

		if (err1.ErrorType != UPnPErrorType::None)
			engine->PrintToConsole("Failed to open info server port via UPnP!"); // TODO: print in log instead

		if (err2.ErrorType != UPnPErrorType::None)
			engine->PrintToConsole("Failed to open game server port via UPnP!"); // TODO: print in log instead

		WSAAsyncSelect(infoSocket, engine->GetGameHWND(), WM_INFOSERVER, FD_ACCEPT | FD_CLOSE);
		listen(infoSocket, 5);
		infoSocketOpen = true;
	}

	void PatchModuleServer::InfoServerStop()
	{
		if (!infoSocketOpen)
			return;

		closesocket(infoSocket);
		int istrue = 1;
		setsockopt(infoSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&istrue, sizeof(int));
		unsigned long shouldAnnounce = 0;
		if (!commands->GetVariableInt("Server.ShouldAnnounce", shouldAnnounce))
			logger->Log(LogSeverity::Error, "ServerPlugin", "Failed to get Server.ShouldAnnounce variable?");

		if (shouldAnnounce)
			commands->Execute("Server.Unannounce");

		infoSocketOpen = false;
		lastAnnounce = 0;
	}
}
