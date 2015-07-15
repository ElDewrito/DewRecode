#pragma once
#include <ElDorito/ModuleBase.hpp>

enum DebugLoggingModes
{
	eDebugLoggingModeNetwork = 1,
	eDebugLoggingModeSSL = 2,
	eDebugLoggingModeUI = 4,
	eDebugLoggingModeGame1 = 8,
	eDebugLoggingModeGame2 = 16,
};

namespace Modules
{
	class ModuleGame : public ModuleBase
	{
	public:
		Command* VarLanguageID;
		Command* VarSkipLauncher;
		Command* VarLogName;

		Hook* NetworkLogHook;
		Hook* SSLLogHook;
		Hook* UILogHook;
		Hook* Game1LogHook;
		PatchSet* Game2LogHook;

		int DebugFlags;
		std::vector<std::string> MapList;
		std::vector<std::string> FiltersExclude;
		std::vector<std::string> FiltersInclude;

		ModuleGame();
	};
}
