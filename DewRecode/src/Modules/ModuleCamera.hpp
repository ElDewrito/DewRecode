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

		// patches to stop camera mode from changing
		PatchSet* CameraPatches;
		PatchSet* StaticPatches;

		Patch* HideHudPatch;
		Patch* CenteredCrosshairPatch;

		ModuleCamera();

		void UpdatePosition();

		//std::unordered_map<std::string, CameraType> CameraTypeStrings;
		//std::unordered_map<CameraType, size_t> CameraTypeFunctions;
	};
}