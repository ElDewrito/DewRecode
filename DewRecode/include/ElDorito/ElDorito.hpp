/* This file is the main part of the ElDewrito* SDK. Including this header into your project will also include all the other SDK interfaces in the. */

#pragma once

#include <Windows.h>
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

/* exports from plugin dlls */
typedef ElDoritoPluginInfo*(__cdecl* GetPluginInfoPtr)();
typedef bool(__cdecl* InitializePluginPtr)();

/* exports from eldewrito */
DORITO_API DWORD GetGameThreadID();
DORITO_API HMODULE GetDoritoModuleHandle();
DORITO_API int GetDoritoVersion();
DORITO_API void* CreateInterface(const char *name, int *returnCode);

#include "Blam/BlamTypes.hpp"
#include "Blam/Tags/GameEngineSettingsDefinition.hpp"
#include "ICommands.hpp"
#include "IDebugLog.hpp"
#include "IEngine.hpp"
#include "IPatchManager.hpp"
#include "IUtils.hpp"
#include "ModuleBase.hpp"
