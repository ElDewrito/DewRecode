#include "ElDorito.hpp"
#include <iostream>
#include <filesystem>
#include <codecvt>
#include <cvt/wstring>

#include "Patches/ArmorPatchProvider.hpp"
#include "Patches/ContentItemsPatchProvider.hpp"
#include "Patches/CorePatchProvider.hpp"
#include "Patches/ForgePatchProvider.hpp"
#include "Patches/InputPatchProvider.hpp"
#include "Patches/NetworkPatchProvider.hpp"
#include "Patches/PlayerPatchProvider.hpp"
#include "Patches/ScoreboardPatchProvider.hpp"
#include "Patches/UIPatchProvider.hpp"
#include "Patches/VirtualKeyboardPatchProvider.hpp"

#include "Packets/ServerChat.hpp"

ElDorito::ElDorito()
{

}

ElDorito::~ElDorito()
{

}

void ElDorito::initPatchProviders()
{
	for (auto patch : Patches)
	{
		auto patchSet = patch->GetPatches();
		if (patchSet.Name != "null")
			PatchManager.TogglePatchSet(PatchManager.Add(patchSet));

		patch->RegisterCallbacks(&Engine);
	}
}

void ElDorito::initCommandProvider(std::shared_ptr<CommandProvider> command)
{
	auto cmds = command->GetCommands();
	for (auto cmd : cmds)
		CommandManager.Add(cmd);

	command->RegisterVariables(&CommandManager);
	command->RegisterCallbacks(&Engine);
}

void ElDorito::initClasses()
{
	// GameCommands has to be inited first since DebugLog uses it
	auto debugPatchProvider = std::make_shared<Debug::DebugPatchProvider>();
	Patches.push_back(debugPatchProvider);

	DebugCommands = std::make_shared<Debug::DebugCommandProvider>(debugPatchProvider);
	initCommandProvider(DebugCommands);

	auto armorPatchProvider = std::make_shared<Armor::ArmorPatchProvider>();
	auto cameraPatchProvider = std::make_shared<Camera::CameraPatchProvider>();
	auto contentItemPatchProvider = std::make_shared<ContentItems::ContentItemsPatchProvider>();
	auto corePatchProvider = std::make_shared<Core::CorePatchProvider>();
	auto forgePatchProvider = std::make_shared<Forge::ForgePatchProvider>();
	auto gameRulesPatchProvider = std::make_shared<GameRules::GameRulesPatchProvider>();
	InputPatches = std::make_shared<Input::InputPatchProvider>();
	NetworkPatches = std::make_shared<Network::NetworkPatchProvider>();
	auto playerPatchProvider = std::make_shared<Player::PlayerPatchProvider>();
	auto scoreboardPatchProvider = std::make_shared<Scoreboard::ScoreboardPatchProvider>();
	auto UIPatchProvider = std::make_shared<UI::UIPatchProvider>();
	auto vkPatchProvider = std::make_shared<VirtualKeyboard::VirtualKeyboardPatchProvider>();

	Patches.push_back(armorPatchProvider);
	Patches.push_back(cameraPatchProvider);
	Patches.push_back(contentItemPatchProvider);
	Patches.push_back(corePatchProvider);
	Patches.push_back(forgePatchProvider);
	Patches.push_back(gameRulesPatchProvider);
	Patches.push_back(InputPatches);
	Patches.push_back(NetworkPatches);
	Patches.push_back(playerPatchProvider);
	Patches.push_back(scoreboardPatchProvider);
	Patches.push_back(UIPatchProvider);
	Patches.push_back(vkPatchProvider);

	CameraCommands = std::make_shared<Camera::CameraCommandProvider>(cameraPatchProvider);
	initCommandProvider(CameraCommands);

	ElDewritoCommands = std::make_shared<ElDewrito::ElDewritoCommandProvider>();
	initCommandProvider(ElDewritoCommands);

	ForgeCommands = std::make_shared<Forge::ForgeCommandProvider>(forgePatchProvider);
	initCommandProvider(ForgeCommands);

	GameCommands = std::make_shared<Game::GameCommandProvider>();
	initCommandProvider(GameCommands);

	GameRulesCommands = std::make_shared<GameRules::GameRulesCommandProvider>(gameRulesPatchProvider);
	initCommandProvider(GameRulesCommands);

	InputCommands = std::make_shared<Input::InputCommandProvider>(InputPatches);
	initCommandProvider(InputCommands);

	IRCCommands = std::make_shared<IRC::IRCCommandProvider>();
	initCommandProvider(IRCCommands);

	PlayerCommands = std::make_shared<Player::PlayerCommandProvider>(armorPatchProvider, playerPatchProvider);
	initCommandProvider(PlayerCommands);

	ServerCommands = std::make_shared<Server::ServerCommandProvider>();
	initCommandProvider(ServerCommands);

	UICommands = std::make_shared<UI::UICommandProvider>(UIPatchProvider);
	initCommandProvider(UICommands);

	UpdaterCommands = std::make_shared<Updater::UpdaterCommandProvider>();
	initCommandProvider(UpdaterCommands);

	//UserInterface = std::make_shared<UI::UserInterface>(&Engine);

	initPatchProviders();

}

