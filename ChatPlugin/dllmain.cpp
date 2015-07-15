#include <Windows.h>
#include <ElDorito/ElDorito.hpp>
#include "ModuleIRC.hpp"

#define PLUGIN_API extern "C" __declspec(dllexport)

ElDoritoPluginInfo ourPluginInfo =
{
	// plugin name
	"Chat Plugin",
	// plugin author
	"emoose",
	// plugin description
	"Handles global, in-game and VoIP chat",
	// plugin version/build number, should be incremented with every release of your plugin
	1,
	// a friendly version number string we can show to the user (no leading "v"/"version" text plz!)
	"0.1"
};

PLUGIN_API ElDoritoPluginInfo* __cdecl GetPluginInfo()
{
	return &ourPluginInfo;
}

PLUGIN_API bool __cdecl InitializePlugin()
{
	int version = GetEDVersion();

	int retCode = 0;
	IDebugLog001* logger = reinterpret_cast<IDebugLog001*>(CreateInterface(DEBUGLOG_INTERFACE_VERSION001, &retCode));
	if (retCode != 0)
		return false;

	logger->Log(LogLevel::Debug, "ChatPlugin", "Chat plugin initialized! (ED version: %d)", version);

	return true;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		return true;
	}

	return false;
}
