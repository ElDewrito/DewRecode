#include "ElDorito.hpp"
#include <iostream>
#include <filesystem>
#include <codecvt>
#include <cvt/wstring>

ElDorito::ElDorito()
{

}

ElDorito::~ElDorito()
{

}

/// <summary>
/// Initializes this instance.
/// </summary>
void ElDorito::Initialize()
{
	if (this->inited)
		return;

	Logger.Log(LogLevel::Info, "ElDorito", "ElDewrito | Version: " + Utils::Version::GetVersionString() + " | Build Date: " __DATE__);
	loadPlugins();

	Logger.Log(LogLevel::Debug, "ElDorito", "Console.FinishAddCommands()...");
	Console.FinishAddCommands(); // call this so that the default values can be applied to the game
	
	Logger.Log(LogLevel::Debug, "ElDorito", "Execute dewrito_prefs.cfg...");
	Console.ExecuteCommand("Execute dewrito_prefs.cfg");

	Logger.Log(LogLevel::Debug, "ElDorito", "Execute autoexec.cfg...");
	Console.ExecuteCommand("Execute autoexec.cfg"); // also execute autoexec, which is a user-made cfg guaranteed not to be overwritten by ElDew/launcher

	// add and toggle(enable) the language patch, can't be done in a module since we have to patch this after cfg files are read
	Patches.TogglePatch(Patches.AddPatch("GameLanguage", 0x6333FD, { (unsigned char)Modules.Game.VarLanguageID->ValueInt }));

	Logger.Log(LogLevel::Debug, "ElDorito", "Parsing command line...");
	// Parse command-line commands
	int numArgs = 0;
	LPWSTR* szArgList = CommandLineToArgvW(GetCommandLineW(), &numArgs);
	bool usingLauncher = Modules.Game.VarSkipLauncher->ValueInt == 1;
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

#ifndef _DEBUG
			if (arg.compare(L"-launcher") == 0)
				usingLauncher = true;
#endif

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

			Console.ExecuteCommand(argname + " \"" + argvalue + "\"", true);
		}
	}

	Logger.Log(LogLevel::Info, "ElDorito", "ElDewrito initialized!");
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
	Logger.Log(LogLevel::Info, "ElDorito", "Loading plugins (plugin dir: %s)", pluginPath.string().c_str());

	for (std::tr2::sys::directory_iterator itr(pluginPath); itr != std::tr2::sys::directory_iterator(); ++itr)
	{
		if (std::tr2::sys::is_directory(itr->status()) || itr->path().extension() != ".dll")
			continue;

		auto& path = itr->path().string();
		this->Utils.ReplaceCharacters(path, '/', '\\');
		Logger.Log(LogLevel::Debug, "Plugins", "Loading plugin from %s...", path.c_str());

		auto dllHandle = LoadLibraryA(path.c_str());
		if (!dllHandle)
		{
			Logger.Log(LogLevel::Error, "Plugins", "Failed to load plugin library %s: LoadLibrary failed (code: %d)", path.c_str(), GetLastError());
			continue;
		}

		auto GetPluginInfo = reinterpret_cast<GetPluginInfoPtr>(GetProcAddress(dllHandle, "GetPluginInfo"));
		auto InitializePlugin = reinterpret_cast<InitializePluginPtr>(GetProcAddress(dllHandle, "InitializePlugin"));

		if (!GetPluginInfo || !InitializePlugin)
		{
			Logger.Log(LogLevel::Error, "Plugins", "Failed to load plugin library %s: Couldn't find exports!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		auto* info = GetPluginInfo();
		if (!info)
		{
			Logger.Log(LogLevel::Error, "Plugins", "Failed to load plugin library %s: Plugin info invalid!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		Logger.Log(LogLevel::Debug, "Plugins", "Initing \"%s\"", info->Name);
		if (!InitializePlugin())
		{
			Logger.Log(LogLevel::Error, "Plugins", "Failed to load plugin library %s: Initialization failed!", path.c_str());
			FreeLibrary(dllHandle);
			continue;
		}

		plugins.insert(std::pair<std::string, HMODULE>(path, dllHandle));
		Logger.Log(LogLevel::Info, "Plugins", "Loaded \"%s\"", info->Name);
	}
}
