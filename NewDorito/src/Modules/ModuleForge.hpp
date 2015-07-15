#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleForge : public ModuleBase
	{
	public:
		ModuleForge();

		void SignalDeleteItem();
	};
}
