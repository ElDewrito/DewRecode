#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>

namespace IRC
{
	class IRCCommandProvider : public CommandProvider
	{
	private:
		bool connected = false;
		SOCKET winSocket;
		std::string IRCNick;
		char buffer[513];

	public:
		Command* VarServer;
		Command* VarServerPort;
		Command* VarGlobalChannel;

		explicit IRCCommandProvider();

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		bool VariableRawInputUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableControllerIndexUpdate(const std::vector<std::string>& Arguments, CommandContext& context);

		bool CommandConnect(const std::vector<std::string>& Arguments, CommandContext& context);
		bool CommandSendMessage(const std::vector<std::string>& Arguments, CommandContext& context);
		bool SendMsg(const std::string& message);

		void Connect();
		void ChangeNick(const std::string& nick);
		bool initIRCChat();
		void ircChatLoop();
		void printMessageToUI(std::vector<std::string> &bufferSplitBySpace, size_t msgPos = 3, bool topic = false);
		void ChannelJoin(const std::string& channel);

		void CallbackServerConnect(void* param);
		void CallbackPlayerChangeName(void* param);
	};
}