#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace ElDewrito
{
	class ElDewritoCommandProvider : public ICommandProvider
	{
	public:
		virtual std::vector<Command> GetCommands() override;
		
		bool CommandHelp(const std::vector<std::string>& Arguments, std::string& returnInfo);
		std::string Help(const std::string& commandName = "");

		bool CommandExecute(const std::vector<std::string>& Arguments, std::string& returnInfo);
		std::string Execute(const std::string& commandFile);

		bool CommandWriteConfig(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool WriteConfig(const std::string& prefsFileName = "dewrito_prefs.cfg");
	};
}