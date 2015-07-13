#pragma once
#include "IConsole.hpp"
#include "IPatchManager.hpp"

#ifdef DORITO_EXPORTS
#define DORITO_API extern "C" __declspec(dllexport)
#else
#define DORITO_API extern "C" __declspec(dllimport)
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
typedef ElDoritoPluginInfo*(__cdecl* GetPluginInfoFunc)();
typedef bool(__cdecl* InitializePluginFunc)();

/* exports from eldorito */
DORITO_API int GetEDVersion();
DORITO_API void* CreateInterface(const char *name, int *returnCode);