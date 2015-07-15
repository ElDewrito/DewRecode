// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
#include "ElDorito.hpp"

bool InitInstance(HINSTANCE module)
{
	DisableThreadLibraryCalls(module);

	auto& dorito = ElDorito::Instance();
	dorito.Engine.SetMainThreadID(GetCurrentThreadId());
	dorito.Engine.SetDoritoModule(module);

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

DORITO_API int GetEDVersion()
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
