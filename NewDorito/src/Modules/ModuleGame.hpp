#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleGame : public ModuleBase
	{
	public:
		Command* VarLanguageID;
		Command* VarSkipLauncher;

		ModuleGame();
	};
}
