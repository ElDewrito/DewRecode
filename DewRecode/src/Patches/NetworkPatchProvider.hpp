#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Network
{
	class NetworkPatchProvider : public PatchProvider
	{
		PatchSet* PatchDedicatedServer;

	public:
		NetworkPatchProvider();
		virtual PatchSet GetPatches() override;

		bool SetDedicatedServerMode(bool isDedicated);
		bool GetDedicatedServerMode();
	};
}