/// <summary>
/// Initializes this instance.
/// </summary>
void ElDorito::Initialize()
{
	if (this->inited)
		return;

	initClasses();

	Logger.Log(LogSeverity::Info, "ElDorito", "ElDewrito | Version: " + Utils::Version::GetVersionString() + " | Build Date: " __DATE__);
	loadPlugins();

	Logger.Log(LogSeverity::Debug, "ElDorito", "Console.FinishAddCommands()...");
	CommandManager.FinishAdd(); // call this so that the default values can be applied to the game

	Server::Chat::Initialize();
	
	Logger.Log(LogSeverity::Debug, "ElDorito", "Executing dewrito_prefs.cfg...");
	ElDewritoCommands->Execute("dewrito_prefs.cfg", CommandManager.NullContext);

	Logger.Log(LogSeverity::Debug, "ElDorito", "Executing autoexec.cfg...");
	ElDewritoCommands->Execute("autoexec.cfg", CommandManager.NullContext); // also execute autoexec, which is a user-made cfg guaranteed not to be overwritten by ElDew/launcher

	// HACKY - As we need a draw call in between this message and the actual generation, check it ahead of it actually generating
//	if (PlayerCommands->VarPubKey->ValueString.empty())
//		Engine.PrintToConsole("Generating player keypair\nThis may take a moment...");

	// add and toggle(enable) the language patch, can't be done in a module since we have to patch this after cfg files are read
	PatchManager.TogglePatch(PatchManager.AddPatch("GameLanguage", 0x6333FD, { (unsigned char)GameCommands->VarLanguageID->ValueInt }));

	Logger.Log(LogSeverity::Debug, "ElDorito", "Parsing command line...");

	int numArgs = 0;
	LPWSTR* szArgList = CommandLineToArgvW(GetCommandLineW(), &numArgs);
	bool skipKill = false;
	bool dedicated = false;

	if (szArgList && numArgs > 1)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

		for (int i = 1; i < numArgs; i++)
		{
			std::wstring arg = std::wstring(szArgList[i]);
			if (arg.compare(0, 1, L"-") != 0) // if it doesn't start with -
				continue;

			if (arg.compare(L"-dedicated") == 0)
			{
				dedicated = true;
			}

			if (arg.compare(L"-multiInstance") == 0)
			{
				skipKill = true;
			}

			size_t pos = arg.find(L"=");
			if (pos == std::wstring::npos || arg.length() <= pos + 1) // if it doesn't contain an =, or there's nothing after the =
				continue;

			std::string argname = converter.to_bytes(arg.substr(1, pos - 1));
			std::string argvalue = converter.to_bytes(arg.substr(pos + 1));

			CommandManager.Execute(argname + " \"" + argvalue + "\"", CommandManager.ConsoleContext);
		}
	}

	if (dedicated)
		NetworkPatches->SetDedicatedServerMode(true);

	Logger.Log(LogSeverity::Info, "ElDorito", "ElDewrito initialized!");
	this->inited = true;
}

/// <summary>
/// Loads and initializes plugins from the mods/plugins folder.
/// </summary>
void ElDorito::loadPlugins()
{
	auto pluginPath = std::tr2::sys::current_path<std::tr2::sys::path>();
	pluginPath /= "mods";
	pluginPath /= "plugins";
	Logger.Log(LogSeverity::Info, "ElDorito", "Loading plugins (plugin dir: %s)", pluginPath.string().c_str());

	for (std::tr2::sys::directory_iterator itr(pluginPath); itr != std::tr2::sys::directory_iterator(); ++itr)
	{
		if (std::tr2::sys::is_directory(itr->status()) || itr->path().extension() != ".dll")
			continue;

		auto& path = itr->path().string();
		this->Utils.ReplaceCharacters(path, '/', '\\');
		Logger.Log(LogSeverity::Debug, "Plugins", "Loading plugin from %s...", path.c_str());

		auto dllHandle = LoadLibraryA(path.c_str());
		if (!dllHandle)
		{
			Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: LoadLibrary failed (code: %d)", path.c_str(), GetLastError());
			continue;
		}

		auto GetPluginInfo = reinterpret_cast<GetPluginInfoPtr>(GetProcAddress(dllHandle, "GetPluginInfo"));
		auto InitializePlugin = reinterpret_cast<InitializePluginPtr>(GetProcAddress(dllHandle, "InitializePlugin"));

		if (!GetPluginInfo || !InitializePlugin)
		{
			Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Couldn't find exports!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		auto* info = GetPluginInfo();
		if (!info)
		{
			Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Plugin info invalid!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		Logger.Log(LogSeverity::Debug, "Plugins", "Initing \"%s\"", info->Name);
		if (!InitializePlugin())
		{
			Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Initialization failed!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		plugins.insert(std::pair<std::string, HMODULE>(path, dllHandle));
		Logger.Log(LogSeverity::Info, "Plugins", "Loaded \"%s\"", info->Name);
	}
}
