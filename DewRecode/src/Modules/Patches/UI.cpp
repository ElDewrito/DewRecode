#include "UI.hpp"
#include "../../ElDorito.hpp"
#include <algorithm>

namespace
{
	void __fastcall UI_MenuUpdateHook(void* a1, int unused, int menuIdToLoad)
	{
		auto& engine = ElDorito::Instance().Engine;
		if (!engine.HasMainMenuShown() && menuIdToLoad == 0x10083)
			engine.Event("Core", "Engine.MainMenuShown");

		bool shouldUpdate = *(DWORD*)((uint8_t*)a1 + 0x10) >= 0x1E;
		int uiData0x18Value = 1;
		//if (menuIdToLoad == 0x100A8) // TODO1: find what 0x100A8(H3E) stringid is in HO
		//	uiData0x18Value = 5;

		typedef void(__thiscall *UI_MenuUpdatePtr)(void* a1, int menuIdToLoad);
		auto UI_MenuUpdate = reinterpret_cast<UI_MenuUpdatePtr>(0xADF6E0);
		UI_MenuUpdate(a1, menuIdToLoad);

		if (shouldUpdate)
		{
			typedef void*(__cdecl * UI_AllocPtr)(int size);
			auto UI_Alloc = reinterpret_cast<UI_AllocPtr>(0xAB4ED0);
			void* UIData = UI_Alloc(0x3C);

			// fill UIData with proper data
			typedef void*(__thiscall * UI_OpenDialogById_InitMessagePtr)(void* a1, unsigned int dialogStringId, int a3, int dialogFlags, unsigned int parentDialogStringId);
			auto UI_OpenDialogById_InitMessage = reinterpret_cast<UI_OpenDialogById_InitMessagePtr>(0xA92780);
			UI_OpenDialogById_InitMessage(UIData, menuIdToLoad, 0xFF, 4, 0x1000D);

			// post UI message
			typedef int(*UI_PostMessagePtr)(void* uiDataStruct);
			auto UI_PostMessage = reinterpret_cast<UI_PostMessagePtr>(0xA93450);
			UI_PostMessage(UIData);

			*(uint32_t*)((char*)UIData + 0x18) = uiData0x18Value;
		}
	}

