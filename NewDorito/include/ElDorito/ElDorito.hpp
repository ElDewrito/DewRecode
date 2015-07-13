#pragma once
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
typedef ElDoritoPluginInfo*(__cdecl* GetPluginInfoFunc)();
typedef bool(__cdecl* InitializePluginFunc)();

/* exports from eldorito */
DORITO_API int GetEDVersion();
DORITO_API void* CreateInterface(const char *name, int *returnCode);

#include "IConsole.hpp"
#include "IPatchManager.hpp"
#include "IEngine.hpp"
#include "ModuleBase.hpp"
