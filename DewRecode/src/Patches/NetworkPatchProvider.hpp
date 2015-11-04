#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Network
{
	class NetworkPatchProvider : public PatchProvider
	{
		PatchSet* PatchDedicatedServer;
		PatchSet* PatchDisableD3D;

	public:
		NetworkPatchProvider();
		virtual PatchSet GetPatches() override;

		bool SetDedicatedServerMode(bool isDedicated);
		bool GetDedicatedServerMode();

		bool SetD3DDisabled(bool isDisabled);
		bool GetD3DDisabled();
	};
}