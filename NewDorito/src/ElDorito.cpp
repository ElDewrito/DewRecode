#include "ElDorito.hpp"
#include <iostream>
#include <filesystem>
#include <codecvt>
#include <cvt/wstring> // wstring_convert

ElDorito::ElDorito()
{

}

ElDorito::~ElDorito()
{

}

void* ElDorito::CreateInterface(std::string name, int *returnCode)
{
	*returnCode = 0;

	if (!name.compare(CONSOLE_INTERFACE_VERSION001))
		return &this->Console;

	if (!name.compare(PATCHMANAGER_INTERFACE_VERSION001))
		return &this->Patches;

	if (!name.compare(ENGINE_INTERFACE_VERSION001))
		return &this->Engine;

	if (!name.compare(DEBUGLOG_INTERFACE_VERSION001))
		return &this->Logger;

	*returnCode = 1;
	return 0;
}

void ElDorito::Initialize()
{
	if (this->inited)
		return;

	loadPlugins();

	Console.FinishAddCommands(); // call this so that the default values can be applied to the game
	
	Console.ExecuteCommand("Execute dewrito_prefs.cfg");
	Console.ExecuteCommand("Execute autoexec.cfg"); // also execute autoexec, which is a user-made cfg guaranteed not to be overwritten by ElDew/launcher

	// add and toggle(enable) the language patch
	Patches.TogglePatch(Patches.AddPatch("GameLanguage", 0x6333FD, { (unsigned char)Modules.Game.VarLanguageID->ValueInt }));

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

	this->inited = true;
}

void ElDorito::loadPlugins()
{
	auto pluginPath = std::tr2::sys::current_path<std::tr2::sys::path>();
	pluginPath /= "mods";
	pluginPath /= "plugins";

	for (std::tr2::sys::directory_iterator itr(pluginPath); itr != std::tr2::sys::directory_iterator(); ++itr)
	{
		if (std::tr2::sys::is_directory(itr->status()) || itr->path().extension() != ".dll")
			continue;

		auto& path = itr->path().string();

		auto dllHandle = LoadLibraryA(path.c_str());
		if (!dllHandle)
			continue; // TODO: write to debug log when any of these continue statements are hit

		auto GetPluginInfo = reinterpret_cast<GetPluginInfoFunc>(GetProcAddress(dllHandle, "GetPluginInfo"));
		auto InitializePlugin = reinterpret_cast<InitializePluginFunc>(GetProcAddress(dllHandle, "InitializePlugin"));

		if (!GetPluginInfo || !InitializePlugin)
		{
			FreeLibrary(dllHandle);
			continue;
		}

		auto* info = GetPluginInfo();
		if (!info)
		{
			FreeLibrary(dllHandle);
			continue;
		}

		std::cout << "Initing plugin \"" << info->Name << "\"..." << std::endl;
		if (!InitializePlugin())
		{
			FreeLibrary(dllHandle);
			continue;
		}

		plugins.insert(std::pair<std::string, HMODULE>(path, dllHandle));
	}
}
