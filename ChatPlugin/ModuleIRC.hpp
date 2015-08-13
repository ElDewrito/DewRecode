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
		void ChannelJoin(const std::string& channel, bool globalChat);
		void ChannelLeave(const std::string& channel);
		void ChannelSendMsg(const std::string& channel, const std::string& line);
		void UserKick(const std::string& nick);
		void ChangeNick(const std::string& nick);

		std::string GenerateIRCNick(const std::string& name, uint64_t uid);
	private:
		char buffer[513];
		SOCKET winSocket;
		ConsoleBuffer* globalBuffer;
		ConsoleBuffer* ingameBuffer;
#ifdef _DEBUG
		std::string userMode = "+B";
#else
		std::string userMode = "+BIc";
#endif

		bool initIRCChat();
		void ircChatLoop();
		void printMessageIntoBuffer(std::vector<std::string>& bufferSplitBySpace, ConsoleBuffer* buffer, size_t msgPos = 3, bool topic = false);
		bool messageIsInChannel(std::vector<std::string>& bufferSplitBySpace, const std::string& channel, size_t channelPos = 2);
		bool receivedPING(const std::string& line);
		bool receivedMessageFromIRCServer(std::vector<std::string>& bufferSplitBySpace);
		bool receivedWelcomeMessage(std::vector<std::string>& bufferSplitBySpace);
		bool receivedChannelTopic(std::vector<std::string>& bufferSplitBySpace);
	};
}
