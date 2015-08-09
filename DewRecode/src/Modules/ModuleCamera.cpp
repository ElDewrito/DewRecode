#include "ModuleCamera.hpp"
#include <sstream>
#include "../ElDorito.hpp"

namespace
{
	enum CameraDefinitionType : int
	{
		Position = 0,
		PositionShift = 1,
		LookShift = 2,
		Depth = 3,
		FieldOfView = 4,
		LookVectors = 5
	};

	// determine which camera definitions are editable based on the current camera mode
	bool __stdcall IsCameraDefinitionEditable(CameraDefinitionType definition)
	{
		auto& dorito = ElDorito::Instance();
		auto mode = dorito.Utils.ToLower(dorito.Modules.Camera.VarCameraMode->ValueString);
 
		if (!mode.compare("first") || !mode.compare("third"))
		{
			if (definition == CameraDefinitionType::PositionShift ||
				definition == CameraDefinitionType::LookShift ||
				definition == CameraDefinitionType::Depth)
			{
				return true;
			}
		}
		else if (!mode.compare("flying") || !mode.compare("static") || !mode.compare("spectator"))
		{
			return true;
		}

		// otherwise let the game do the camera updating
		return false;
	}

	// hook @ 0x61440D - allows for the modification of specific camera components based on current perspective
	__declspec(naked) void UpdateCameraDefinitions()
	{
		__asm
		{
			pushad
			shr		esi, 1						; convert camera definition offset to an index
			push	esi							; CameraDefinitionType
			call	IsCameraDefinitionEditable
			test	al, al
			popad
			jnz		skip
			mov		eax, [eax + ecx * 4]		; get data(definition + item * 4)
			mov		[edi + ecx * 4], eax		; store it in the 3rd camera array
			skip:
			push	0614413h
			ret
		}
	}

	// hook @ 0x614818 - allows for the modification of specific camera components based on current perspective
	__declspec(naked) void UpdateCameraDefinitionsAlt1()
	{
		__asm
		{
			pushad
			shr		esi, 1
			push	esi
			call	IsCameraDefinitionEditable
			test	al, al
			popad
			jnz		skip
			movss   dword ptr [edx + eax * 4], xmm0
			skip:
			push	061481Dh
			ret
		}
	}

	// hook @ 0x6148BE - allows for the modification of specific camera components based on current perspective
	__declspec(naked) void UpdateCameraDefinitionsAlt2()
	{
		__asm
		{
			pushad
			shr		esi, 1
			push	esi
			call	IsCameraDefinitionEditable
			test	al, al
			popad
			jnz		skip
			movss   dword ptr [edx + eax * 4], xmm1
			skip:
			push	06148C3h
			ret
		}
	}

	// hook @ 0x614902 - allows for the modification of specific camera components based on current perspective
	__declspec(naked) void UpdateCameraDefinitionsAlt3()
	{
		__asm
		{
			pushad
			shr		esi, 1
			push	esi
			call	IsCameraDefinitionEditable
			test	al, al
			popad
			jnz		skip
			movss   dword ptr [edi + eax * 4], xmm0
			skip:
			push	0614907h
			ret
		}
	}

	bool VariableCameraCrosshairUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		unsigned long value = dorito.Modules.Camera.VarCameraCrosshair->ValueInt;

		std::string status = "disabled.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "enabled.";

		dorito.Patches.EnablePatch(dorito.Modules.Camera.CenteredCrosshairPatch, statusBool);

