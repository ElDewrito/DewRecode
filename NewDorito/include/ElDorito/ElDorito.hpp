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

/* exports from eldorito */
DORITO_API DWORD GetGameThreadID();
DORITO_API HMODULE GetDoritoModuleHandle();
DORITO_API int GetDoritoVersion();
DORITO_API void* CreateInterface(const char *name, int *returnCode);

#include "ICommands.hpp"
#include "IDebugLog.hpp"
#include "IEngine.hpp"
#include "IPatchManager.hpp"
#include "IUtils.hpp"
#include "ModuleBase.hpp"
