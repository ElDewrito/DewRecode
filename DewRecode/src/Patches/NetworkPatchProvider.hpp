#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Network
{
	class NetworkPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}