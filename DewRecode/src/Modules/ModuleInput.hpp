#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleInput : public ModuleBase
	{
	public:
		Command* VarInputRawInput;

		ModuleInput();
	};
}