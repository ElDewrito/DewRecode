#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace ContentItems
{
	class ContentItemsPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}