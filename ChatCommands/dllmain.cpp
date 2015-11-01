#include <Windows.h>
#include <ElDorito/ElDorito.hpp>
#include "RockTheVote.hpp"

#define PLUGIN_API extern "C" __declspec(dllexport)

ElDoritoPluginInfo ourPluginInfo =
{
	// plugin name
	"ChatCommands",
	// plugin author
	"emoose",
	// plugin description
	"A plugin which provides chat commands to the server",
	// plugin version/build number, should be incremented with every release of your plugin
	1,
	// a friendly version number string we can show to the user (no leading "v"/"version" text plz!)
	"0.1"
};

std::shared_ptr<ChatCommands::RockTheVote> cmdRockTheVote;

PLUGIN_API ElDoritoPluginInfo* __cdecl GetPluginInfo()
{
	return &ourPluginInfo;
}

PLUGIN_API bool __cdecl InitializePlugin(std::vector<std::shared_ptr<CommandProvider>>* commandProviders, std::vector<std::shared_ptr<PatchProvider>>* patchProviders)
{
	int version = GetDoritoVersion();

	cmdRockTheVote = std::make_shared<ChatCommands::RockTheVote>();
	int retCode = 0;
	IEngine* engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
	if (retCode != 0)
		throw std::runtime_error("Failed to create engine interface");

	engine->AddChatHandler(cmdRockTheVote);

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
