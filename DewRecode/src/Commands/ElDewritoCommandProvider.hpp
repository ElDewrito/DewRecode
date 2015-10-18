#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace ElDewrito
{
	class ElDewritoCommandProvider : public ICommandProvider
	{
	public:
		virtual std::vector<Command> GetCommands() override;
		
		bool CommandHelp(const std::vector<std::string>& Arguments, ICommandContext& context);
		std::string Help(const std::string& commandName = "");

		bool CommandExecute(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool Execute(const std::string& commandFile, ICommandContext& context);

		bool CommandWriteConfig(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool WriteConfig(const std::string& prefsFileName = "dewrito_prefs.cfg");
	};
}