		returnInfo = "Centered crosshair " + status;
		return true;
	}

	bool VariableCameraFovUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		float value = ElDorito::Instance().Modules.Camera.VarCameraFov->ValueFloat;

		Pointer(0x2301D98).Write(value);
		Pointer(0x189D42C).Write(value);

		return true;
	}

	bool VariableCameraHideHudUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		unsigned long value = dorito.Modules.Camera.VarCameraHideHud->ValueInt;

		std::string status = "shown.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "hidden.";

		dorito.Patches.EnablePatch(dorito.Modules.Camera.HideHudPatch, statusBool);

		returnInfo = "HUD " + status;
		return true;
	}

	bool VariableCameraSpeedUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		float speed = ElDorito::Instance().Modules.Camera.VarCameraSpeed->ValueFloat;

		std::stringstream ss;
		ss << "Camera speed set to " << speed;
		returnInfo = ss.str();

		return true;
	}

	bool SpectatorIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		unsigned long index = dorito.Modules.Camera.VarSpectatorIndex->ValueInt;

		// get current player count and clamp index within bounds
		Pointer &players = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];
		int playerCount = players(0x38).Read<int>();
		int clampedIndex = index % playerCount;
		dorito.Modules.Camera.VarSpectatorIndex->ValueInt = clampedIndex;

		std::stringstream ss;
		ss << "Spectating player index " << clampedIndex;
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
	
	bool VariableCameraModeUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		auto mode = dorito.Utils.ToLower(ElDorito::Instance().Modules.Camera.VarCameraMode->ValueString);
		auto& camera = dorito.Modules.Camera;

		// get some globals
		Pointer &playerControlGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		Pointer &directorGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Director::TLSOffset)[0];
		Pointer &observerGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Observer::TLSOffset)[0];

		// patches allowing us to control the camera when a non-default mode is selected
		dorito.Patches.EnablePatchSet(camera.CustomModePatches, mode.compare("default") != 0);

		// prevents the engine from modifying any camera components while in static/spectator mode
		dorito.Patches.EnablePatchSet(camera.StaticModePatches, mode.compare("static") == 0 || mode.compare("spectator") == 0);

		// makes sure the hud is hidden when flying/spectator/static camera mode
		if (!camera.VarCameraHideHud->ValueInt)
			dorito.Patches.EnablePatch(camera.HideHudPatch, mode.compare("flying") == 0 || mode.compare("static") == 0 || mode.compare("spectator") == 0);

		// disable player movement while in flycam
		playerControlGlobalsPtr(GameGlobals::Input::DisablePlayerInputIndex).Write(mode.compare("flying") == 0);

		// get new camera perspective function offset 
		size_t offset = 0x166ACB0;
		if (!mode.compare("first")) // c_first_person_camera
		{
			offset = 0x166ACB0;
			observerGlobalsPtr(0x18C).Write(0.0f);			// x camera shift
			observerGlobalsPtr(0x190).Write(0.0f);			// y camera shift
			observerGlobalsPtr(0x194).Write(0.0f);			// z camera shift
			observerGlobalsPtr(0x198).Write(0.0f);			// horizontal look shift
			observerGlobalsPtr(0x19C).Write(0.0f);			// vertical look shift
			observerGlobalsPtr(0x1A0).Write(0.0f);			// depth
		}
		else if (!mode.compare("third")) // c_following_camera
		{
			offset = 0x16724D4;
			observerGlobalsPtr(0x18C).Write(0.0f);			// x camera shift
			observerGlobalsPtr(0x190).Write(0.0f);			// y camera shift
			observerGlobalsPtr(0x194).Write(0.1f);			// z camera shift
			observerGlobalsPtr(0x198).Write(0.0f);			// horizontal look shift
			observerGlobalsPtr(0x19C).Write(0.0f);			// vertical look shift
			observerGlobalsPtr(0x1A0).Write(0.5f);			// depth
			observerGlobalsPtr(0x1A4).Write(1.91986218f);	// 110 degrees
		}
		else if (!mode.compare("flying")) // c_flying_camera
		{
			offset = 0x16726D0;
			observerGlobalsPtr(0x18C).Write(0.0f);			// x camera shift
			observerGlobalsPtr(0x190).Write(0.0f);			// y camera shift
			observerGlobalsPtr(0x194).Write(0.0f);			// z camera shift
			observerGlobalsPtr(0x198).Write(0.0f);			// horizontal look shift
			observerGlobalsPtr(0x19C).Write(0.0f);			// vertical look shift
			observerGlobalsPtr(0x1A0).Write(0.0f);			// depth
		}
		else if (!mode.compare("static") || !mode.compare("spectator")) // c_static_camera
		{
			offset = 0x16728A8;
			observerGlobalsPtr(0x18C).Write(0.0f);			// x camera shift
			observerGlobalsPtr(0x190).Write(0.0f);			// y camera shift
			observerGlobalsPtr(0x194).Write(0.0f);			// z camera shift
			observerGlobalsPtr(0x198).Write(0.0f);			// horizontal look shift
			observerGlobalsPtr(0x19C).Write(0.0f);			// vertical look shift
			observerGlobalsPtr(0x1A0).Write(0.0f);			// depth
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

	// TODO: make this use a lambda func instead once VC supports converting lambdas to funcptrs
	void CameraPatches_TickCallback(const std::chrono::duration<double>& deltaTime)
	{
		ElDorito::Instance().Modules.Camera.UpdatePosition();
	}
}

