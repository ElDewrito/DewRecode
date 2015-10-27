#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>

namespace ElDewrito
{
	class ElDewritoCommandProvider : public CommandProvider
	{
	public:
		virtual std::vector<Command> GetCommands() override;
		
		bool CommandHelp(const std::vector<std::string>& Arguments, CommandContext& context);
		std::string Help(const std::string& commandName = "");

		bool CommandExecute(const std::vector<std::string>& Arguments, CommandContext& context);
		bool Execute(const std::string& commandFile, CommandContext& context);

		bool CommandWriteConfig(const std::vector<std::string>& Arguments, CommandContext& context);
		bool WriteConfig(const std::string& prefsFileName = "dewrito_prefs.cfg");
	};
}