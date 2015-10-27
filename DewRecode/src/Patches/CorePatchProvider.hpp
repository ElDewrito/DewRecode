#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Core
{
	class CorePatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}