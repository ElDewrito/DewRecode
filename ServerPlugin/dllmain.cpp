#include <Windows.h>
#include <ElDorito/ElDorito.hpp>
#include "PatchModuleServer.hpp"

#define PLUGIN_API extern "C" __declspec(dllexport)

ElDoritoPluginInfo ourPluginInfo =
{
	// plugin name
	"Info+RCON+WebSocket plugin",
	// plugin author
	"emoose",
	// plugin description
	"Provides an info server, RCON server and RCON/WebSocket server",
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
	int version = GetDoritoVersion();

	int retCode = 0;
	IDebugLog* logger = reinterpret_cast<IDebugLog*>(CreateInterface(DEBUGLOG_INTERFACE_LATEST, &retCode));
	if (retCode != 0)
		return false;

	logger->Log(LogSeverity::Debug, "ServerPlugin", "Server plugin initialized! (ED version: %d)", version);

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