	int UI_ShowHalo3PauseMenu(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
	{
		auto& uiPatches = ElDorito::Instance().Modules.UIPatches;
		uiPatches.UIData = 0; // hacky fix for h3 pause menu, i think each different DialogParentStringId should have it's own UIData ptr in a std::map or something
		// so that the games ptr to the UIData ptr is always the same for that dialog parent id

		uiPatches.DialogStringId = 0x10084;
		uiPatches.DialogArg1 = 0;
		uiPatches.DialogFlags = 4;
		uiPatches.DialogParentStringId = 0x1000C;
		uiPatches.DialogShow = true;

		return 1;
	}

	bool LocalizedStringHookImpl(int tagIndex, int stringId, wchar_t *outputBuffer)
	{
		const size_t MaxStringLength = 0x400;

		switch (stringId)
		{
		case 0x1010A: // start_new_campaign
		{
			// Get the version string, convert it to uppercase UTF-16, and return it
			std::string version = Utils::Version::GetVersionString();
			std::transform(version.begin(), version.end(), version.begin(), toupper);
			std::wstring unicodeVersion(version.begin(), version.end());
			swprintf(outputBuffer, MaxStringLength, L"ELDEWRITO %s", unicodeVersion.c_str());
			return true;
		}
		}
		return false;
	}

	__declspec(naked) void LocalizedStringHook()
	{
		__asm
		{
			// Run the hook implementation function and fallback to the original if it returned false
			push ebp
			mov ebp, esp
			push[ebp + 0x10]
			push[ebp + 0xC]
			push[ebp + 0x8]
			call LocalizedStringHookImpl
			add esp, 0xC
			test al, al
			jz fallback

			// Don't run the original function
			mov esp, ebp
			pop ebp
			ret

		fallback:
			// Execute replaced code and jump back to original function
			sub esp, 0x800
			mov edx, 0x51E049
			jmp edx
		}
	}

	void WindowTitleSprintfHook(char* destBuf, char* format, char* version)
	{
		std::string windowTitle = "ElDewrito | Version: " + Utils::Version::GetVersionString() + " | Build Date: " __DATE__;
		strcpy_s(destBuf, 0x40, windowTitle.c_str());
	}

	void UIPatches_ApplyMapNameFixes(void* param)
	{
		uint32_t levelsGlobalPtr = Pointer(0x189E2E0).Read<uint32_t>();
		if (!levelsGlobalPtr)
			return;

		// TODO: map out these global arrays, content items seems to use same format

		uint32_t numLevels = Pointer(levelsGlobalPtr + 0x34).Read<uint32_t>();

		const wchar_t* search[6] = { L"guardian", L"riverworld", L"s3d_avalanche", L"s3d_edge", L"s3d_reactor", L"s3d_turf" };
		const wchar_t* names[6] = { L"Guardian", L"Valhalla", L"Diamondback", L"Edge", L"Reactor", L"Icebox" };
		// TODO: Get names/descs using string ids? Seems the unic tags have descs for most of the maps
		const wchar_t* descs[6] = {
			L"Millennia of tending has produced trees as ancient as the Forerunner structures they have grown around. 2-6 players.",
			L"The crew of V-398 barely survived their unplanned landing in this gorge...this curious gorge. 6-16 players.",
			L"Hot winds blow over what should be a dead moon. A reminder of the power Forerunners once wielded. 6-16 players.",
			L"The remote frontier world of Partition has provided this ancient databank with the safety of seclusion. 6-16 players.",
			L"Being constructed just prior to the Invasion, its builders had to evacuate before it was completed. 6-16 players.",
			L"Though they dominate on open terrain, many Scarabs have fallen victim to the narrow streets of Earth's cities. 4-10 players."
		};
		for (uint32_t i = 0; i < numLevels; i++)
		{
			Pointer levelNamePtr = Pointer(levelsGlobalPtr + 0x54 + (0x360 * i) + 0x8);
			Pointer levelDescPtr = Pointer(levelsGlobalPtr + 0x54 + (0x360 * i) + 0x8 + 0x40);

			wchar_t levelName[0x21] = { 0 };
			levelNamePtr.Read(levelName, sizeof(wchar_t) * 0x20);

			for (uint32_t y = 0; y < sizeof(search) / sizeof(*search); y++)
			{
				if (wcscmp(search[y], levelName) == 0)
				{
					memset(levelNamePtr, 0, sizeof(wchar_t) * 0x20);
					wcscpy_s(levelNamePtr, 0x20, names[y]);

					memset(levelDescPtr, 0, sizeof(wchar_t) * 0x80);
					wcscpy_s(levelDescPtr, 0x80, descs[y]);
					break;
				}
			}
		}
	}

	// TODO: make this use a lambda func instead once VC supports converting lambdas to funcptrs
	void UIPatches_TickCallback(const std::chrono::duration<double>& deltaTime)
	{
		ElDorito::Instance().Modules.UIPatches.Tick(deltaTime);
	}

	// Fix to ignore duplicate button presses when using keyboard
	// TODO: find the proper fix for this, it seems like when your on the pause menu / forge menu it sends 3 button presses when you press a key (two dpad presses and an analog press)
	// eg. for dpad right it sends DpadRight (sent @ 0xA93E40), DpadRight (sent @ 0xA941B9), Right (sent @ 0xA93E23)
	// only sends one button press on the main menu though, which is strange
	// we just ignore the dpad button presses so options don't get skipped.
	int __fastcall c_start_menu__ButtonPressHook(void* thisPtr, int unused, uint8_t* controllerStruct)
	{
		bool usingController = Pointer(0x244DE98).Read<uint32_t>() == 1;
		if (!usingController)
		{
			uint32_t btnCode = *(uint32_t*)(controllerStruct + 0x1C);

			if (btnCode >= Blam::ButtonCodes::eButtonCodesDpadUp && btnCode <= Blam::ButtonCodes::eButtonCodesDpadRight)
				return 1; // ignore the dpad button presses
		}

		typedef int(__thiscall* c_start_menu__ButtonPressPtr)(void* thisPtr, uint8_t* controllerStruct);
		auto c_start_menu__ButtonPress = reinterpret_cast<c_start_menu__ButtonPressPtr>(0xB1F620);
		return c_start_menu__ButtonPress(thisPtr, controllerStruct);
	}

	int __fastcall c_main_menu_screen_widget__ButtonPressHook(void* thisPtr, int unused, uint8_t* controllerStruct)
	{
		uint32_t btnCode = *(uint32_t*)(controllerStruct + 0x1C);
		if (btnCode == Blam::ButtonCodes::eButtonCodesStart)
			return UI_ShowHalo3PauseMenu(0, 0, 0, 0, 0);

		typedef int(__thiscall* c_main_menu_screen_widget__ButtonPressPtr)(void* thisPtr, uint8_t* controllerStruct);
		auto c_main_menu_screen_widget__ButtonPress = reinterpret_cast<c_main_menu_screen_widget__ButtonPressPtr>(0xAE7660);
		return c_main_menu_screen_widget__ButtonPress(thisPtr, controllerStruct);
	}

	// hooks the lobbys button handler and opens game settings window if X or Start is pressed
	int __fastcall c_gui_screen_pregame_lobby__ButtonPressHook(void* thisPtr, int unused, uint8_t* controllerStruct)
	{
		uint32_t btnCode = *(uint32_t*)(controllerStruct + 0x1C);
		if (btnCode == Blam::ButtonCodes::eButtonCodesStart || btnCode == Blam::ButtonCodes::eButtonCodesX)
		{
			typedef int(__thiscall* c_gui_screen_pregame_lobby__OpenGameSettingsPtr)(void* thisPtr, uint32_t a2);
			auto c_gui_screen_pregame_lobby__OpenGameSettings = reinterpret_cast<c_gui_screen_pregame_lobby__OpenGameSettingsPtr>(0xB225B0);
			return c_gui_screen_pregame_lobby__OpenGameSettings(thisPtr, *(uint32_t*)(controllerStruct + 0x10));

			return 1;
		}

		typedef int(__thiscall* c_gui_screen_pregame_lobby__ButtonPressPtr)(void* thisPtr, uint8_t* controllerStruct);
		auto c_gui_screen_pregame_lobby__ButtonPress = reinterpret_cast<c_gui_screen_pregame_lobby__ButtonPressPtr>(0xB21A20);
		return c_gui_screen_pregame_lobby__ButtonPress(thisPtr, controllerStruct);
	}
}

namespace Modules
{
	PatchModuleUI::PatchModuleUI() : ModuleBase("Patches.UI")
	{
		// register our tick callbacks
		engine->OnTick(UIPatches_TickCallback);
		engine->OnEvent("Core", "Engine.FirstTick", UIPatches_ApplyMapNameFixes);

		// add our patches
		AddModulePatches(
		{
			// Fix "Network" setting in lobbies (change broken 0x100B7 menuID to 0x100B6)
			Patch("NetworkFix", 0xAC34B0, { 0xB6 }),

			// Fix gamepad option in settings (todo: find out why it doesn't detect gamepads
			// and find a way to at least enable pressing ESC when gamepad is enabled)
			Patch("GamepadFix", 0x60D7F2, 0x90, 2),

			// Remove Xbox Live from the network menu
			Patch("RemoveXBL1", 0xB23D85, 0x90, 0x17),
			Patch("RemoveXBL2", 0xB23DA1, { 0 }),
			Patch("RemoveXBL3", 0xB23DB8, { 1 }),
			Patch("RemoveXBL4", 0xB23DFF, 0x90, 3),
			Patch("RemoveXBL5", 0xB23E07, { 0 }),
			Patch("RemoveXBL6", 0xB23E1C, { 0 }),

			// Remove "BUILT IN" text when choosing map/game variants by feeding the UI_SetVisiblityOfElement func a nonexistant string ID for the element (0x202E8 instead of 0x102E8)
			// TODO: find way to fix this text instead of removing it, since the 0x102E8 element (subitem_edit) is used for other things like editing/viewing map variant metadata
			Patch("RemoveBuiltInText", 0xB05D6F, { 0x2 }),

			// Fix for leave game button to show H3 pause menu
			Patch("ShowH3PauseMenu1", 0x7B682B, 0x90, 1),

			// Allows you to press B to close the H3 pause menu
			// TODO: find out what the byte that's being checked does, we're just patching out the check here but maybe it's important
			Patch("CloseStartMenuFix", 0xAE05F3, 0x90, 2),

			// Allows you to semi-close the voice settings menu by pressing B (pressing it once takes you to blank menu, pressing it again goes back to pause menu)
			// TODO: find a proper fix, since the part that gets patched here is used by a lot of different dialogs. Alternatively we could remove this menu altogether.
			Patch("CloseVoiceMenuFix", 0xAB130D, 0x90, 2)
		}, 
		{
			// Fix for leave game button to show H3 pause menu
			Hook("ShowH3PauseMenu2", 0x7B6826, UI_ShowHalo3PauseMenu, HookType::Call),

			// Fix menu update code to include missing mainmenu code
			Hook("MenuUpdate", 0xADFB73, UI_MenuUpdateHook, HookType::Call),

			// Localized string override hook
			Hook("LocalizedString", 0x51E040, LocalizedStringHook, HookType::Jmp),

			// Hook window title sprintf to replace the dest buf with our string
			Hook("WindowTitle", 0x42EB84, WindowTitleSprintfHook, HookType::Call)
		});

		// hook c_start_menu::ButtonPress on each c_start_menu_* vftable
		Pointer(0x0169FE08).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x0169FFA8).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A0118).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A0350).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A0488).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A18B0).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A1BE8).Write((uint32_t)&c_start_menu__ButtonPressHook);
		Pointer(0x016A6C80).Write((uint32_t)&c_start_menu__ButtonPressHook);

		// hook c_main_menu_screen_widget::ButtonPress so we can show settings menu if they push start
		Pointer(0x0169FCA8).Write((uint32_t)&c_main_menu_screen_widget__ButtonPressHook);

		// hook c_gui_screen_pregame_lobby::ButtonPress so we can open game options
		// Sorta hacky way of getting game options screen to show when you press X/start in lobby
		// TODO: find real way of showing the [X] Edit Game Options text, that might enable it to work without patching
		Pointer(0x016A3BB0).Write((uint32_t)&c_gui_screen_pregame_lobby__ButtonPressHook);
		Pointer(0x016A3D68).Write((uint32_t)&c_gui_screen_pregame_lobby__ButtonPressHook);
		Pointer(0x016A3F80).Write((uint32_t)&c_gui_screen_pregame_lobby__ButtonPressHook);
		Pointer(0x016A40F0).Write((uint32_t)&c_gui_screen_pregame_lobby__ButtonPressHook);
		Pointer(0x016A7080).Write((uint32_t)&c_gui_screen_pregame_lobby__ButtonPressHook);
	}

	void PatchModuleUI::Tick(const std::chrono::duration<double>& deltaTime)
	{
		if (DialogShow)
		{
			if (DialogStringId == 0x10084) // start_menu
			{
				// seems like this func is meant to be called which returns the first arg to give AE0BE0, but it seems to only return 0?
				/*
				int test = 0;
				typedef bool(__cdecl* sub_A933D0Ptr)(int a1, int a2, int a3, int a4);
				auto sub_A933D0 = reinterpret_cast<sub_A933D0Ptr>(0xA933D0);

				sub_A933D0(0xFFFFFFFF, 0xFF, 0xFF, (int)&test);
				*/

				// this func was part of a larger func in H3E, but in HO the func was removed and only this part remains, which loads up the start_menu
				// in H3E the call to this larger func is at 0x8211D334, in HO this call is at 0xAAC8A3
				// the func in HO does some of what the H3E func does, but it call other funcs instead of this func we're calling
				// the H3E equivalent to this func we call is at 0x825D5C7C
				typedef bool(__cdecl* sub_AE0BE0Ptr)(int a1, int a2, int a3, int a4, void* a5, int a6);
				auto sub_AE0BE0 = reinterpret_cast<sub_AE0BE0Ptr>(0xAE0BE0);
				sub_AE0BE0(0, 0, 0, 0, 0, 0);
				DialogShow = false;
				return;
			}

			if (!UIData) // the game can also free this mem at any time afaik, but it also looks like it resets this ptr to 0, so we can just alloc it again
			{
				typedef void*(__cdecl * UI_AllocPtr)(int size);
				auto UI_Alloc = reinterpret_cast<UI_AllocPtr>(0xAB4ED0);
				UIData = UI_Alloc(0x40);
			}

			// fill UIData with proper data
			typedef void*(__thiscall * UI_OpenDialogByIdPtr)(void* a1, unsigned int dialogStringId, int a3, int dialogFlags, unsigned int parentDialogStringId);
			auto UI_OpenDialogById = reinterpret_cast<UI_OpenDialogByIdPtr>(0xA92780);
			UI_OpenDialogById(UIData, DialogStringId, DialogArg1, DialogFlags, DialogParentStringId);

			// post UI message
			typedef int(*UI_PostMessagePtr)(void* uiDataStruct);
			auto UI_PostMessage = reinterpret_cast<UI_PostMessagePtr>(0xA93450);
			UI_PostMessage(UIData);

			//alternate way of posting UI message
			/*uint32_t eax = (uint32_t)UIData;
			uint32_t ecx = *(uint32_t*)0x5260254;
			*(DWORD*)(ecx + 8) = eax;

			eax = *(uint32_t*)0x5260254;
			eax = *(uint32_t*)eax;
			*(uint32_t*)0x5260254 = eax;*/

			//*(uint32_t*)((char*)UIData + 0x18) = 1;

			DialogShow = false;
		}
	}
}
