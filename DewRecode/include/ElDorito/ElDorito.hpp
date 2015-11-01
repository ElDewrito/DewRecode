/* This file is the main part of the ElDewrito* SDK. Including this header into your project will also include all the other SDK interfaces in the. */

#pragma once

#include <Windows.h>
#include <memory>
#include <vector>

#ifdef DORITO_EXPORTS
#define DORITO_API extern "C" __declspec(dllexport)
#define DORITO_CPP_API __declspec(dllexport)
#else
#define DORITO_API extern "C" __declspec(dllimport)
#define DORITO_CPP_API __declspec(dllimport)
#endif

struct ElDoritoPluginInfo
{
	char* Name;
	char* Author;
	char* Description;
	int Version;
	char* FriendlyVersion;
};

class CommandProvider;
class PatchProvider;

/* exports from plugin dlls */
typedef ElDoritoPluginInfo*(__cdecl *GetPluginInfoPtr)();
typedef bool(__cdecl *InitializePluginPtr)(std::vector<std::shared_ptr<CommandProvider>>* commandProviders, std::vector<std::shared_ptr<PatchProvider>>* patchProviders);

/* exports from eldewrito */
DORITO_API DWORD GetGameThreadID();
DORITO_API HMODULE GetDoritoModuleHandle();
DORITO_API int GetDoritoVersion();
DORITO_API void* CreateInterface(const char* name, int* returnCode);

#include "Blam/BlamTypes.hpp"
#include "Blam/BlamInput.hpp"
#include "Blam/Tags/GameEngineSettingsDefinition.hpp"
#include "InputContext.hpp"
#include "CustomPackets.hpp"
#include "CommandContext.hpp"
#include "ICommandManager.hpp"
#include "IDebugLog.hpp"
#include "IEngine.hpp"
#include "IPatchManager.hpp"
#include "IUtils.hpp"
#include "UI/IUserInterface.hpp"
#include "UI/UIWindow.hpp"
#include "CommandProvider.hpp"
#include "PatchProvider.hpp"
#include "ServerChat.hpp"