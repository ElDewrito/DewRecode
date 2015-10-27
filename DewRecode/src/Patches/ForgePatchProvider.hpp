#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Forge
{
	class ForgePatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;

		void SignalDelete();
	};
}