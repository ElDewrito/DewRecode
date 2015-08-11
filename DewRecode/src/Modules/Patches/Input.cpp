#include "Input.hpp"
#include "../../ElDorito.hpp"

namespace
{
	bool swallowedKeys[Blam::eKeyCodes_Count];
	void KeyboardUpdateHandler()
	{
		auto& dorito = ElDorito::Instance();
		memset(swallowedKeys, 0, sizeof(swallowedKeys));

		dorito.Engine.Event("Core", "Input.KeyboardUpdate", &swallowedKeys);
	}

	// Hook for the keyboard update function to call our update handler after
	// the function returns
	__declspec(naked) void KeyboardUpdateHook()
	{
		__asm
		{
			// Epilogue of hooked function
			mov esp, ebp
			pop ebp

			jmp KeyboardUpdateHandler
		}
	}

	// Hook for the engine's "get key ticks" and "get key ms" functions to
	// block the key if it's been swallowed. If the zero flag is not set when
	// this returns, the key will be blocked.
	__declspec(naked) void KeyTestHook()
	{
		__asm
		{
			// Replaced code (checks if the input type is blocked)
			cmp byte ptr ds:0x238DBEB[eax], 0
			jnz done

			// Also block the key if it's been swallowed
			movsx eax, word ptr[ebp + 8] // Get key code
			cmp byte ptr swallowedKeys[eax], 0

		done:
			// The replaced instruction is 7 bytes long, so get the return
			// address and add 2 without affecting the flags
			pop eax
			lea eax, [eax + 2]
			jmp eax
		}
	}

	__declspec(naked) void AimAssistHook()
	{
		__asm
		{
			// Check if the player is using a mouse
			mov edx, 0x244DE98
			mov edx, [edx]
			test edx, edx
			jnz controller

			// Set magnetism angle to 0
			xor edx, edx
			mov[edi + 0x14], edx

			// Skip past magnetism angle code
			mov edx, 0x58AA23
			jmp edx

		controller:
			// Load magnetism angle normally
			movss xmm0, dword ptr[ebx + 0x388]
			mov edx, 0x58AA1F
			jmp edx
		}
	}

	bool RawInputHookImpl(RAWINPUT* rwInput)
	{
		auto& dorito = ElDorito::Instance();

		// Disable mouse input if a controller is plugged in (this needs to be done
		// even if raw input is off)
		auto controllerEnabled = Pointer(0x244DE98).Read<bool>();
		if (controllerEnabled)
			return true;

		if (!dorito.Modules.Input.VarInputRawInput->ValueInt)
			return false;
		if (rwInput->header.dwType != RIM_TYPEMOUSE)
			return true;

		Pointer InputPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		if (!InputPtr)
			return true;
		Pointer &horizPtr = InputPtr(GameGlobals::Input::HorizontalViewAngle);
		Pointer &vertPtr = InputPtr(GameGlobals::Input::VerticalViewAngle);
		float currentHoriz = horizPtr.Read<float>();
		float currentVert = vertPtr.Read<float>();
		float weaponSensitivity = Pointer(0x50DEF14).Read<float>();
		float maxVertAngle = Pointer(0x18B49E4).Read<float>();

		Pointer SettingsPtr = dorito.Engine.GetMainTls(GameGlobals::GameSettings::TLSOffset)[0];
		if (SettingsPtr == 0) // game itself does this the same way, not sure why it'd be 0 in TLS data though since the game is also meant to set it in TLS if its 0
			SettingsPtr = Pointer(0x22C0128);

		Pointer &invertedPtr = SettingsPtr(GameGlobals::GameSettings::YAxisInverted);
		Pointer &yaxisPtr = SettingsPtr(GameGlobals::GameSettings::YAxisSensitivity);
		Pointer &xaxisPtr = SettingsPtr(GameGlobals::GameSettings::XAxisSensitivity);
		Pointer &yaxisPtrVehicle = SettingsPtr(GameGlobals::GameSettings::VehicleYAxisSensitivity);
		Pointer &xaxisPtrVehicle = SettingsPtr(GameGlobals::GameSettings::VehicleXAxisSensitivity);
		uint32_t isInverted = invertedPtr.Read<uint32_t>();
		float yaxisSens = (float)yaxisPtr.Read<uint32_t>() / 25.f;
		float xaxisSens = (float)xaxisPtr.Read<uint32_t>() / 25.f;

		Pointer PlayerData = dorito.Engine.GetMainTls(GameGlobals::PlayerAlt::TLSOffset)[0];
		if (!PlayerData)
			return true;
		Pointer vehicleData = Pointer(PlayerData(GameGlobals::PlayerAlt::VehicleData).Read<uint32_t>()); // Note: this has data for each local player, but since there's no splitscreen support yet, player index is always 0
		char isInVehicle = 0;
		if (vehicleData)
			isInVehicle = vehicleData(GameGlobals::PlayerAlt::VehicleDataIsInVehicle).Read<char>();

		if (isInVehicle != 0)
		{
			yaxisSens = (float)yaxisPtrVehicle.Read<uint32_t>() / 25.f;
			xaxisSens = (float)xaxisPtrVehicle.Read<uint32_t>() / 25.f;
		}

		if (isInverted != 0)
			currentVert += (float)rwInput->data.mouse.lLastY * yaxisSens / 1000.0f / weaponSensitivity;
		else
			currentVert -= (float)rwInput->data.mouse.lLastY * yaxisSens / 1000.0f / weaponSensitivity;

		if (currentVert > maxVertAngle)
			currentVert = maxVertAngle;
		else if (currentVert < (0.f - maxVertAngle))
			currentVert = (0.f - maxVertAngle);

		currentHoriz -= (float)rwInput->data.mouse.lLastX * xaxisSens / 1000.0f / weaponSensitivity;

		horizPtr.Write(currentHoriz);
		vertPtr.Write(currentVert);
		return true;
	}