namespace Modules
{
	ModuleCamera::ModuleCamera() : ModuleBase("Camera")
	{
		// register our tick callbacks
		engine->OnTick(CameraPatches_TickCallback);

		// TODO: commands for setting camera speed, positions, save/restore etc.

		VarCameraCrosshair = AddVariableInt("Crosshair", "crosshair", "Controls whether the crosshair should be centered", eCommandFlagsArchived, 0, VariableCameraCrosshairUpdate);
		VarCameraCrosshair->ValueIntMin = 0;
		VarCameraCrosshair->ValueIntMax = 1;

		VarCameraFov = AddVariableFloat("FOV", "fov", "The cameras field of view", eCommandFlagsArchived, 90.f, VariableCameraFovUpdate);
		VarCameraFov->ValueFloatMin = 55.f;
		VarCameraFov->ValueFloatMax = 155.f;

		VarCameraHideHud = AddVariableInt("HideHUD", "hud", "Toggles the HUD", eCommandFlagsArchived, 0, VariableCameraHideHudUpdate);
		VarCameraHideHud->ValueIntMin = 0;
		VarCameraHideHud->ValueIntMax = 1;

		VarCameraSpeed = AddVariableFloat("Speed", "camera_speed", "The camera speed", eCommandFlagsArchived, 0.1f, VariableCameraSpeedUpdate);
		VarCameraSpeed->ValueFloatMin = 0.01f;
		VarCameraSpeed->ValueFloatMax = 5.0f;

		VarSpectatorIndex = AddVariableInt("SpectatorIndex", "spectator_index", "The player index to spectate", eCommandFlagsDontUpdateInitial, 0, SpectatorIndexUpdate);
		VarSpectatorIndex->ValueIntMin = 0;
		VarSpectatorIndex->ValueIntMax = 15;

		VarCameraMode = AddVariableString("Mode", "camera_mode", "Camera mode, valid modes: default, first, third, flying, static, spectator", (CommandFlags)(eCommandFlagsDontUpdateInitial | eCommandFlagsCheat), "default", VariableCameraModeUpdate);

		CustomModePatches = patches->AddPatchSet("CustomModePatches",
		{
			// prevents the engine from switching camera perspectives when using a custom camera mode
			Patch("CameraDebug1", 0x725A80, 0x90, 6),
			Patch("CameraDebug2", 0x591525, 0x90, 6),
			Patch("CameraThirdPerson", 0x728640, 0x90, 6),
			Patch("CameraFirstPerson", 0x65F420, 0x90, 6),
			Patch("CameraDead", 0x729E6F, 0x90, 6),
		},
		{
			// allows us to control certain parts of the camera depending on the current custom camera mode
			Hook("CameraPermHook", 0x61440D, UpdateCameraDefinitions, HookType::Jmp),
			Hook("CameraPermHookAlt1", 0x614818, UpdateCameraDefinitionsAlt1, HookType::Jmp),
			Hook("CameraPermHookAlt2", 0x6148BE, UpdateCameraDefinitionsAlt2, HookType::Jmp),
			Hook("CameraPermHookAlt3", 0x614902, UpdateCameraDefinitionsAlt3, HookType::Jmp),
		});

		// prevents the engine from modifying camera components while in static/spectator mode
		StaticModePatches = patches->AddPatchSet("DisableStaticUpdatePatches",
		{
			Patch("IForwardLook", 0x611433, 0x90, 8),
			Patch("KForwardLook", 0x61143E, 0x90, 6),

			Patch("IForwardLook2", 0x6146EB, 0x90, 9),
			Patch("JForwardLook2", 0x6146FE, 0x90, 9),
			Patch("JForwardLook2", 0x614711, 0x90, 9),

			Patch("IJForwardLook3", 0x611EC8, 0x90, 4),
			Patch("KForwardLook3", 0x611ECF, 0x90, 3),
			
			Patch("IUpLook", 0x614648, 0x90, 9),
			Patch("JUpLook", 0x61463B, 0x90, 9),
			Patch("KUpLook", 0x614632, 0x90, 9),

			Patch("IUpLook2", 0x6147B0, 0x90, 9),
			Patch("JUpLook2", 0x6147C3, 0x90, 9),
			Patch("KUpLook2", 0x6147D6, 0x90, 9),

			Patch("IJUpLook3", 0x611EDB, 0x90, 4),
			Patch("KUpLook3", 0x611EE2, 0x90, 3),

			//Patch("HLookAngle", 0x5D3808, 0x90, 4),
			//Patch("HLookAngle2", 0x5D3A0B, 0x90, 4),
			//Patch("HLookAngle3", 0x5D3A2B, 0x90, 4),

			//Patch("VLookAngle", 0x5D3F44, 0x90, 9),
			//Patch("VLookAngle2", 0x5D3F75, 0x90, 9),

			// fov - 0x611EE5, 0x6122E8
		});

		HideHudPatch = patches->AddPatch("HideHud", 0x16B5A5C, { 0xC3, 0xF5, 0x48, 0x40 }); // 3.14f in hex form
		CenteredCrosshairPatch = patches->AddPatch("CenteredCrosshair", 0x65FA43, { 0x31, 0xC0, 0x90, 0x90 });
	}

