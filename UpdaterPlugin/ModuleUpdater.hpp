#pragma once
#include <ElDorito/ElDorito.hpp>

namespace Modules
{
	class ModuleUpdater : public ModuleBase
	{
	public:
		Command* VarUpdaterBranch;

		ModuleUpdater();

		std::string Check(std::string branch);
	};
}
