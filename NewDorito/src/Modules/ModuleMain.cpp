#include "ModuleMain.hpp"

bool CommandHelp(const std::vector<std::string>& Arguments, std::string& returnInfo)
{
	returnInfo = "Help command";
	return true;
}

namespace Modules
{
	ModuleMain::ModuleMain() : ModuleBase("")
	{
		AddCommand("Help", "", "Displays some help text", eCommandFlagsNone, CommandHelp);
	}
}
