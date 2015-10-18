#include <winsock2.h>
#include <ws2tcpip.h>
#include "ModuleIRC.hpp"
#include <algorithm>

Modules::ModuleIRC IRCModule;
IUtils* PublicUtils;

namespace
{
	void IRCConnect_impl()
	{
		IRCModule.Connect();
	}

	void CallbackIRCConnect(void* param)
	{
		if (IRCModule.Connected)
			return;

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&IRCConnect_impl, 0, 0, 0);
	}

	void CallbackServerStart(void* param)
	{
		// create an IRC channel for this game
		std::string xnkid;
		PublicUtils->BytesToHexString((char*)Pointer(0x2247b80), 0x10, xnkid);
		IRCModule.ChannelJoin("#eldoritogame-" + xnkid, false);
	}

	void CallbackServerStop(void* param)
	{
		if (!IRCModule.GameChatChannel.empty())
		{
			IRCModule.ChannelLeave(IRCModule.GameChatChannel);
			// TODO5: kick everyone from the channel
		}
	}

	void CallbackServerPlayerKick(void* param)
	{
		PlayerInfo* playerInfo = reinterpret_cast<PlayerInfo*>(param);
		IRCModule.UserKick(IRCModule.GenerateIRCNick(playerInfo->Name, playerInfo->UID));
	}

	void CallbackGameJoining(void* param)
	{
		// join the servers IRC channel
		std::string xnkid;
		PublicUtils->BytesToHexString((char*)Pointer(0x2240BB4), 0x10, xnkid);
		IRCModule.ChannelJoin("#eldoritogame-" + xnkid, false);
	}

	void CallbackGameLeave(void* param)
	{
		if (!IRCModule.GameChatChannel.empty())
			IRCModule.ChannelLeave(IRCModule.GameChatChannel);
	}

	void CallbackPlayerChangeName(void* param)
	{
		IRCModule.ChangeNick(((Command*)param)->ValueString);
	}

	bool CommandIRCConnect(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		CallbackIRCConnect(0);
		return true;
	}

	void ChatMsgSend(const std::string& input, ConsoleBuffer* buffer)
	{
		std::string destChannel = !buffer->Name.compare("Global Chat") ? IRCModule.GlobalChatChannel : IRCModule.GameChatChannel;
		if (destChannel.empty() || IRCModule.IRCNick.empty())
			return;

		IRCModule.ChannelSendMsg(destChannel, input);

		std::string preparedLine = IRCModule.IRCNick;
		preparedLine = preparedLine.substr(preparedLine.find_first_of("|") + 1, std::string::npos);
		preparedLine += ": ";
		preparedLine += input;
		buffer->PushLine(preparedLine);
	}

	std::vector<std::string>& split(const std::string& s, char delim, std::vector<std::string>& elems, bool keepDelimiter)
	{
		std::stringstream ss(s);
		std::string item;
		while (std::getline(ss, item, delim))
			if (keepDelimiter)
				elems.push_back(item + delim);
			else
				elems.push_back(item);

		return elems;
	}
}

namespace Modules
{
	ModuleIRC::ModuleIRC() : ModuleBase("IRC")
	{
		PublicUtils = utils;

		engine->OnEvent("Core", "Engine.FirstTick", CallbackIRCConnect);
		engine->OnEvent("Core", "Server.Start", CallbackServerStart);
		engine->OnEvent("Core", "Server.Stop", CallbackServerStop);
		engine->OnEvent("Core", "Server.PlayerKick", CallbackServerPlayerKick);
		engine->OnEvent("Core", "Game.Joining", CallbackGameJoining);
		engine->OnEvent("Core", "Game.Leave", CallbackGameLeave);
		engine->OnEvent("Core", "Player.ChangeName", CallbackPlayerChangeName);

		VarIRCServer = AddVariableString("Server", "irc_server", "The IRC server for the global and game chats", eCommandFlagsArchived, "irc.snoonet.org");

		VarIRCServerPort = AddVariableInt("ServerPort", "irc_serverport", "The IRC server port", eCommandFlagsArchived, 6667);
		VarIRCServerPort->ValueIntMin = 1;
		VarIRCServerPort->ValueIntMax = 0xFFFF;

		VarIRCGlobalChannel = AddVariableString("GlobalChannel", "irc_globalchan", "The IRC channel for global chat", eCommandFlagsArchived, "#haloonline");
#ifdef _DEBUG
		VarIRCGlobalChannel->DefaultValueString = "#haloonline-debug";
		VarIRCGlobalChannel->ValueString = "#haloonline-debug";
#endif

		AddCommand("Connect", "irc_connect", "Begins connecting to IRC", eCommandFlagsNone, CommandIRCConnect);

		globalBuffer = engine->AddConsoleBuffer(ConsoleBuffer("Global Chat", "Chat", ChatMsgSend, true));
		ingameBuffer = engine->AddConsoleBuffer(ConsoleBuffer("Game Chat", "Chat", ChatMsgSend, false));
	}

