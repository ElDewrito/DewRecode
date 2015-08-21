#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace ContentItems
{
	class ContentItemsPatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}