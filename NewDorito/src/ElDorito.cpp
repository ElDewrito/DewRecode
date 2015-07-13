#include "ElDorito.hpp"
#include <iostream>
#include <filesystem>

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

	*returnCode = 1;
	return 0;
}

void ElDorito::Initialize()
{
	if (this->inited)
		return;

	std::cout << "Inited!" << std::endl;

	loadPlugins();

	std::string ret = Console.ExecuteCommand("Example.Test");

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