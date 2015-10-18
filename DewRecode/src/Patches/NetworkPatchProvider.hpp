#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Network
{
	class NetworkPatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}