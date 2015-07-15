#include "PatchModuleServer.hpp"
#include "../NewDorito/src/Blam/BlamTypes.hpp"
#include "../NewDorito/src/ThirdParty/rapidjson/writer.h"
#include "../NewDorito/src/ThirdParty/rapidjson/stringbuffer.h"

Modules::PatchModuleServer ServerPatches;

namespace
{
	LRESULT __stdcall PluginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ServerPatches.WndProc(hWnd, msg, wParam, lParam);
	}

	void CallbackRemoteConsoleStart(void* param)
	{
		ServerPatches.RemoteConsoleStart();
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
}

namespace Modules
{
	PatchModuleServer::PatchModuleServer() : ModuleBase("Plugins.Patches.Server")
	{
		engine->OnWndProc(PluginWndProc);
		engine->OnEvent("Core", "Engine.FirstTick", CallbackRemoteConsoleStart);
		engine->OnEvent("Core", "Server.Start", CallbackInfoServerStart);
		engine->OnEvent("Core", "Server.Stop", CallbackInfoServerStop);
	}

	LRESULT PatchModuleServer::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!infoSocketOpen)
			return 0;

		unsigned long shouldAnnounce = 0;
		if (!commands->GetVariableInt("Server.ShouldAnnounce", shouldAnnounce))
		{
			logger->Log(LogLevel::Error, "ServerPlugin", "Failed to get Server.ShouldAnnounce variable?");
			return 0;
		}

		if (shouldAnnounce)
		{
			time_t curTime;
			time(&curTime);
			if (curTime - lastAnnounce > serverContactTimeLimit) // re-announce every "serverContactTimeLimit" seconds
			{
				lastAnnounce = curTime;
				commands->Execute("Server.Announce");
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
			clientSocket = accept(wParam, NULL, NULL);
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
					std::string serverName;
					std::string serverPassword;
					std::string playerName;
					unsigned long maxPlayers;

					// TODO: check return values of these? should be fine though
					commands->GetVariableString("Server.Name", serverName);
					commands->GetVariableString("Server.Password", serverPassword);
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
					writer.String(serverName.c_str());
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
					if (!serverPassword.empty())
					{
						std::string authString = "dorito:" + serverPassword;
						authString = "Authorization: Basic " + utils->Base64Encode((const unsigned char*)authString.c_str(), authString.length()) + "\r\n";
						authenticated = std::string(inDataBuffer).find(authString) != std::string::npos;
					}

					if (authenticated)
					{
						writer.Key("xnkid");
						writer.String(xnkid.c_str());
						writer.Key("xnaddr");
						writer.String(xnaddr.c_str());
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
			logger->Log(LogLevel::Error, "ServerPlugin", "Failed to get Server.Port variable?");
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
				logger->Log(LogLevel::Error, "ServerPlugin", "Failed to create socket for info server, no ports available?");
				return;
			}
		}
		commands->SetVariable("Server.Port", std::to_string(port), std::string());
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
			logger->Log(LogLevel::Error, "ServerPlugin", "Failed to get Server.ShouldAnnounce variable?");

		if (shouldAnnounce)
			commands->Execute("Server.Unannounce");

		infoSocketOpen = false;
		lastAnnounce = 0;
	}
}
