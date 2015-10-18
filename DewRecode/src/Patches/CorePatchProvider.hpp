#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Core
{
	class CorePatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}