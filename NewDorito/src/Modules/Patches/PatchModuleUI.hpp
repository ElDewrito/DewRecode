#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class PatchModuleUI : public ModuleBase
	{
	public:
		bool DialogShow; // todo: put this somewhere better
		unsigned int DialogStringId;
		int DialogArg1; // todo: figure out a better name for this
		int DialogFlags;
		unsigned int DialogParentStringId;
		void* UIData = 0;

		PatchModuleUI();

		bool Tick(const std::chrono::duration<double>& deltaTime);
	};
}