	void ModuleIRC::Connect()
	{
		Connected = true;

		for (int i = 0; !initIRCChat(); i++)
		{
			if (i >= 2)
			{
				globalBuffer->PushLine("Error: failed to connect to IRC.");
				closesocket(winSocket);
				Connected = false;
				return;
			}
			else
			{
				globalBuffer->PushLine("Error: failed to connect to IRC. Retrying in 5 seconds.");
				Sleep(5000);
			}
		}

		for (int i = 0;; i++)
		{
			ircChatLoop();

			if (i >= 2)
			{
				globalBuffer->PushLine("Error: failed to loop in IRC.");
				break;
			}
			else
			{
				globalBuffer->PushLine("Error: failed to loop in IRC. Retrying in 5 seconds.");
				Sleep(5000);
			}
		}
		closesocket(winSocket);
		Connected = false;
	}

	bool ModuleIRC::initIRCChat()
	{
		int retVal;

		struct addrinfo hints, *ai;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (retVal = getaddrinfo(VarIRCServer->ValueString.c_str(), VarIRCServerPort->ValueString.c_str(), &hints, &ai))
		{
			globalBuffer->PushLine("IRC GAI error: " + std::string(gai_strerrorA(retVal)) + " (" + std::to_string(retVal) + "/" + std::to_string(WSAGetLastError()) + ")");
			return false;
		}
		winSocket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (retVal = connect(winSocket, ai->ai_addr, ai->ai_addrlen))
		{
			globalBuffer->PushLine("IRC connect error: " + std::string(gai_strerrorA(retVal)) + " (" + std::to_string(retVal) + "/" + std::to_string(WSAGetLastError()) + ")");
			return false;
		}
		freeaddrinfo(ai);

		sprintf_s(buffer, "USER %s 0 * :#ElDorito player\r\n", IRCNick.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
		commands->GetVariableString("Player.Name", IRCNick);
		ChangeNick(IRCNick);
		return true;
	}

	void ModuleIRC::ircChatLoop()
	{
		int inDataLength;

		while ((inDataLength = recv(winSocket, buffer, 512, 0)) > 0) { // received a packet from IRC server
			buffer[inDataLength] = '\0'; // recv function doesn't put a null-terminator character

			std::vector<std::string> bufferSplitByNewLines;
			split(buffer, '\n', bufferSplitByNewLines, true);

			for (size_t i = 0; i < bufferSplitByNewLines.size(); i++)
			{
				// OutputDebugString(bufferSplitByNewLines.at(i).c_str()); // use this line to debug IRC backend

				if (receivedPING(bufferSplitByNewLines.at(i)))
				{
					std::string sendBuffer = bufferSplitByNewLines.at(i);
					sendBuffer[1] = 'O'; // modify PING to PONG
					send(winSocket, sendBuffer.c_str(), strlen(sendBuffer.c_str()), 0);
				}

				std::vector<std::string> bufferSplitBySpace;
				split(bufferSplitByNewLines.at(i), ' ', bufferSplitBySpace, false);

				if (receivedWelcomeMessage(bufferSplitBySpace))
				{
					ChannelJoin(VarIRCGlobalChannel->ValueString, true);
					globalBuffer->PushLine("Connected to global chat!");
				}
				else if (receivedChannelTopic(bufferSplitBySpace))
				{
					if (messageIsInChannel(bufferSplitBySpace, GlobalChatChannel, 3))
					{
						printMessageIntoBuffer(bufferSplitBySpace, globalBuffer, 4, true);
					}
				}
				else if (receivedMessageFromIRCServer(bufferSplitBySpace))
				{
					if (messageIsInChannel(bufferSplitBySpace, GlobalChatChannel))
						printMessageIntoBuffer(bufferSplitBySpace, globalBuffer);
					else if (messageIsInChannel(bufferSplitBySpace, GameChatChannel))
						printMessageIntoBuffer(bufferSplitBySpace, ingameBuffer);
				}
				else if (bufferSplitByNewLines.at(i).find("Erroneous Nickname") != std::string::npos)
				{
					globalBuffer->PushLine("Error: invalid username.");
				}
			}
		}

		int nError = WSAGetLastError();
		std::string errorString("Winsock error code: ");
		errorString.append(std::to_string(nError));
		globalBuffer->PushLine(errorString);
	}

	void ModuleIRC::ChannelSendMsg(const std::string& channel, const std::string& line)
	{
		sprintf_s(buffer, "PRIVMSG %s :%s\r\n", channel.c_str(), line.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	void ModuleIRC::ChannelJoin(const std::string& channel, bool globalChat)
	{
		auto newChannel = utils->ToLower(channel);

		if (globalChat)
			GlobalChatChannel = newChannel;
		else
		{
			if (!GameChatChannel.empty())
				ChannelLeave(GameChatChannel);
			GameChatChannel = newChannel;
			engine->SetActiveConsoleBuffer(ingameBuffer);
			ingameBuffer->Visible = true;
		}

		sprintf_s(buffer, "MODE %s %s\r\nJOIN %s\r\n", IRCNick.c_str(), userMode.c_str(), newChannel.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	void ModuleIRC::ChannelLeave(const std::string& channel)
	{
		auto newChannel = utils->ToLower(channel);

		sprintf_s(buffer, "MODE %s %s\r\nPART %s\r\n", IRCNick.c_str(), userMode.c_str(), newChannel.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
		GameChatChannel = "";
		engine->SetActiveConsoleBuffer(globalBuffer);
		ingameBuffer->Visible = false;
	}

	void ModuleIRC::UserKick(const std::string& nick)
	{
		if (GameChatChannel.length() <= 0)
			return;

		sprintf_s(buffer, "KICK %s %s\r\n", GameChatChannel.c_str(), nick.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	void ModuleIRC::ChangeNick(const std::string& nick)
	{
		IRCNick = GenerateIRCNick(nick, Pointer(0x19AB730).Read<uint64_t>());
		sprintf_s(buffer, "NICK %s\r\n", IRCNick.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	bool ModuleIRC::receivedWelcomeMessage(std::vector<std::string> &bufferSplitBySpace)
	{
		if (bufferSplitBySpace.size() <= 1)
			return false;
		return strncmp(bufferSplitBySpace.at(1).c_str(), "001", 3) == 0;
	}

	bool ModuleIRC::receivedChannelTopic(std::vector<std::string> &bufferSplitBySpace)
	{
		if (bufferSplitBySpace.size() <= 1)
			return false;
		return strncmp(bufferSplitBySpace.at(1).c_str(), "332", 3) == 0;
	}

	bool ModuleIRC::receivedMessageFromIRCServer(std::vector<std::string> &bufferSplitBySpace)
	{
		if (bufferSplitBySpace.size() <= 1)
			return false;
		return strncmp(bufferSplitBySpace.at(1).c_str(), "PRIVMSG", 7) == 0;
	}

	bool ModuleIRC::receivedPING(const std::string& line)
	{
		return strncmp(line.c_str(), "PING ", 5) == 0;
	}

	bool ModuleIRC::messageIsInChannel(std::vector<std::string> &bufferSplitBySpace, const std::string& channel, size_t channelPos)
	{
		if (bufferSplitBySpace.size() <= channelPos)
			return false;
		std::transform(bufferSplitBySpace.at(channelPos).begin(), bufferSplitBySpace.at(channelPos).end(), bufferSplitBySpace.at(channelPos).begin(), ::tolower);
		return strncmp(bufferSplitBySpace.at(channelPos).c_str(), channel.c_str(), channel.length()) == 0;
	}

	void ModuleIRC::printMessageIntoBuffer(std::vector<std::string> &bufferSplitBySpace, ConsoleBuffer* buffer, size_t msgPos, bool topic)
	{
		if (bufferSplitBySpace.size() <= msgPos)
			return;
		std::string buf(this->buffer);
		std::string message = buf.substr(buf.find(bufferSplitBySpace.at(msgPos)), buf.length());
		message.erase(0, 1); // remove first character ":"
		message.resize(message.size() - 2); // remove last 2 characters "\n"
		if (message.find("\r\n") != std::string::npos)
			message = message.substr(0, message.find("\r\n"));

		std::string preparedLineForUI = bufferSplitBySpace.at(0).substr(bufferSplitBySpace.at(0).find_first_of("|") + 1, std::string::npos);
		preparedLineForUI = preparedLineForUI.substr(0, preparedLineForUI.find_first_of("!"));
		preparedLineForUI += ": " + message;
		if (topic)
			preparedLineForUI = "Channel topic: " + message;

		buffer->PushLine(preparedLineForUI);
	}

	std::string ModuleIRC::GenerateIRCNick(const std::string& name, uint64_t uid)
	{
		std::string ircNick;
		utils->BytesToHexString(&uid, sizeof(uint64_t), ircNick);
		ircNick += "|" + name;

		size_t maxLen = 27; // TODO: get max name len from server
		maxLen -= 3; // dew prefix

		if (ircNick.length() > maxLen)
			ircNick = ircNick.substr(ircNick.length() - maxLen, maxLen);

		ircNick = "new" + ircNick;
		return ircNick;
	}
}
