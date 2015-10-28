#include <winsock2.h>
#include <ws2tcpip.h>
#include "IRCCommandProvider.hpp"
#include "../ElDorito.hpp"
#include <algorithm>

IRC::IRCCommandProvider* ircCmds;

namespace
{
	void IRCConnect_impl()
	{
		ircCmds->Connect();
	}
}

namespace IRC
{
	IRCCommandProvider::IRCCommandProvider()
	{
		ircCmds = this;
	}

	std::vector<Command> IRCCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("IRC", "Connect", "irc_connect", "Begins connecting to IRC", eCommandFlagsNone, BIND_COMMAND(this, &IRCCommandProvider::CommandConnect)),
			Command::CreateCommand("IRC", "SendMessage", "irc_send", "Sends a message to the games global IRC channel", eCommandFlagsNone, BIND_COMMAND(this, &IRCCommandProvider::CommandSendMessage)),
		};

		return commands;
	}

	void IRCCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarServer = manager->Add(Command::CreateVariableString("IRC", "Server", "irc_server", "The IRC server for the global and game chats", eCommandFlagsArchived, "irc.justsomegamers.com"));

		VarServerPort = manager->Add(Command::CreateVariableInt("IRC", "ServerPort", "irc_serverport", "The IRC server port", eCommandFlagsArchived, 6667));
		VarServerPort->ValueIntMin = 1;
		VarServerPort->ValueIntMax = 0xFFFF;

		VarGlobalChannel = manager->Add(Command::CreateVariableString("IRC", "GlobalChannel", "irc_globalchan", "The IRC channel for global chat", eCommandFlagsArchived, "#haloonline"));
#ifdef _DEBUG
		VarGlobalChannel->DefaultValueString = "#haloonline-debug";
		VarGlobalChannel->ValueString = "#haloonline-debug";