	__declspec(naked) void RawInputHook()
	{
		__asm
		{
			lea eax, [ebp - 0x2C] // Address of RAWINPUT structure
			push eax
			call RawInputHookImpl
			add esp, 4
			test al, al
			jz disabled

			// Skip past the game's mouse handling code
			mov ecx, [ebp - 0x18]
			push 0x5123AA
			ret

		disabled:
			// Run the game's mouse handling code
			mov eax, [ebp - 0x10]
			add ds : [0x238E6C0], eax
			push 0x51239E
			ret
		}
	}
}

namespace Modules
{
	PatchModuleInput::PatchModuleInput() : ModuleBase("Patches.Input")
	{
		AddModulePatches({},
		{
			Hook("KeyboardUpdate", 0x51299D, KeyboardUpdateHook, HookType::Jmp),
			Hook("KeyTest1", 0x511B66, KeyTestHook, HookType::Call),
			Hook("KeyTest2", 0x511CE6, KeyTestHook, HookType::Call),
			// Aim assist hook
			Hook("AimAssist", 0x58AA17, AimAssistHook, HookType::Jmp),
			// Hook the input handling routine to fix mouse acceleration
			Hook("RawInput", 0x512395, RawInputHook, HookType::Jmp)
		});
	}

	uint8_t PatchModuleInput::GetKeyTicks(Blam::KeyCodes key, Blam::InputType type)
	{
		typedef uint8_t(*EngineGetKeyTicksPtr)(Blam::KeyCodes, Blam::InputType);
		auto EngineGetKeyTicks = reinterpret_cast<EngineGetKeyTicksPtr>(0x511B60);
		return EngineGetKeyTicks(key, type);
	}

	uint16_t PatchModuleInput::GetKeyMs(Blam::KeyCodes key, Blam::InputType type)
	{
		typedef uint8_t(*EngineGetKeyMsPtr)(Blam::KeyCodes, Blam::InputType);
		auto EngineGetKeyMs = reinterpret_cast<EngineGetKeyMsPtr>(0x511CE0);
		return EngineGetKeyMs(key, type);
	}

	void PatchModuleInput::BlockInput(Blam::InputType type, bool block)
	{
		typedef uint8_t(*EngineBlockInputPtr)(Blam::InputType, bool);
		auto EngineBlockInput = reinterpret_cast<EngineBlockInputPtr>(0x511CE0);
		EngineBlockInput(type, block);
	}

	void PatchModuleInput::Swallow(Blam::KeyCodes key)
	{
		if (key >= 0 && key < Blam::eKeyCodes_Count)
			swallowedKeys[key] = true;
	}
}