#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Armor
{
	class ArmorPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		void SignalRefreshUIPlayerArmor(void* param = 0);
	};
}