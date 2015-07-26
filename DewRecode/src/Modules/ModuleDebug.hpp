#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleDebug : public ModuleBase
	{
	public:
		Command* VarMemcpySrc;
		Command* VarMemcpyDst;

		ModuleDebug();
	};
}