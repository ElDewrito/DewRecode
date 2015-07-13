#pragma once
#include "ModuleGame.hpp"

namespace Modules
{
	class ModuleMain : public ModuleBase
	{
	public:
		ModuleGame GameModule;

		ModuleMain();
	};
}