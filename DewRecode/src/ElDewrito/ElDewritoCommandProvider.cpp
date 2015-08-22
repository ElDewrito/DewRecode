#include "ElDewritoCommandProvider.hpp"
#include "../ElDorito.hpp"
#include <fstream>

namespace ElDewrito
{
	std::vector<Command> ElDewritoCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("ElDewrito", "Help", "help", "Displays this help text", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandHelp)),
			Command::CreateCommand("ElDewrito", "Execute", "exec", "Executes a list of commands", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandExecute), { "filename(string) The list of commands to execute" }),
			Command::CreateCommand("ElDewrito", "WriteConfig", "config_write", "Writes the ElDewrito config file", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandWriteConfig), { "filename(string) Optional, the filename to write the config to" })
		};

		return commands;
	}

	bool ElDewritoCommandProvider::CommandHelp(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		std::string cmdName = "";
		if (Arguments.size() > 0)
			cmdName = Arguments[0];

		returnInfo = Help(cmdName);
		return true;
	}

	std::string ElDewritoCommandProvider::Help(const std::string& commandName)
	{
		auto& dorito = ElDorito::Instance();

		if (commandName.empty())
			return dorito.CommandManager.GenerateHelpText();

		auto* cmd = dorito.CommandManager.Find(commandName);
		if (!cmd)
		{
			// try searching for it as a module
			bool isModule = false;
			for (auto it = dorito.CommandManager.List.begin(); it < dorito.CommandManager.List.end(); it++)
				if (!it->ModuleName.empty() && !_stricmp(it->ModuleName.c_str(), commandName.c_str()))
				{
					isModule = true;
					break;
				}

			if (isModule)
				return dorito.CommandManager.GenerateHelpText(commandName);
			else
				return "Command/Variable not found";
		}

		return dorito.CommandManager.GenerateHelpText(*cmd);
	}

	bool ElDewritoCommandProvider::CommandExecute(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() <= 0)
		{
			returnInfo = "Usage: Execute <filename>";
			return false;
		}

		returnInfo = Execute(Arguments[0]);
		return true;
	}

	std::string ElDewritoCommandProvider::Execute(const std::string& commandFile)
	{
		std::ifstream in(commandFile, std::ios::in | std::ios::binary);
		if (in && in.is_open())
		{
			std::string contents;
			in.seekg(0, std::ios::end);
			contents.resize((unsigned int)in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&contents[0], contents.size());
			in.close();
			return ElDorito::Instance().CommandManager.ExecuteList(contents, true);
		}
		return "Unable to open file " + commandFile + " for reading.";
	}

	bool ElDewritoCommandProvider::CommandWriteConfig(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		std::string prefsName = "dewrito_prefs.cfg";
		if (Arguments.size() > 0)
			prefsName = Arguments[0];

		auto retVal = WriteConfig(prefsName);
		if (!retVal)
		{
			returnInfo = "Failed to write config to " + prefsName + "!";
			return false;
		}

		returnInfo = "Wrote config to " + prefsName;
		return true;
	}

	bool ElDewritoCommandProvider::WriteConfig(const std::string& prefsFileName)
	{
		std::ofstream outFile(prefsFileName, std::ios::trunc);
		if (outFile.fail())
			return false;

		outFile << ElDorito::Instance().CommandManager.SaveVariables();

		return true;
	}
}