#include "ForgePatchProvider.hpp"
#include "../ElDorito.hpp"

namespace
{
	bool shouldDelete = false;

	void UpdateForgeInputHook();
	char __fastcall UI_Forge_ButtonPressHandlerHook(void* a1, int unused, uint8_t* controllerStruct);
}

namespace Forge
{
	PatchSet ForgePatchProvider::GetPatches()
	{
		auto forgeButtonHook = Utils::Misc::ConvertToVector<uint32_t>((uint32_t)&UI_Forge_ButtonPressHandlerHook);

		PatchSet patches("ForgePatches", 
		{
			Patch("TeleporterRadius", 0xAE4796, 0x90, 0x66),
			// Hook forge dialog button handler in the vftable, so arrow keys can act as bumpers
			// added side effect: analog stick left/right can also navigate through menus
			Patch("ForgeButtonHook", 0x169EFD8, forgeButtonHook)
		},
		{
			Hook("UpdateForgeInput", 0x59D482, UpdateForgeInputHook, HookType::Call)
		});

		return patches;
	}

	void ForgePatchProvider::SignalDelete()
	{
		shouldDelete = true;
	}
}

namespace
{
	__declspec(naked) void UpdateForgeInputHook()
	{
		__asm
		{
			mov al, shouldDelete
			test al, al
			jnz del

			// Not deleting - just call the original function
			push esi
			mov eax, 0x59F0E0
			call eax
			retn 4

		del:
			mov shouldDelete, 0
			// Simulate a Y button press
			mov eax, 0x244D1F0             // Controller data
			mov byte ptr[eax + 0x9E], 1    // Ticks = 1
			and byte ptr[eax + 0x9F], 0xFE // Clear the "handled" flag

			// Call the original function
			push esi
			mov eax, 0x59F0E0
			call eax

			// Make sure nothing else gets the fake press
			mov eax, 0x244D1F0         // Controller data
			or byte ptr[eax + 0x9F], 1 // Set the "handled" flag
			retn 4
		}
	}

	std::chrono::high_resolution_clock::time_point PrevTime = std::chrono::high_resolution_clock::now();
	char __fastcall UI_Forge_ButtonPressHandlerHook(void* a1, int unused, uint8_t* controllerStruct)
	{
		bool usingController = Pointer(0x244DE98).Read<uint32_t>() == 1;
		if (!usingController)
		{
			uint32_t btnCode = *(uint32_t*)(controllerStruct + 0x1C);

			if (btnCode >= Blam::Input::ButtonCodes::eButtonCodesDpadUp && btnCode <= Blam::Input::ButtonCodes::eButtonCodesDpadRight)
				return 1; // ignore the dpad button presses

			auto CurTime = std::chrono::high_resolution_clock::now();
			auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(CurTime - PrevTime);

			if (btnCode == Blam::Input::ButtonCodes::eButtonCodesLeft || btnCode == Blam::Input::ButtonCodes::eButtonCodesRight)
			{
				if (timeSinceLastAction.count() < 200) // 200ms between button presses otherwise it spams the key
					return 1;

				PrevTime = CurTime;

				if (btnCode == Blam::Input::ButtonCodes::eButtonCodesLeft) // analog left / arrow key left
					*(uint32_t*)(controllerStruct + 0x1C) = Blam::Input::ButtonCodes::eButtonCodesLB;

				if (btnCode == Blam::Input::ButtonCodes::eButtonCodesRight) // analog right / arrow key right
					*(uint32_t*)(controllerStruct + 0x1C) = Blam::Input::ButtonCodes::eButtonCodesRB;
			}
		}

		typedef char(__thiscall *UI_Forge_ButtonPressHandlerPtr)(void* a1, void* controllerStruct);
		auto UI_Forge_ButtonPressHandler = reinterpret_cast<UI_Forge_ButtonPressHandlerPtr>(0xAE2180);
		return UI_Forge_ButtonPressHandler(a1, controllerStruct);
	}
}