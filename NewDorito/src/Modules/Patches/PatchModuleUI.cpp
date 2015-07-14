#include "PatchModuleUI.hpp"
#include "../../ElDorito.hpp"
#include <algorithm>

namespace
{
	void __fastcall UI_MenuUpdateHook(void* a1, int unused, int menuIdToLoad)
	{
		auto& engine = ElDorito::Instance().Engine;
		if (!engine.HasMainMenuShown() && menuIdToLoad == 0x10083)
			engine.MainMenuShown();

		bool shouldUpdate = *(DWORD*)((uint8_t*)a1 + 0x10) >= 0x1E;

		typedef void(__thiscall *UIMenuUpdateFunc)(void* a1, int menuIdToLoad);
		auto UIMenuUpdate = reinterpret_cast<UIMenuUpdateFunc>(0xADF6E0);
		UIMenuUpdate(a1, menuIdToLoad);

		if (shouldUpdate)
		{
			auto& uiPatches = ElDorito::Instance().Modules.UIPatches;
			uiPatches.DialogStringId = menuIdToLoad;
			uiPatches.DialogArg1 = 0xFF;
			uiPatches.DialogFlags = 4;
			uiPatches.DialogParentStringId = 0x1000D;
			uiPatches.DialogShow = true;
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

	__declspec(naked) void LobbyMenuButtonHandlerHook()
	{
		__asm
		{
			// call sub that handles showing game options
			mov ecx, esi
				push[edi + 0x10]
				mov eax, 0xB225B0
				call eax
				// jump back to original function
				mov eax, 0xB21B9F
				jmp eax
		}
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

	void UIPatches_ApplyMapNameFixes()
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
}

namespace Modules
{
	PatchModuleUI::PatchModuleUI() : ModuleBase("Patches.UI")
	{
		// register our tick callbacks
		engine->OnTick(UIPatches_TickCallback);
		engine->OnFirstTick(UIPatches_ApplyMapNameFixes);

		// add our patches
		patches->TogglePatchSet(patches->AddPatchSet("Patches.UI",
		{
			// Fix "Network" setting in lobbies (change broken 0x100B7 menuID to 0x100B6)
			Patch("NetworkFix", 0xAC34B0, { 0xB6 }),

			// Fix gamepad option in settings (todo: find out why it doesn't detect gamepads
			// and find a way to at least enable pressing ESC when gamepad is enabled)
			Patch("GamepadFix", 0x60D7F2, true, 2),

			// Hacky fix to stop the game crashing when you move selection on UI
			// (todo: find out what's really causing this)
			Patch("UICrashFix", 0x969D07, true, 3),

			// Remove Xbox Live from the network menu
			Patch("RemoveXBL1", 0xB23D85, true, 0x17),
			Patch("RemoveXBL2", 0xB23DA1, { 0 }),
			Patch("RemoveXBL3", 0xB23DB8, { 1 }),
			Patch("RemoveXBL4", 0xB23DFF, true, 3),
			Patch("RemoveXBL5", 0xB23E07, { 0 }),
			Patch("RemoveXBL6", 0xB23E1C, { 0 }),

			// Remove "BUILT IN" text when choosing map/game variants by feeding the UI_SetVisiblityOfElement func a nonexistant string ID for the element (0x202E8 instead of 0x102E8)
			// TODO: find way to fix this text instead of removing it, since the 0x102E8 element (subitem_edit) is used for other things like editing/viewing map variant metadata
			Patch("RemoveBuiltInText", 0xB05D6F, { 0x2 }),

			// Fix for leave game button to show H3 pause menu
			Patch("ShowH3PauseMenu1", 0x7B682B, true, 1),
		}, 
		{

			// Fix for leave game button to show H3 pause menu
			Hook("ShowH3PauseMenu2", 0x7B6826, UI_ShowHalo3PauseMenu, HookType::Call),

			// Fix menu update code to include missing mainmenu code
			Hook("MenuUpdate", 0xADFB73, UI_MenuUpdateHook, HookType::Call),

			// Sorta hacky way of getting game options screen to show when you press X on lobby
			// TODO: find real way of showing the [X] Edit Game Options text, that might enable it to work without patching
			Hook("GameOptions", 0xB21B8A, LobbyMenuButtonHandlerHook, HookType::JmpIfEqual),

			// Localized string override hook
			Hook("LocalizedString", 0x51E040, LocalizedStringHook, HookType::Jmp),

			// Hook window title sprintf to replace the dest buf with our string
			Hook("WindowTitle", 0x42EB84, WindowTitleSprintfHook, HookType::Call),
		}));

		// TODO: forge menu hook, pause menu hook
	}

	void PatchModuleUI::Tick(const std::chrono::duration<double>& deltaTime)
	{
		if (DialogShow)
		{
			if (!UIData) // the game can also free this mem at any time afaik, but it also looks like it resets this ptr to 0, so we can just alloc it again
			{
				typedef void*(__cdecl * UIAllocFunc)(int size);
				auto UIAlloc = reinterpret_cast<UIAllocFunc>(0xAB4ED0);
				UIData = UIAlloc(0x40);
			}

			// fill UIData with proper data
			typedef void*(__thiscall * OpenUIDialogByIdFunc)(void* a1, unsigned int dialogStringId, int a3, int dialogFlags, unsigned int parentDialogStringId);
			auto OpenUIDialogById = reinterpret_cast<OpenUIDialogByIdFunc>(0xA92780);
			OpenUIDialogById(&UIData, DialogStringId, DialogArg1, DialogFlags, DialogParentStringId);

			// send UI notification
			typedef int(*SendUINotificationFunc)(void* uiDataStruct);
			auto SendUINotification = reinterpret_cast<SendUINotificationFunc>(0xA93450);
			SendUINotification(&UIData);

			DialogShow = false;
		}
	}
}
