#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Forge
{
	class ForgePatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;

		void SignalDelete();
	};
}