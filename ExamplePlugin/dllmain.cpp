#include <Windows.h>
#include "../NewDorito/include/ElDorito/ElDorito.hpp"
// TODO: add this to the additional include dirs so we can do
// #include <ElDorito/ElDorito.hpp>

#define PLUGIN_API extern "C" __declspec(dllexport)

ElDoritoPluginInfo ourPluginInfo =
{
	// plugin name
	"Example Plugin",
	// plugin author
	"emoose",
	// plugin description
	"A small plugin",
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

	IConsole001* console001 = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &version));
	console001->ExecuteCommand("Command001-plugin");

	IConsole002* console002 = reinterpret_cast<IConsole002*>(CreateInterface(CONSOLE_INTERFACE_VERSION002, &version));
	console002->ExecuteCommands("Command002-plugin");

	IPatchManager001* patches001 = reinterpret_cast<IPatchManager001*>(CreateInterface(PATCHMANAGER_INTERFACE_VERSION001, &version));

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

