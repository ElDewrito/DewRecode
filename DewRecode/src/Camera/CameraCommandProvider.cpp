#include "CameraCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace Camera
{
	CameraCommandProvider::CameraCommandProvider(std::shared_ptr<CameraPatchProvider> cameraPatches)
	{
		this->cameraPatches = cameraPatches;
	}

	void CameraCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarCrosshair = manager->Add(Command::CreateVariableInt("Camera", "Crosshair", "crosshair", "Controls whether the crosshair should be centered", eCommandFlagsArchived, 0, BIND_COMMAND(this, &CameraCommandProvider::VariableCrosshairUpdate)));
		VarCrosshair->ValueIntMin = 0;
		VarCrosshair->ValueIntMax = 1;

		VarFov = manager->Add(Command::CreateVariableFloat("Camera", "FOV", "fov", "The cameras field of view", eCommandFlagsArchived, 90.f, BIND_COMMAND(this, &CameraCommandProvider::VariableFovUpdate)));
		VarFov->ValueFloatMin = 55.f;
		VarFov->ValueFloatMax = 155.f;

		VarHideHud = manager->Add(Command::CreateVariableInt("Camera", "HideHUD", "hud", "Toggles the HUD", eCommandFlagsArchived, 0, BIND_COMMAND(this, &CameraCommandProvider::VariableHideHudUpdate)));
		VarHideHud->ValueIntMin = 0;
		VarHideHud->ValueIntMax = 1;

		VarSpeed = manager->Add(Command::CreateVariableFloat("Camera", "Speed", "camera_speed", "The camera speed", eCommandFlagsArchived, 0.1f, BIND_COMMAND(this, &CameraCommandProvider::VariableSpeedUpdate)));
		VarSpeed->ValueFloatMin = 0.01f;
		VarSpeed->ValueFloatMax = 5.0f;

		VarSpectatorIndex = manager->Add(Command::CreateVariableInt("Camera", "SpectatorIndex", "spectator_index", "The player index to spectate", eCommandFlagsDontUpdateInitial, 0, BIND_COMMAND(this, &CameraCommandProvider::VariableSpectatorIndexUpdate)));
		VarSpectatorIndex->ValueIntMin = 0;
		VarSpectatorIndex->ValueIntMax = 15;

		VarMode = manager->Add(Command::CreateVariableString("Camera", "Mode", "camera_mode", "Camera mode, valid modes: default, first, third, flying, static, spectator", (CommandFlags)(eCommandFlagsDontUpdateInitial | eCommandFlagsCheat), "default", BIND_COMMAND(this, &CameraCommandProvider::VariableModeUpdate)));

	}

	bool CameraCommandProvider::VariableCrosshairUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = VarCrosshair->ValueInt;

		std::string status = "disabled.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "enabled.";

		ElDorito::Instance().PatchManager.EnablePatch(cameraPatches->CenteredCrosshairPatch, statusBool);

		returnInfo = "Centered crosshair " + status;
		return true;
	}

	bool CameraCommandProvider::VariableFovUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		float value = VarFov->ValueFloat;

		Pointer(0x2301D98).Write(value);
		Pointer(0x189D42C).Write(value);

		return true;
	}

	bool CameraCommandProvider::VariableHideHudUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = VarHideHud->ValueInt;

		std::string status = "shown.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "hidden.";

		ElDorito::Instance().PatchManager.EnablePatch(cameraPatches->HideHudPatch, statusBool);

		returnInfo = "HUD " + status;
		return true;
	}

	bool CameraCommandProvider::VariableSpeedUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		float speed = VarSpeed->ValueFloat;

		std::stringstream ss;
		ss << "Camera speed set to " << speed;
		returnInfo = ss.str();

		return true;
	}

	bool CameraCommandProvider::VariableSpectatorIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long index = VarSpectatorIndex->ValueInt;

		// get current player count and clamp index within bounds
		auto* players = ElDorito::Instance().Engine.GetArrayGlobal(GameGlobals::Players::TLSOffset);
		int clampedIndex = index % players->GetCount();
		VarSpectatorIndex->ValueInt = clampedIndex;

		std::stringstream ss;
		ss << "Spectating player index " << clampedIndex;
		returnInfo = ss.str();

		return true;
	}

	bool CameraCommandProvider::VariableModeUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		auto mode = dorito.Utils.ToLower(VarMode->ValueString);

		// get some globals
		Pointer &playerControlGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		Pointer &directorGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Director::TLSOffset)[0];
		Pointer &observerGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Observer::TLSOffset)[0];

		// patches allowing us to control the camera when a non-default mode is selected
		dorito.PatchManager.EnablePatchSet(cameraPatches->CustomModePatches, mode.compare("default") != 0);

		// prevents the engine from modifying any camera components while in static/spectator mode
		dorito.PatchManager.EnablePatchSet(cameraPatches->StaticModePatches, mode.compare("static") == 0 || mode.compare("spectator") == 0);

		// makes sure the hud is hidden when flying/spectator/static camera mode
		if (!VarHideHud->ValueInt)
			dorito.PatchManager.EnablePatch(cameraPatches->HideHudPatch, mode.compare("flying") == 0 || mode.compare("static") == 0 || mode.compare("spectator") == 0);

		// disable player movement while in flycam
		playerControlGlobalsPtr(GameGlobals::Input::DisablePlayerInputIndex).Write(mode.compare("flying") == 0);

		// get new camera perspective function offset 
		size_t offset = 0x166ACB0;
		if (!mode.compare("first")) // c_first_person_camera
		{
			offset = 0x166ACB0;
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftX).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftY).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftHorizontal).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftVertical).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Write(0.0f);
		}
		else if (!mode.compare("third")) // c_following_camera
		{
			offset = 0x16724D4;
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftX).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftY).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Write(0.1f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftHorizontal).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftVertical).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Write(0.5f);
			observerGlobalsPtr(GameGlobals::Observer::CameraFieldOfView).Write(1.91986218f);	// 110 degrees
		}
		else if (!mode.compare("flying")) // c_flying_camera
		{
			offset = 0x16726D0;
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftX).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftY).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftHorizontal).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftVertical).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Write(0.0f);
		}
		else if (!mode.compare("static") || !mode.compare("spectator")) // c_static_camera
		{
			offset = 0x16728A8;
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftX).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftY).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftHorizontal).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftVertical).Write(0.0f);
			observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Write(0.0f);
		}

		/*
		else if (!mode.compare("dead")) // c_dead_camera
		offset = 0x16725DC;
		else if (!mode.compare("scripted")) // c_scripted_camera
		offset = 0x167280C;
		else if (!mode.compare("pancam1")) // c_director
		offset = 0x165A64C;
		else if (!mode.compare("pancam2"))
		offset = 0x165A680;
		else if (!mode.compare("pancam3"))
		offset = 0x165A674;
		else if (!mode.compare("unk1")) // c_orbiting_camera
		offset = 0x167265C;
		else if (!mode.compare("debug")) // c_camera
		offset = 0x1672130;
		else if (!mode.compare("debug2")) // c_null_camera - crashes
		offset = 0x165A6E4;
		else if (!mode.compare("unk4")) // c_authored_camera
		offset = 0x1672920;
		*/

		// update camera perspective function
		size_t oldOffset = directorGlobalsPtr(GameGlobals::Director::CameraFunctionIndex).Read<size_t>();
		directorGlobalsPtr(GameGlobals::Director::CameraFunctionIndex).Write(offset);

		// output old -> new perspective function information to console
		std::stringstream ss;
		ss << "0x" << std::hex << oldOffset << " -> 0x" << offset;
		returnInfo = ss.str();

		return true;
	}

	//bool VariableCameraSave(const std::vector<std::string>& Arguments, std::string& returnInfo)
	//{
	//	auto mode = Utils::String::ToLower(Modules::ModuleCamera::Instance().VarCameraMode->ValueString);
	//	Pointer &directorGlobalsPtr = ElDorito::GetMainTls(GameGlobals::Director::TLSOffset)[0];

	//	// only allow saving while in flycam or static modes
	//	if (mode != "flying" && mode != "static")
	//		return true;

	//	// TODO: finish

	//	return true;
	//}
	//
	//bool VariableCameraLoad(const std::vector<std::string>& Arguments, std::string& returnInfo)
	//{
	//	auto mode = Utils::String::ToLower(Modules::ModuleCamera::Instance().VarCameraMode->ValueString);
	//	Pointer &directorGlobalsPtr = ElDorito::GetMainTls(GameGlobals::Director::TLSOffset)[0];

	//	// only allow loading while in flycam or static modes
	//	if (mode != "flying" && mode != "static")
	//		return true;

	//	// TODO: finish

	//	return true;
	//}
}