#pragma once
#include <ElDorito/ModuleBase.hpp>

#include <ElDorito/Blam/BlamTypes.hpp>
#include <unordered_map>

namespace Modules
{
	class ModuleCamera : public ModuleBase
	{
	public:
		Command* VarCameraCrosshair;
		Command* VarCameraFov;
		Command* VarCameraHideHud;
		Command* VarCameraMode;
		Command* VarCameraSpeed;
		Command* VarCameraSave;
		Command* VarCameraLoad;
		Command* VarSpectatorIndex;

		PatchSet* CustomModePatches;
		PatchSet* StaticModePatches;

		Patch* HideHudPatch;
		Patch* CenteredCrosshairPatch;

		ModuleCamera();

		void UpdatePosition();
	};
}