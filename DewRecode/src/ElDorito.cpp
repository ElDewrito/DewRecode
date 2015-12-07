#include "ElDorito.hpp"
#include <iostream>
#include <filesystem>
#include <codecvt>
#include <cvt/wstring>

#include "Patches/ArmorPatchProvider.hpp"
#include "Patches/ContentItemsPatchProvider.hpp"
#include "Patches/CorePatchProvider.hpp"
#include "Patches/ForgePatchProvider.hpp"
#include "Patches/GraphicsPatchProvider.hpp"
#include "Patches/InputPatchProvider.hpp"
#include "Patches/NetworkPatchProvider.hpp"
#include "Patches/PlayerPatchProvider.hpp"
#include "Patches/ScoreboardPatchProvider.hpp"
#include "Patches/UIPatchProvider.hpp"
#include "Patches/VirtualKeyboardPatchProvider.hpp"

#include "Chat/ServerChat.hpp"
#include "Chat/ChatCommandManager.hpp"

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
	auto graphicsPatchProvider = std::make_shared<Graphics::GraphicsPatchProvider>();
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
	Patches.push_back(graphicsPatchProvider);
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

	GraphicsCommands = std::make_shared<Graphics::GraphicsCommandProvider>(graphicsPatchProvider);
	initCommandProvider(GraphicsCommands);

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

	loadPlugins();
	loadModPackages();

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

	Logger.Log(LogSeverity::Debug, "ElDorito", "Console.FinishAddCommands()...");
	CommandManager.FinishAdd(); // call this so that the default values can be applied to the game

	ChatCommandManager = std::make_shared<Chat::ChatCommandManager>();
	Chat::Initialize();
	Chat::AddHandler(ChatCommandManager);

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
	bool nod3d = false;
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
				dedicated = true;

			if (arg.compare(L"-nod3d") == 0)
				nod3d = true;

			size_t pos = arg.find(L"=");
			if (pos == std::wstring::npos || arg.length() <= pos + 1) // if it doesn't contain an =, or there's nothing after the =
				continue;

			std::string argname = converter.to_bytes(arg.substr(1, pos - 1));
			std::string argvalue = converter.to_bytes(arg.substr(pos + 1));

			CommandManager.Execute(argname + " \"" + argvalue + "\"", CommandManager.ConsoleContext);
		}
	}
	std::string text = "ElDewrito initialized!";

	if (dedicated)
	{
		NetworkPatches->SetDedicatedServerMode(true);
		text += " (dedicated)";
	}

	if (nod3d)
	{
		NetworkPatches->SetD3DDisabled(true);
		text += " (d3d disabled)";
	}

	Engine.OnEvent("Core", EDEVENT_ENGINE_TAGSLOADED, BIND_CALLBACK(this, &ElDorito::onTagsLoaded));

	Logger.Log(LogSeverity::Info, "ElDorito", text);
	this->inited = true;
}

void ElDorito::onTagsLoaded(void* param)
{
	for (auto patch : Patches)
		patch->PatchTags(0x1337);

	for (auto mod : ModPackages)
		mod->onTagsLoaded();
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

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA((pluginPath.string() + "\\*.dll").c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			std::string fname = pluginPath.string() + "\\" + ffd.cFileName;
			if (fname.length() < 3 || fname.substr(fname.length() - 3, 3) != "dll")
				continue;

			this->Utils.ReplaceCharacters(fname, '/', '\\');
			Logger.Log(LogSeverity::Debug, "Plugins", "Loading plugin from %s...", fname.c_str());

			auto dllHandle = LoadLibraryA(fname.c_str());
			if (!dllHandle)
			{
				Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: LoadLibrary failed (code: %d)", fname.c_str(), GetLastError());
				continue;
			}

			auto GetPluginInfo = reinterpret_cast<GetPluginInfoPtr>(GetProcAddress(dllHandle, "GetPluginInfo"));
			auto InitializePlugin = reinterpret_cast<InitializePluginPtr>(GetProcAddress(dllHandle, "InitializePlugin"));

			if (!GetPluginInfo || !InitializePlugin)
			{
				Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Couldn't find exports!", fname.c_str());
				FreeLibrary(dllHandle);
				continue;
			}

			auto* info = GetPluginInfo();
			if (!info)
			{
				Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Plugin info invalid!", fname.c_str());
				FreeLibrary(dllHandle);
				continue;
			}

			Logger.Log(LogSeverity::Debug, "Plugins", "Initing \"%s\"", info->Name);
			std::vector<std::shared_ptr<CommandProvider>> cmdProviders;
			std::vector<std::shared_ptr<PatchProvider>> patchProviders;
			if (!InitializePlugin(&cmdProviders, &patchProviders))
			{
				Logger.Log(LogSeverity::Error, "Plugins", "Failed to load plugin library %s: Initialization failed!", fname.c_str());
				FreeLibrary(dllHandle);
				continue;
			}

			for (auto cmd : cmdProviders)
				initCommandProvider(cmd);

			for (auto patch : patchProviders)
				Patches.push_back(patch);

			Plugins.insert(std::pair<std::string, HMODULE>(fname, dllHandle));
			Logger.Log(LogSeverity::Info, "Plugins", "Loaded \"%s\" v%s by %s", info->Name, info->FriendlyVersion, info->Author);
		}
	}
	while (FindNextFileA(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		FindClose(hFind);
		return;
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}

/// <summary>
/// Loads and initializes mod packages from the mods/packages folder.
/// </summary>
void ElDorito::loadModPackages()
{
	auto packagePath = std::tr2::sys::current_path<std::tr2::sys::path>();
	packagePath /= "mods";
	packagePath /= "packages";
	Logger.Log(LogSeverity::Info, "ElDorito", "Loading mod packages (package dir: %s)", packagePath.string().c_str());

	WIN32_FIND_DATAA ffd;
	HANDLE hFind = FindFirstFileA((packagePath.string() + "\\*").c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0)
		{
			std::string fname = packagePath.string() + "\\" + ffd.cFileName;

			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				if (fname.length() < 3 || fname.substr(fname.length() - 3, 3) != "zip")
					continue;

			this->Utils.ReplaceCharacters(fname, '/', '\\');

			ModPackage* mod = nullptr;

			if (fname.find(".zip") != std::string::npos)
				mod = new ModPackage(new libzippp::ZipArchive(fname), std::string(ffd.cFileName));
			else
				mod = new ModPackage(fname);

			if (mod->load())
				ModPackages.push_back(mod);

			mod->Enabled = Engine.GetModEnabled(mod->ID);
		}
	} while (FindNextFileA(hFind, &ffd) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		FindClose(hFind);
		return;
	}

	FindClose(hFind);
	hFind = INVALID_HANDLE_VALUE;
}