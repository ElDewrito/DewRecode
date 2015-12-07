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
			Command::CreateCommand("ElDewrito", "Execute", "execute", "Executes a list of commands", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandExecute), { "filename(string) The list of commands to execute" }),
			Command::CreateCommand("ElDewrito", "WriteConfig", "writeconfig", "Writes the ElDewrito config file", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandWriteConfig), { "filename(string) Optional, the filename to write the config to" }),

			Command::CreateCommand("ElDewrito", "Mods", "mods", "Allows mod installation/maintenance", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandMods)),

#ifdef _DEBUG
			Command::CreateCommand("ElDewrito", "BreakOn", "breakon", "Breaks on the given event", eCommandFlagsNone, BIND_COMMAND(this, &ElDewritoCommandProvider::CommandBreakOn), { "namespace(string) The events namespace", "name(string) The events name" })
#endif
		};

		return commands;
	}

	void ElDewritoCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarModList = manager->Add(Command::CreateVariableString("ElDewrito", "ModList", "modlist", "The list of enabled/disabled mods", eCommandFlagsArchived, ""));
	}

	bool ElDewritoCommandProvider::CommandMods(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		auto& dorito = ElDorito::Instance();

		if (Arguments.size() <= 0)
		{
			context.WriteOutput("Loaded mods:");
			
			int i = 0;
			for (auto mod : dorito.ModPackages)
			{
				std::stringstream ss;
				ss << "[" << i << "] " << mod->Name << " ";
				if (!mod->Version.empty())
					ss << "v" << mod->Version << "b" << mod->Build << " ";
				ss << (mod->Enabled ? "(enabled)" : "(disabled)");
				context.WriteOutput(ss.str());
				i++;
			}

			context.WriteOutput("Commands:");
			context.WriteOutput("Mods [enable/disable] [mod-idx] - enable/disable a mod based on its index");
			context.WriteOutput("Mods [enableID/disableID] [mod-id] - enable/disable a mod based on its ID");
			return true;
		}

		if (Arguments[0] == "disable" || Arguments[0] == "enable" || Arguments[0] == "disableID" || Arguments[0] == "enableID")
		{
			if (Arguments.size() <= 1)
			{
				context.WriteOutput("Usage: Mods " + Arguments[0] + " [mod-id]");
				return false;
			}

			auto idx = -1;
			std::string guid = "";
			if (Arguments[0] == "disableID" || Arguments[1] == "enableID")
				guid = Arguments[1];
			else
				idx = std::stoi(Arguments[1], 0, 0);

			int i = 0;
			ModPackage* package = nullptr;
			for (auto mod : dorito.ModPackages)
			{
				if ((idx != -1 && i == idx) || (guid != "" && mod->ID == guid))
				{
					mod->Enabled = Arguments[0] == "enable";
					package = mod;
					break;
				}
				i++;
			}

			bool disabled = Arguments[0] == "disable" || Arguments[0] == "disableID";

			// if they're using the ID funcs we'll update our disabled list now since we have the GUID
			if (Arguments[0] == "disableID" || Arguments[0] == "enableID")
				dorito.Engine.SetModEnabled(guid, !disabled);

			if (!package)
			{
				context.WriteOutput("Invalid mod id!");
				return true;
			}

			if (Arguments[0] == "disable" || Arguments[0] == "enable")
				dorito.Engine.SetModEnabled(package->ID, !disabled);

			context.WriteOutput((disabled ? "Disabled" : "Enabled") + std::string(" mod #") + std::to_string(i) + " (" + package->Name + ")");

			// update the config file in order to update our disabled mods list
			dorito.CommandManager.Execute("WriteConfig", context);
			return true;
		}

		return false;
	}

	bool ElDewritoCommandProvider::CommandHelp(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		std::string cmdName = "";
		if (Arguments.size() > 0)
			cmdName = Arguments[0];

		context.WriteOutput(Help(cmdName));
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

	bool ElDewritoCommandProvider::CommandExecute(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.size() <= 0)
		{
			context.WriteOutput("Usage: Execute <filename>");
			return false;
		}

		return Execute(Arguments[0], context);
	}

	bool ElDewritoCommandProvider::Execute(const std::string& commandFile, CommandContext& context)
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
			return ElDorito::Instance().CommandManager.ExecuteList(contents, context);
		}

		context.WriteOutput("Unable to open file " + commandFile + " for reading.");
		return false;
	}

	bool ElDewritoCommandProvider::CommandWriteConfig(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		std::string prefsName = "dewrito_prefs.cfg";
		if (Arguments.size() > 0)
			prefsName = Arguments[0];

		auto retVal = WriteConfig(prefsName);
		if (!retVal)
		{
			context.WriteOutput("Failed to write config to " + prefsName + "!");
			return false;
		}

		context.WriteOutput("Wrote config to " + prefsName);
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

#ifdef _DEBUG
	void ElDewritoCommandProvider::EventBreakpoint(void* param)
	{
		auto& dorito = ElDorito::Instance();
		for (auto cmd : breakCommands)
			dorito.CommandManager.Execute(cmd, dorito.CommandManager.LogFileContext);

		dorito.Logger.Log(LogSeverity::Debug, "BreakOnEvent", "Calling CrtDbgBreak for event!");
		DebugBreak();
	}

	bool ElDewritoCommandProvider::CommandBreakOn(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.size() < 2)
		{
			context.WriteOutput("Usage: BreakOn [options] <event-namespace> <event-name>");
			context.WriteOutput("Options: -c - add a command to be executed");
			context.WriteOutput("eg. BreakOn Core Engine.TagsLoaded");
			context.WriteOutput("BreakOn -c \"tag 0x1234\"");
			return false;
		}
		auto evtNamespace = Arguments[0];
		auto evtName = Arguments[1];
		if (evtNamespace == "-c" || evtNamespace == "/c")
		{
			auto command = evtName;
			for (size_t i = 2; i < Arguments.size(); i++)
				command += " " + Arguments[i];

			breakCommands.push_back(command);
			context.WriteOutput("Added break command \"" + command + "\"");
			return true;
		}
		ElDorito::Instance().Engine.OnEvent(evtNamespace, evtName, BIND_CALLBACK(this, &ElDewritoCommandProvider::EventBreakpoint));

		context.WriteOutput("Breaking on event " + evtNamespace + "." + evtName);
		return true;
	}
#endif
}