#include "DebugCommandProvider.hpp"
#include "../ElDorito.hpp"
#include <sstream>
#include <fstream>


namespace Debug
{
	DebugCommandProvider::DebugCommandProvider(std::shared_ptr<DebugPatchProvider> debugPatches)
	{
		this->debugPatches = debugPatches;
	}

	std::vector<Command> DebugCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Debug", "LogMode", "debug", "Chooses which debug messages to print to the log file", eCommandFlagsNone, BIND_COMMAND(this, &DebugCommandProvider::CommandLogMode), { "network|ssl|ui|game1|game2|all|off The log mode to enable" }),
			Command::CreateCommand("Debug", "LogFilter", "debug_filter", "Allows you to set filters to apply to the debug messages", eCommandFlagsNone, BIND_COMMAND(this, &DebugCommandProvider::CommandLogFilter), { "include/exclude The type of filter", "add/remove Add or remove the filter", "string The filter to add" }),
		};

		return commands;
	}

	void DebugCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarLogName = manager->Add(Command::CreateVariableString("Debug", "LogName", "debug_logname", "Filename to store debug log messages", eCommandFlagsArchived, "dorito.log"));
		VarMemcpySrc = manager->Add(Command::CreateVariableInt("Debug", "MemcpySrc", "memcpy_src", "Allows breakpointing memcpy based on specified source address filter.", static_cast<CommandFlags>(eCommandFlagsDontUpdateInitial | eCommandFlagsHidden), 0, BIND_COMMAND(this, &DebugCommandProvider::VariableMemcpySrcUpdate)));
		VarMemcpyDst = manager->Add(Command::CreateVariableInt("Debug", "MemcpyDst", "memcpy_dst", "Allows breakpointing memcpy based on specified destination address filter.", static_cast<CommandFlags>(eCommandFlagsDontUpdateInitial | eCommandFlagsHidden), 0, BIND_COMMAND(this, &DebugCommandProvider::VariableMemcpyDstUpdate)));
		VarMemsetDst = manager->Add(Command::CreateVariableInt("Debug", "MemsetDst", "memset_dst", "Allows breakpointing memset based on specified destination address filter.", static_cast<CommandFlags>(eCommandFlagsDontUpdateInitial | eCommandFlagsHidden), 0, BIND_COMMAND(this, &DebugCommandProvider::VariableMemsetDstUpdate)));
	}

	bool DebugCommandProvider::CommandLogMode(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		auto& dorito = ElDorito::Instance();
		auto newFlags = DebugFlags;

		if (Arguments.size() > 0)
		{
			for (auto arg : Arguments)
			{
				if (arg.compare("off") == 0)
				{
					// Disable it.
					newFlags = 0;

					dorito.PatchManager.EnableHook(debugPatches->NetworkLogHook, false);
					dorito.PatchManager.EnableHook(debugPatches->SSLLogHook, false);
					dorito.PatchManager.EnableHook(debugPatches->UILogHook, false);
					dorito.PatchManager.EnableHook(debugPatches->Game1LogHook, false);
					dorito.PatchManager.EnablePatchSet(debugPatches->Game2LogHook, false);
					dorito.PatchManager.EnablePatchSet(debugPatches->PacketsHook, false);
				}
				else
				{
					auto hookNetwork = arg.compare("network") == 0;
					auto hookSSL = arg.compare("ssl") == 0;
					auto hookUI = arg.compare("ui") == 0;
					auto hookGame1 = arg.compare("game1") == 0;
					auto hookGame2 = arg.compare("game2") == 0;
					auto hookPackets = arg.compare("packets") == 0;

					if (arg.compare("all") == 0 || arg.compare("on") == 0)
						hookNetwork = hookSSL = hookUI = hookGame1 = hookGame2 = hookPackets = true;

					if (hookNetwork)
					{
						newFlags |= (int)DebugLoggingModes::Network;
						dorito.PatchManager.EnableHook(debugPatches->NetworkLogHook, true);
					}

					if (hookSSL)
					{
						newFlags |= (int)DebugLoggingModes::SSL;
						dorito.PatchManager.EnableHook(debugPatches->SSLLogHook, true);
					}

					if (hookUI)
					{
						newFlags |= (int)DebugLoggingModes::UI;
						dorito.PatchManager.EnableHook(debugPatches->UILogHook, true);
					}

					if (hookGame1)
					{
						newFlags |= (int)DebugLoggingModes::Game1;
						dorito.PatchManager.EnableHook(debugPatches->Game1LogHook, true);
					}

					if (hookGame2)
					{
						newFlags |= (int)DebugLoggingModes::Game2;
						dorito.PatchManager.EnablePatchSet(debugPatches->Game2LogHook, true);
					}

					if (hookPackets)
					{
						newFlags |= (int)DebugLoggingModes::Packets;
						dorito.PatchManager.EnablePatchSet(debugPatches->PacketsHook, true);
					}
				}
			}
		}

		DebugFlags = newFlags;

		std::stringstream ss;
		ss << "Debug logging: ";
		if (newFlags == 0)
			ss << "disabled";
		else
		{
			ss << "enabled: ";
			if (newFlags & (int)DebugLoggingModes::Network)
				ss << "Network ";
			if (newFlags & (int)DebugLoggingModes::SSL)
				ss << "SSL ";
			if (newFlags & (int)DebugLoggingModes::UI)
				ss << "UI ";
			if (newFlags & (int)DebugLoggingModes::Game1)
				ss << "Game1 ";
			if (newFlags & (int)DebugLoggingModes::Game2)
				ss << "Game2 ";
			if (newFlags & (int)DebugLoggingModes::Packets)
				ss << "Packets ";
		}
		if (Arguments.size() <= 0)
		{
			ss << std::endl << "Usage: Game.DebugMode <network | ssl | ui | game1 | game2 | packets | all | off>";
		}
		context.WriteOutput(ss.str());
		return true;
	}

	bool DebugCommandProvider::CommandLogFilter(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		std::stringstream ss;
		if (Arguments.size() != 3)
		{
			ss << "Usage: Debug.LogFilter <include/exclude> <add/remove> <string>" << std::endl << std::endl;
		}
		else
		{
			bool exclude = (Arguments[0].compare("exclude") == 0);
			bool remove = Arguments[1].compare("remove") == 0;
			auto vect = exclude ? &FiltersExclude : &FiltersInclude;
			auto str = Arguments[2];
			bool exists = std::find(vect->begin(), vect->end(), str) != vect->end();
			if (remove && !exists)
			{
				ss << "Failed to find string \"" << str << "\" inside " << (exclude ? "exclude" : "include") << " filters list" << std::endl << std::endl;
			}
			else if (remove)
			{
				auto pos = std::find(vect->begin(), vect->end(), str);
				vect->erase(pos);
				ss << "Removed \"" << str << "\" from " << (exclude ? "exclude" : "include") << " filters list" << std::endl << std::endl;
			}
			else
			{
				vect->push_back(str);
				ss << "Added \"" << str << "\" to " << (exclude ? "exclude" : "include") << " filters list" << std::endl << std::endl;
			}
		}

		ss << "Include filters (message must contain these strings):";

		for (auto filter : FiltersInclude)
			ss << std::endl << filter;

		ss << std::endl << std::endl << "Exclude filters (message must not contain these strings):";

		for (auto filter : FiltersExclude)
			ss << std::endl << filter;

		context.WriteOutput(ss.str());
		return true;
	}

	bool DebugCommandProvider::VariableMemcpySrcUpdate(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		auto src = VarMemcpySrc->ValueInt;

		context.WriteOutput("Memcpy source address filter set to " + std::to_string(src));
		return true;
	}

	bool DebugCommandProvider::VariableMemcpyDstUpdate(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		auto dst = VarMemcpyDst->ValueInt;

		context.WriteOutput("Memcpy destination address filter set to " + std::to_string(dst));
		return true;
	}

	bool DebugCommandProvider::VariableMemsetDstUpdate(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		auto dst = VarMemsetDst->ValueInt;

		context.WriteOutput("Memset destination address filter set to " + std::to_string(dst));
		return true;
	}
}