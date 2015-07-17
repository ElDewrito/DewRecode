#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class PatchModulePlayerUid : public ModuleBase
	{
	public:
		PatchModulePlayerUid();

		uint64_t GetUid();
		std::string GetFormattedPrivKey();
		void EnsureValidUid();
	};
}
