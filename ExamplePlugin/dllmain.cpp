#include <Windows.h>
#include <ElDorito/ElDorito.hpp>
#include "ExampleCommandProvider.hpp"

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

std::shared_ptr<Example::ExampleCommandProvider> exampleCmds;

PLUGIN_API ElDoritoPluginInfo* __cdecl GetPluginInfo()
{
	return &ourPluginInfo;
}

PLUGIN_API bool __cdecl InitializePlugin(std::vector<std::shared_ptr<CommandProvider>>* commandProviders, std::vector<std::shared_ptr<PatchProvider>>* patchProviders)
{
	int version = GetDoritoVersion();

	exampleCmds = std::make_shared<Example::ExampleCommandProvider>();
	commandProviders->push_back(exampleCmds);

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
