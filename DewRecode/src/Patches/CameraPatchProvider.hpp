#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Camera
{
	class CameraPatchProvider : public IPatchProvider
	{
	public:
		PatchSet* CustomModePatches;
		PatchSet* StaticModePatches;

		Patch* HideHudPatch;
		Patch* CenteredCrosshairPatch;

		CameraPatchProvider();

		virtual void RegisterCallbacks(IEngine* engine) override;

		void UpdatePosition(const std::chrono::duration<double>& deltaTime);
	};
}