#endif
	}

	void IRCCommandProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnEvent("Core", "Engine.FirstTick", BIND_CALLBACK(this, &IRCCommandProvider::CallbackServerConnect));
		engine->OnEvent("Core", "Player.ChangeName", BIND_CALLBACK(this, &IRCCommandProvider::CallbackPlayerChangeName));
	}

	bool IRCCommandProvider::CommandConnect(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		CallbackServerConnect(0);
		return false;
	}

	bool IRCCommandProvider::CommandSendMessage(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.empty())
		{
			context.WriteOutput("Usage: IRC.SendMessage [message]");
			return false;
		}

		SendMsg(Arguments[0]);
		return true;
	}

	bool IRCCommandProvider::SendMsg(const std::string& msg)
	{
		if (!connected)
			return false;
		sprintf_s(buffer, "PRIVMSG %s :%s\r\n", VarGlobalChannel->ValueString.c_str(), msg.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
		return true;
	}

	void IRCCommandProvider::CallbackServerConnect(void* param)
	{
		if (!connected)
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&IRCConnect_impl, 0, 0, 0);
	}

	void IRCCommandProvider::CallbackPlayerChangeName(void* param)
	{
		ChangeNick(((Command*)param)->ValueString);
	}

	void IRCCommandProvider::Connect()
	{
		auto& dorito = ElDorito::Instance();
		connected = true;

		for (int i = 0; !initIRCChat(); i++)
		{
			if (i >= 2)
			{
				dorito.UserInterface.AddToChat("(Failed to connect to IRC!)", true);
				closesocket(winSocket);
				connected = false;
				return;
			}
			else
			{
				dorito.UserInterface.AddToChat("(Failed to connect to IRC, retrying in 5 seconds...)", true);
				Sleep(5000);
			}
		}

		for (int i = 0;; i++)
		{
			ircChatLoop();

			if (i >= 2)
			{
				dorito.UserInterface.AddToChat("(Failed to run IRC loop!)", true);
				break;
			}
			else
			{
				dorito.UserInterface.AddToChat("(Failed to run IRC loop, retrying in 5 seconds...)", true);
				Sleep(5000);
			}
		}

		closesocket(winSocket);
		connected = false;
	}

	bool IRCCommandProvider::initIRCChat()
	{
		auto& dorito = ElDorito::Instance();
		int retVal;

		struct addrinfo hints, *ai;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (retVal = getaddrinfo(VarServer->ValueString.c_str(), VarServerPort->ValueString.c_str(), &hints, &ai))
		{
			dorito.UserInterface.AddToChat("(IRC GAI error: " + std::string(gai_strerrorA(retVal)) + " (" + std::to_string(retVal) + "/" + std::to_string(WSAGetLastError()) + "))", true);
			return false;
		}
		winSocket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (retVal = connect(winSocket, ai->ai_addr, ai->ai_addrlen))
		{
			dorito.UserInterface.AddToChat("(IRC connect error: " + std::string(gai_strerrorA(retVal)) + " (" + std::to_string(retVal) + "/" + std::to_string(WSAGetLastError()) + "))", true);
			return false;
		}
		freeaddrinfo(ai);

		dorito.CommandManager.GetVariableString("Player.Name", IRCNick);
		ChangeNick(IRCNick);

		sprintf_s(buffer, "USER %s 0 * :#ElDorito player\r\n", IRCNick.c_str());
		send(winSocket, buffer, strlen(buffer), 0);

		return true;
	}

	std::string GenerateIRCNick(const std::string& name, uint64_t uid)
	{
		std::string ircNick;
		ElDorito::Instance().Utils.BytesToHexString(&uid, sizeof(uint64_t), ircNick);
		ircNick += "|" + name;

		size_t maxLen = 27; // TODO: get max name len from server
		maxLen -= 3; // dew prefix

		if (ircNick.length() > maxLen)
			ircNick = ircNick.substr(ircNick.length() - maxLen, maxLen);

		ircNick = "new" + ircNick;
		return ircNick;
	}

	void IRCCommandProvider::ChangeNick(const std::string& nick)
	{
		IRCNick = GenerateIRCNick(nick, Pointer(0x19AB730).Read<uint64_t>());
		sprintf_s(buffer, "NICK %s\r\n", IRCNick.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	bool messageIsInChannel(std::vector<std::string> &bufferSplitBySpace, const std::string& channel, size_t channelPos)
	{
		if (bufferSplitBySpace.size() <= channelPos)
			return false;
		std::transform(bufferSplitBySpace.at(channelPos).begin(), bufferSplitBySpace.at(channelPos).end(), bufferSplitBySpace.at(channelPos).begin(), ::tolower);
		return strncmp(bufferSplitBySpace.at(channelPos).c_str(), channel.c_str(), channel.length()) == 0;
	}

	void IRCCommandProvider::printMessageToUI(std::vector<std::string> &bufferSplitBySpace, size_t msgPos, bool topic)
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

		ElDorito::Instance().UserInterface.AddToChat(preparedLineForUI, true);
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

	void IRCCommandProvider::ChannelJoin(const std::string& channel)
	{
		auto newChannel = ElDorito::Instance().Utils.ToLower(channel);

#ifdef _DEBUG
		std::string userMode = "+B";
#else
		std::string userMode = "+BIc";
#endif

		sprintf_s(buffer, "MODE %s %s\r\nJOIN %s\r\n", IRCNick.c_str(), userMode.c_str(), newChannel.c_str());
		send(winSocket, buffer, strlen(buffer), 0);
	}

	void IRCCommandProvider::ircChatLoop()
	{
		auto& dorito = ElDorito::Instance();
		int inDataLength;

		while ((inDataLength = recv(winSocket, buffer, 512, 0)) > 0) // received a packet from IRC server
		{
			buffer[inDataLength] = '\0'; // recv function doesn't put a null-terminator character

			std::vector<std::string> bufferSplitByNewLines;
			split(buffer, '\n', bufferSplitByNewLines, true);

			for (size_t i = 0; i < bufferSplitByNewLines.size(); i++)
			{
				// OutputDebugString(bufferSplitByNewLines.at(i).c_str()); // use this line to debug IRC backend

				if (!strncmp(bufferSplitByNewLines[i].c_str(), "PING ", 5))
				{
					std::string sendBuffer = bufferSplitByNewLines[i];
					sendBuffer[1] = 'O'; // modify PING to PONG
					send(winSocket, sendBuffer.c_str(), strlen(sendBuffer.c_str()), 0);
				}

				std::vector<std::string> bufferSplitBySpace;
				split(bufferSplitByNewLines[i], ' ', bufferSplitBySpace, false);

				if (bufferSplitBySpace.size() <= 1)
					continue;


				if (!strncmp(bufferSplitBySpace.at(1).c_str(), "001", 3)) // received welcome msg
				{
					ChannelJoin(VarGlobalChannel->ValueString);
					dorito.UserInterface.AddToChat("(Connected to global chat!)", true);
				}
				else if (!strncmp(bufferSplitBySpace.at(1).c_str(), "332", 3)) // received channel topic
				{
					if (messageIsInChannel(bufferSplitBySpace, VarGlobalChannel->ValueString, 3))
						printMessageToUI(bufferSplitBySpace, 4, true);
				}
				else if (!strncmp(bufferSplitBySpace.at(1).c_str(), "PRIVMSG", 7)) // received msg
				{
					if (messageIsInChannel(bufferSplitBySpace, VarGlobalChannel->ValueString, 2))
						printMessageToUI(bufferSplitBySpace);
				}
				else if (bufferSplitByNewLines[i].find("Erroneous Nickname") != std::string::npos)
				{
					dorito.UserInterface.AddToChat("(Global chat error: Invalid username)", true);
				}
			}
		}

		int nError = WSAGetLastError();
		std::string errorString("Winsock error code: ");
		errorString.append(std::to_string(nError));
		dorito.UserInterface.AddToChat("(" + errorString + ")", true);
	}
}