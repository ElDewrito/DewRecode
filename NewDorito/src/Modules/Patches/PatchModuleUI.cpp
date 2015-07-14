#include "PatchModuleUI.hpp"
#include "../../ElDorito.hpp"

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

	// TODO: make this use a lambda func instead once VC supports converting lambdas to funcptrs
	bool UIPatches_TickCallback(const std::chrono::duration<double>& deltaTime)
	{
		return ElDorito::Instance().Modules.UIPatches.Tick(deltaTime);
	}
}

namespace Modules
{
	PatchModuleUI::PatchModuleUI() : ModuleBase("Patches.UI")
	{
		// register our tick callback
		engine->OnTick(UIPatches_TickCallback);

		// add our patches
		patches->TogglePatchSet(patches->AddPatchSet("Patches.UI", {}, 
		{
			{ "MenuUpdate", 0xADFB73, UI_MenuUpdateHook, HookType::Call, {}, false }
		}));
	}

	bool PatchModuleUI::Tick(const std::chrono::duration<double>& deltaTime)
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

		return true;
	}
}
