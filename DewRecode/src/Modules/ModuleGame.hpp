#pragma once
#include <ElDorito/ModuleBase.hpp>

enum DebugLoggingModes
{
	eDebugLoggingModeNetwork = 1 << 0,
	eDebugLoggingModeSSL = 1 << 1,
	eDebugLoggingModeUI = 1 << 2,
	eDebugLoggingModeGame1 = 1 << 3,
	eDebugLoggingModeGame2 = 1 << 4,
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
