#pragma once
#include <ElDorito/ModuleBase.hpp>

#include "../Blam/BlamTypes.hpp"
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

		// patches to stop camera mode from changing
		Patch* Debug1CameraPatch;
		Patch* Debug2CameraPatch;
		Patch* ThirdPersonPatch;
		Patch* FirstPersonPatch;
		Patch* DeadPersonPatch;

		Patch* StaticILookVectorPatch;
		Patch* StaticKLookVectorPatch;

		Patch* HideHudPatch;
		Patch* CenteredCrosshairPatch;

		Hook* CameraPermissionHook;
		Hook* CameraPermissionHookAlt1;
		Hook* CameraPermissionHookAlt2;
		Hook* CameraPermissionHookAlt3;

		ModuleCamera();

		void UpdatePosition();

		//std::unordered_map<std::string, CameraType> CameraTypeStrings;
		//std::unordered_map<CameraType, size_t> CameraTypeFunctions;
	};
}