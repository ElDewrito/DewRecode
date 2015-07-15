#include <Windows.h>
#include <ElDorito/ElDorito.hpp>
#include "ModuleExample.hpp"

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

Modules::ModuleExample test;

PLUGIN_API ElDoritoPluginInfo* __cdecl GetPluginInfo()
{
	return &ourPluginInfo;
}

PLUGIN_API bool __cdecl InitializePlugin()
{
	int version = GetEDVersion();

	ICommands001* commands = reinterpret_cast<ICommands001*>(CreateInterface(COMMANDS_INTERFACE_VERSION001, &version));
	commands->Execute("Command001-plugin");

	IPatchManager001* patches001 = reinterpret_cast<IPatchManager001*>(CreateInterface(PATCHMANAGER_INTERFACE_VERSION001, &version));

	auto ret = commands->Execute("Help");
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
