#include "CameraPatchProvider.hpp"
#include "../ElDorito.hpp"
#include <d3dx9.h>

namespace
{
	void UpdateCameraDefinitions();
	void UpdateCameraDefinitionsAlt1();
	void UpdateCameraDefinitionsAlt2();
	void UpdateCameraDefinitionsAlt3();
}

namespace Camera
{
	CameraPatchProvider::CameraPatchProvider()
	{
		auto& patches = ElDorito::Instance().PatchManager;

		CustomModePatches = patches.AddPatchSet("CustomModePatches",
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
		StaticModePatches = patches.AddPatchSet("DisableStaticUpdatePatches",
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

		HideHudPatch = patches.AddPatch("HideHud", 0x16B5A5C, { 0xC3, 0xF5, 0x48, 0x40 }); // 3.14f in hex form
		CenteredCrosshairPatch = patches.AddPatch("CenteredCrosshair", 0x65FA43, { 0x31, 0xC0, 0x90, 0x90 });
	}

	void CameraPatchProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnTick(BIND_CALLBACK(this, &CameraPatchProvider::UpdatePosition));
	}

	void CameraPatchProvider::UpdatePosition(const std::chrono::duration<double>& deltaTime)
	{
		auto& dorito = ElDorito::Instance();
		auto mode = dorito.Utils.ToLower(dorito.CameraCommands->VarMode->ValueString);

		Pointer &observerGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Observer::TLSOffset)[0];
		Pointer &playerControlGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		auto* playersPtr = dorito.Engine.GetArrayGlobal(GameGlobals::Players::TLSOffset);
		auto* objectHeaderPtr = dorito.Engine.GetArrayGlobal(GameGlobals::ObjectHeader::TLSOffset);

		// only allow flycam input outside of cli/chat
		if (!mode.compare("flying")) //TODO2: && !dorito.Modules.Console.IsVisible())
		{
			float moveDelta = dorito.CameraCommands->VarSpeed->ValueFloat;
			float lookDelta = 0.01f;	// not used yet

			// current values
			float hLookAngle = playerControlGlobalsPtr(GameGlobals::Input::ViewAngleHorizontal).Read<float>();
			float vLookAngle = playerControlGlobalsPtr(GameGlobals::Input::ViewAngleVertical).Read<float>();
			float xPos = observerGlobalsPtr(GameGlobals::Observer::CameraPositionX).Read<float>();
			float yPos = observerGlobalsPtr(GameGlobals::Observer::CameraPositionY).Read<float>();
			float zPos = observerGlobalsPtr(GameGlobals::Observer::CameraPositionZ).Read<float>();
			float xShift = observerGlobalsPtr(GameGlobals::Observer::CameraShiftX).Read<float>();
			float yShift = observerGlobalsPtr(GameGlobals::Observer::CameraShiftY).Read<float>();
			float zShift = observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Read<float>();
			float hShift = observerGlobalsPtr(GameGlobals::Observer::CameraShiftHorizontal).Read<float>();
			float vShift = observerGlobalsPtr(GameGlobals::Observer::CameraShiftVertical).Read<float>();
			float depth = observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Read<float>();
			float fov = observerGlobalsPtr(GameGlobals::Observer::CameraFieldOfView).Read<float>();
			float iForward = observerGlobalsPtr(GameGlobals::Observer::CameraForwardI).Read<float>();
			float jForward = observerGlobalsPtr(GameGlobals::Observer::CameraForwardJ).Read<float>();
			float kForward = observerGlobalsPtr(GameGlobals::Observer::CameraForwardK).Read<float>();
			float iUp = observerGlobalsPtr(GameGlobals::Observer::CameraUpI).Read<float>();
			float jUp = observerGlobalsPtr(GameGlobals::Observer::CameraUpJ).Read<float>();
			float kUp = observerGlobalsPtr(GameGlobals::Observer::CameraUpK).Read<float>();
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
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionX).Write<float>(xPos);
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionY).Write<float>(yPos);
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionZ).Write<float>(zPos);

			// update look angles
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardI).Write<float>(cos(hLookAngle) * cos(vLookAngle));
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardJ).Write<float>(sin(hLookAngle) * cos(vLookAngle));
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardK).Write<float>(sin(vLookAngle));
			observerGlobalsPtr(GameGlobals::Observer::CameraUpI).Write<float>(-cos(hLookAngle) * sin(vLookAngle));
			observerGlobalsPtr(GameGlobals::Observer::CameraUpJ).Write<float>(-sin(hLookAngle) * sin(vLookAngle));
			observerGlobalsPtr(GameGlobals::Observer::CameraUpK).Write<float>(cos(vLookAngle));

			observerGlobalsPtr(GameGlobals::Observer::CameraFieldOfView).Write<float>(fov);
		}
		else if (!mode.compare("spectator"))
		{
			// TODO: disable player input and allow custom controls for cycling through players and adjusting camera orientation

			// check spectator index against max player count
			int playerIndex = (int)dorito.CameraCommands->VarSpectatorIndex->ValueInt;
			if (playerIndex >= playersPtr->GetCount())
				return;

			// get player object datum
			auto playerStructAddress = playersPtr->GetEntry(playerIndex);
			uint32_t playerObjectDatum = playerStructAddress(0x30).Read<uint32_t>();

			// get player object index
			uint32_t objectIndex = playerObjectDatum & 0xFFFF;

			// TODO: double-check that it's a valid object index and of the bipd class

			// get player object data address
			auto objectAddress = objectHeaderPtr->GetEntry(objectIndex);
			uint32_t objectDataAddress = objectAddress(0xC).Read<uint32_t>();

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
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionX).Write<float>(position.x);
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionY).Write<float>(position.y);
			observerGlobalsPtr(GameGlobals::Observer::CameraPositionZ).Write<float>(position.z + 0.3f);
			observerGlobalsPtr(GameGlobals::Observer::CameraShiftZ).Write<float>(0.1f);
			observerGlobalsPtr(GameGlobals::Observer::CameraDepth).Write<float>(0.5f);
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardI).Write<float>(forward.x);
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardJ).Write<float>(forward.y);
			observerGlobalsPtr(GameGlobals::Observer::CameraForwardK).Write<float>(forward.z);
			observerGlobalsPtr(GameGlobals::Observer::CameraUpI).Write<float>(up.x);
			observerGlobalsPtr(GameGlobals::Observer::CameraUpJ).Write<float>(up.y);
			observerGlobalsPtr(GameGlobals::Observer::CameraUpK).Write<float>(up.z);
		}
	}
}

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
		auto mode = dorito.Utils.ToLower(dorito.CameraCommands->VarMode->ValueString);

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
}