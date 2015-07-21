#pragma once
#include <ElDorito/ModuleBase.hpp>
#include <map>
#include <ElDorito/Blam/BlamTypes.hpp>

namespace Modules
{
	class ModuleInput : public ModuleBase
	{
	public:
		Command* VarInputRawInput;
		Command* VarInputControllerIndex;

		ModuleInput();
	};
}