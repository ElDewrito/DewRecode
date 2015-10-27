#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Camera
{
	class CameraPatchProvider : public PatchProvider
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