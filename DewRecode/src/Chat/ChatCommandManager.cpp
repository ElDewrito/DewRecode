#include "ChatCommandManager.hpp"
#include "../ElDorito.hpp"
#include "../CommandContexts/ChatCommandContext.hpp"

namespace Chat
{
	bool ChatCommandManager::HostMessageReceived(Blam::Network::Session *session, int peer, const Chat::ChatMessage &message)
	{
		std::string body = message.Body;
		if (body.length() <= 0)
			return false;

		auto firstChar = body[0];
		if (firstChar == '!' || firstChar == '/')
			body = body.substr(1);

		// split command name from the rest
		auto cmdNameEndIdx = body.find_first_of(" ");
		if (cmdNameEndIdx == std::string::npos)
			cmdNameEndIdx = body.length();

		auto cmdName = body.substr(0, cmdNameEndIdx);

		// search our commands for the command name
		auto& dorito = ElDorito::Instance();
		auto* cmd = dorito.CommandManager.Find(cmdName);

		// make sure it's a valid chat command
		if (!cmd || !(cmd->Flags & eCommandFlagsChatCommand)) 
			return false;

		// execute it under a chat context
		ChatCommandContext ctx(session, peer);

		auto res = dorito.CommandManager.Execute(body, ctx, false);
		return res == CommandExecuteResult::Success; // if success then we return true so that the players input doesn't get forwarded to other players
	}
}