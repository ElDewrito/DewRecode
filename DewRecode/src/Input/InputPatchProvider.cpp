#include "InputPatchProvider.hpp"
#include "../ElDorito.hpp"

#include <stack>

namespace
{
	int controllerIndex = 0;

	void AimAssistHook();
	void RawInputHook();

	void Input_ControllerIndexHook();

	void ProcessGameInputHook();
	void ProcessUiInputHook();
	void KeyTestHook();

	std::stack<std::shared_ptr<Input::InputContext>> contextStack;
	std::vector<Input::DefaultInputHandler> defaultHandlers;
	bool contextDone = false;
}

namespace Input
{
	PatchSet InputPatchProvider::GetPatches()
	{
		PatchSet patches("InputPatches",
		{
			Patch("ControllerIndexNop", 0x5128F0, 0x90, 4)
		},
		{
			Hook("ProcessGameInputHook", 0x505C35, ProcessGameInputHook, HookType::Call),
			Hook("ProcessUiInputHook", 0x505CBA, ProcessUiInputHook, HookType::Call),
			Hook("KeyTestHook", 0x511B66, KeyTestHook, HookType::Call),
			Hook("KeyTestHook", 0x511CE6, KeyTestHook, HookType::Call),
			// Aim assist hook
			Hook("AimAssist", 0x58AA17, AimAssistHook, HookType::Jmp),
			// Hook the input handling routine to fix mouse acceleration
			Hook("RawInput", 0x512395, RawInputHook, HookType::Jmp),

			Hook("ControllerIndex", 0x5128EB, Input_ControllerIndexHook, HookType::Jmp)
		});

		return patches;
	}

	void InputPatchProvider::RegisterCallbacks(IEngine* engine)
	{
		RegisterDefaultInputHandler(std::bind(&InputPatchProvider::KeyboardUpdated, this));
	}

	void InputPatchProvider::PushContext(std::shared_ptr<InputContext> context)
	{
		if (!contextStack.empty())
			contextStack.top()->InputDeactivated();
		contextStack.push(context);
		context->InputActivated();
	}

	void InputPatchProvider::RegisterDefaultInputHandler(DefaultInputHandler func)
	{
		defaultHandlers.push_back(func);
	}

	void InputPatchProvider::KeyboardUpdated()
	{
		auto& dorito = ElDorito::Instance();
		for (auto i = 0; i < Blam::Input::eKeyCodes_Count; i++)
		{
			const auto binding = dorito.CommandManager.GetBinding(i);
			if (binding->command.empty())
				continue; // Key is not bound

			// Read the key
			auto keyCode = static_cast<Blam::Input::KeyCodes>(i);
			auto keyTicks = dorito.Engine.GetKeyTicks(keyCode, Blam::Input::eInputTypeSpecial);

			auto command = binding->command;
			auto isHold = command.at(0) == '+';
			if (isHold)
				command = command.substr(1);

			// We're only interested in the key if it was just pressed or if
			// this is a hold binding and it was just released
			if (keyTicks > 1 || (keyTicks == 0 && !(isHold && binding->active)))
				continue;

			if (isHold)
			{
				// The command is a hold binding - append an argument depending
				// on whether it was pressed or released
				if (keyTicks == 1)
					command += " 1";
				else if (keyTicks == 0)
					command += " 0";

				binding->active = (keyTicks > 0);
			}

			// Execute the command and print its result
			dorito.Engine.PrintToConsole(dorito.CommandManager.Execute(command, true));
		}
	}

	void InputPatchProvider::SetControllerIndex(int idx)
	{
		controllerIndex = idx;
	}
}

namespace
{
	void PopContext()
	{
		contextStack.top()->InputDeactivated();
		contextStack.pop();
		if (!contextStack.empty())
			contextStack.top()->InputActivated();
	}

	void ProcessGameInputHook()
	{
		// If the current context is done, pop it off
		if (contextDone)
		{
			PopContext();
			contextDone = false;
		}

		if (!contextStack.empty())
		{
			// Tick the active context
			if (!contextStack.top()->GameInputTick())
				contextDone = true;
		}
		else
		{
			// Run default handlers
			for (auto &&handler : defaultHandlers)
				handler();
		}
		
		// Run default input
		typedef void(*DefaultProcessInputPtr)();
		auto DefaultProcessInput = reinterpret_cast<DefaultProcessInputPtr>(0x60D880);
		DefaultProcessInput();
	}

	void ProcessUiInputHook()
	{
		// Pump Windows messages (replaced function)
		typedef void(*PumpMessagesPtr)();
		auto PumpMessages = reinterpret_cast<PumpMessagesPtr>(0x508170);
		PumpMessages();

		// Tick the active context
		if (!contextStack.empty() && !contextStack.top()->UiInputTick())
			contextDone = true;
	}

	// Returns true if a key should be blocked.
	bool KeyTestHookImpl(Blam::Input::InputType type)
	{
		return (type == Blam::Input::eInputTypeGame || type == Blam::Input::eInputTypeSpecial) && !contextStack.empty();
	}

	// Hook for the engine's "get key ticks" and "get key ms" functions. If the
	// zero flag is not set when this returns, the key will be blocked.
	__declspec(naked) void KeyTestHook()
	{
		__asm
		{
			// Replaced code (checks if the input type is blocked)
			cmp byte ptr ds:0x238DBEB[eax], 0
			jnz done

			push eax
			call KeyTestHookImpl
			add esp, 4
			test eax, eax

		done:
			// The replaced instruction is 7 bytes long, so get the return
			// address and add 2 without affecting the flags
			pop eax
			lea eax, [eax + 2]
			jmp eax
		}
	}

	__declspec(naked) void Input_ControllerIndexHook()
	{
		__asm
		{
			mov edi, controllerIndex
			mov esi, eax
			mov ebx, 0x238E714
			push 0x5128F4
			ret
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
		auto camMode = dorito.Utils.ToLower(dorito.CameraCommands->VarMode->ValueString);

		// Disable mouse input if a controller is plugged in (this needs to be done
		// even if raw input is off). Leave it enabled while superman.
		auto controllerEnabled = Pointer(0x244DE98).Read<bool>();
		if (controllerEnabled && camMode != "flying")
			return true;

		if (!dorito.InputCommands->VarRawInput->ValueInt)
			return false;

		if (rwInput->header.dwType != RIM_TYPEMOUSE)
			return true;

		Pointer InputPtr = dorito.Engine.GetMainTls(GameGlobals::Input::TLSOffset)[0];
		if (!InputPtr)
			return true;
		Pointer &horizPtr = InputPtr(GameGlobals::Input::ViewAngleHorizontal);
		Pointer &vertPtr = InputPtr(GameGlobals::Input::ViewAngleVertical);
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

		Pointer PlayerData = dorito.Engine.GetMainTls(GameGlobals::Observer::TLSOffset)[0];
		if (!PlayerData)
			return true;
		Pointer vehicleData = Pointer(PlayerData(GameGlobals::Observer::VehicleData).Read<uint32_t>()); // Note: this has data for each local player, but since there's no splitscreen support yet, player index is always 0
		char isInVehicle = 0;
		if (vehicleData)
			isInVehicle = vehicleData(GameGlobals::Observer::VehicleDataIsInVehicle).Read<char>();

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