	void ModuleCamera::UpdatePosition()
	{
		auto& dorito = ElDorito::Instance();
		auto mode = dorito.Utils.ToLower(dorito.Modules.Camera.VarCameraMode->ValueString);

		Pointer &observerGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Observer::TLSOffset)[0];
		Pointer &playerControlGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		Pointer &playersPtr = dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset)[0];
		Pointer &objectHeaderPtr = dorito.Engine.GetMainTls(GameGlobals::ObjectHeader::TLSOffset)[0];

		// only allow flycam input outside of cli/chat
		if (!mode.compare("flying") && !dorito.Modules.Console.IsVisible())
		{
			float moveDelta = dorito.Modules.Camera.VarCameraSpeed->ValueFloat;
			float lookDelta = 0.01f;	// not used yet

			// current values
			float hLookAngle = playerControlGlobalsPtr(0x30C).Read<float>();
			float vLookAngle = playerControlGlobalsPtr(0x310).Read<float>();
			float xPos = observerGlobalsPtr(0x180).Read<float>();
			float yPos = observerGlobalsPtr(0x184).Read<float>();
			float zPos = observerGlobalsPtr(0x188).Read<float>();
			float xShift = observerGlobalsPtr(0x18C).Read<float>();
			float yShift = observerGlobalsPtr(0x190).Read<float>();
			float zShift = observerGlobalsPtr(0x194).Read<float>();
			float hShift = observerGlobalsPtr(0x198).Read<float>();
			float vShift = observerGlobalsPtr(0x19C).Read<float>();
			float depth = observerGlobalsPtr(0x1A0).Read<float>();
			float fov = observerGlobalsPtr(0x1A4).Read<float>();
			float iForward = observerGlobalsPtr(0x1A8).Read<float>();
			float jForward = observerGlobalsPtr(0x1AC).Read<float>();
			float kForward = observerGlobalsPtr(0x1B0).Read<float>();
			float iUp = observerGlobalsPtr(0x1B4).Read<float>();
			float jUp = observerGlobalsPtr(0x1B8).Read<float>();
			float kUp = observerGlobalsPtr(0x1BC).Read<float>();
			float iRight = cos(hLookAngle + 3.14159265359f / 2);
			float jRight = sin(hLookAngle + 3.14159265359f / 2);

			// TODO: use shockfire's keyboard hooks instead

			// down
			if (GetAsyncKeyState('Q') & 0x8000)
			{
				zPos -= moveDelta;
			}

			// up
			if (GetAsyncKeyState('E') & 0x8000)
			{
				zPos += moveDelta;
			}

			// forward
			if (GetAsyncKeyState('W') & 0x8000)
			{
				xPos += iForward * moveDelta;
				yPos += jForward * moveDelta;
				zPos += kForward * moveDelta;
			}

			// back
			if (GetAsyncKeyState('S') & 0x8000)
			{
				xPos -= iForward * moveDelta;
				yPos -= jForward * moveDelta;
				zPos -= kForward * moveDelta;
			}

			// left
			if (GetAsyncKeyState('A') & 0x8000)
			{
				xPos += iRight * moveDelta;
				yPos += jRight * moveDelta;
			}

			// right
			if (GetAsyncKeyState('D') & 0x8000)
			{
				xPos -= iRight * moveDelta;
				yPos -= jRight * moveDelta;
			}

			if (GetAsyncKeyState(VK_UP))
			{
				// TODO: look up
			}
			if (GetAsyncKeyState(VK_DOWN))
			{
				// TODO: look down
			}
			if (GetAsyncKeyState(VK_LEFT))
			{
				// TODO: look left
			}
			if (GetAsyncKeyState(VK_RIGHT))
			{
				// TODO: look right
			}

			if (GetAsyncKeyState('Z') & 0x8000)
			{
				fov -= 0.003f;
			}
			if (GetAsyncKeyState('C') & 0x8000)
			{
				fov += 0.003f;
			}

			// update position
			observerGlobalsPtr(0x180).Write<float>(xPos);
			observerGlobalsPtr(0x184).Write<float>(yPos);
			observerGlobalsPtr(0x188).Write<float>(zPos);

			// update look angles
			observerGlobalsPtr(0x1A8).Write<float>(cos(hLookAngle) * cos(vLookAngle));
			observerGlobalsPtr(0x1AC).Write<float>(sin(hLookAngle) * cos(vLookAngle));
			observerGlobalsPtr(0x1B0).Write<float>(sin(vLookAngle));
			observerGlobalsPtr(0x1B4).Write<float>(-cos(hLookAngle) * sin(vLookAngle));
			observerGlobalsPtr(0x1B8).Write<float>(-sin(hLookAngle) * sin(vLookAngle));
			observerGlobalsPtr(0x1BC).Write<float>(cos(vLookAngle));

			observerGlobalsPtr(0x1A4).Write<float>(fov);
		}
		else if (!mode.compare("spectator"))
		{
			// TODO: disable player input and allow custom controls for cycling through players and adjusting camera orientation

			// check spectator index against max player count
			unsigned long playerIndex = dorito.Modules.Camera.VarSpectatorIndex->ValueInt;
			if (playerIndex >= playersPtr(0x38).Read<unsigned long>())
				return;

			// get player object datum
			uint32_t playerStructAddress = playersPtr(0x44).Read<uint32_t>() + playerIndex * GameGlobals::Players::PlayerEntryLength;
			uint32_t playerObjectDatum = *(uint32_t*)(playerStructAddress + 0x30);
				
			// get player object index
			uint32_t objectIndex = playerObjectDatum & 0xFFFF;

			// TODO: double-check that it's a valid object index and of the bipd class

			// get player object data address
			uint32_t objectAddress = objectHeaderPtr(0x44).Read<uint32_t>() + 0xC + objectIndex * 0x10;
			uint32_t objectDataAddress = *(uint32_t*)objectAddress;
			
			// get player position and look direction
			float x = *(float*)(objectDataAddress + 0x20);
			float y = *(float*)(objectDataAddress + 0x24);
			float z = *(float*)(objectDataAddress + 0x28);
			float i = *(float*)(objectDataAddress + 0x1B8);
			float j = *(float*)(objectDataAddress + 0x1BC);
			float k = *(float*)(objectDataAddress + 0x1C0);

			// vectorize everything
			D3DXVECTOR3 position = D3DXVECTOR3(x, y, z);
			D3DXVECTOR3 forward = D3DXVECTOR3(i, j, k);
			D3DXVECTOR3 absoluteUp = D3DXVECTOR3(0, 0, 1);
			D3DXVECTOR3 left, up;

			// get the up vector relative to the current look direction
			D3DXVec3Cross(&left, &forward, &absoluteUp);
			D3DXVec3Cross(&up, &left, &forward);

			// update camera values
			observerGlobalsPtr(0x180).Write<float>(position.x);
			observerGlobalsPtr(0x184).Write<float>(position.y);
			observerGlobalsPtr(0x188).Write<float>(position.z + 0.3f);
			observerGlobalsPtr(0x194).Write<float>(0.1f);	// z shift
			observerGlobalsPtr(0x1A0).Write<float>(0.5f);	// depth
			observerGlobalsPtr(0x1A8).Write<float>(forward.x);
			observerGlobalsPtr(0x1AC).Write<float>(forward.y);
			observerGlobalsPtr(0x1B0).Write<float>(forward.z);
			observerGlobalsPtr(0x1B4).Write<float>(up.x);
			observerGlobalsPtr(0x1B8).Write<float>(up.y);
			observerGlobalsPtr(0x1BC).Write<float>(up.z);
		}
	}
}