#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Armor
{
	class ArmorPatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		void SignalRefreshUIPlayerArmor(void* param = 0);
	};
}