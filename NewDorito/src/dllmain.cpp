// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include "ElDorito.hpp"

DWORD gameThreadID = 0;
HMODULE doritoModuleHandle = 0;

bool InitInstance(HINSTANCE module)
{
	DisableThreadLibraryCalls(module);

	gameThreadID = GetCurrentThreadId();
	doritoModuleHandle = module;

	auto& dorito = ElDorito::Instance();

	dorito.Initialize();

	return true;
}

bool ExitInstance()
{
	return true;
}

// for compatibility with older-ED patched exes
DORITO_API int GetAdaptersInfo()
{
	return 1337;
}

DORITO_API DWORD GetGameThreadID()
{
	return gameThreadID;
}

DORITO_API HMODULE GetDoritoModuleHandle()
{
	return doritoModuleHandle;
}

DORITO_API int GetDoritoVersion()
{
	return Utils::Version::GetVersionInt();
}

DORITO_API void* CreateInterface(const char *name, int *returnCode)
{
	return ElDorito::Instance().Engine.CreateInterface(name, returnCode);
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH: return InitInstance(module);
		case DLL_PROCESS_DETACH: return ExitInstance();
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			return true;
	}

	return false;
}
