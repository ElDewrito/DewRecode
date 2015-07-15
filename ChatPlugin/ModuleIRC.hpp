#pragma once
#include <ElDorito/ElDorito.hpp>

namespace Modules
{
	class ModuleIRC : public ModuleBase
	{
	public:
		Command* VarIRCServer;
		Command* VarIRCServerPort;
		Command* VarIRCGlobalChannel;
		std::string GlobalChatChannel = "";
		std::string GameChatChannel = "";
		bool Connected = false;
		std::string IRCNick;

		ModuleIRC();

		void Connect();
		void ChannelJoin(std::string channel, bool globalChat);
		void ChannelLeave(std::string channel);
		void ChannelSendMsg(std::string channel, std::string line);
		void UserKick(std::string nick);

		std::string GenerateIRCNick(std::string name, uint64_t uid);
	private:
		char buffer[513];
		SOCKET winSocket;
		ConsoleBuffer* globalBuffer;
		ConsoleBuffer* ingameBuffer;

		bool initIRCChat();
		void ircChatLoop();
		void printMessageIntoBuffer(std::vector<std::string>& bufferSplitBySpace, ConsoleBuffer* buffer, size_t msgPos = 3, bool topic = false);
		bool messageIsInChannel(std::vector<std::string>& bufferSplitBySpace, std::string channel, size_t channelPos = 2);
		bool receivedPING(std::string line);
		bool receivedMessageFromIRCServer(std::vector<std::string>& bufferSplitBySpace);
		bool receivedWelcomeMessage(std::vector<std::string>& bufferSplitBySpace);
		bool receivedChannelTopic(std::vector<std::string>& bufferSplitBySpace);
	};
}
