#include "ModuleForge.hpp"
#include "../ElDorito.hpp"

namespace
{
	bool shouldDelete = false;

	bool CommandForgeDeleteItem(const std::vector<std::string>& arguments, std::string& returnInfo)
	{
		shouldDelete = true;
		return true;
	}

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
		uint32_t btnCode = *(uint32_t*)(controllerStruct + 0x1C);

		auto CurTime = std::chrono::high_resolution_clock::now();
		auto timeSinceLastAction = std::chrono::duration_cast<std::chrono::milliseconds>(CurTime - PrevTime);

		if (btnCode == Blam::ButtonCodes::eButtonCodesLeft || btnCode == Blam::ButtonCodes::eButtonCodesRight)
		{
			if (timeSinceLastAction.count() < 200) // 200ms between button presses otherwise it spams the key
				return 1;

			PrevTime = CurTime;

			if (btnCode == Blam::ButtonCodes::eButtonCodesLeft) // analog left / arrow key left
				*(uint32_t*)(controllerStruct + 0x1C) = Blam::ButtonCodes::eButtonCodesLB;

			if (btnCode == Blam::ButtonCodes::eButtonCodesRight) // analog right / arrow key right
				*(uint32_t*)(controllerStruct + 0x1C) = Blam::ButtonCodes::eButtonCodesRB;
		}

		typedef char(__thiscall *UI_Forge_ButtonPressHandler)(void* a1, void* controllerStruct);
		UI_Forge_ButtonPressHandler buttonHandler = (UI_Forge_ButtonPressHandler)0xAE2180;
		return buttonHandler(a1, controllerStruct);
	}
}
namespace Modules
{
	ModuleForge::ModuleForge() : ModuleBase("Forge")
	{
		AddCommand("DeleteItem", "forge_delete", "Deletes the Forge item under the crosshairs", eCommandFlagsNone, CommandForgeDeleteItem);

		AddModulePatches({},
		{
			Hook("UpdateForgeInput", 0x59D482, UpdateForgeInputHook, HookType::Call)
		});

		// Hook forge dialog button handler in the vftable, so arrow keys can act as bumpers
		// added side effect: analog stick left/right can also navigate through menus
		Pointer(0x169EFD8).Write<uint32_t>((uint32_t)&UI_Forge_ButtonPressHandlerHook);
	}
}