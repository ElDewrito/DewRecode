#pragma once
#include <map>
#include <Windows.h>

#include "GameConsole.hpp"
#include "PatchManager.hpp"
#include "Engine.hpp"
#include "DebugLog.hpp"
#include "Modules/ModuleMain.hpp"

class ElDorito
{
public:
	GameConsole Console;
	PatchManager Patches;
	Engine Engine;
	DebugLog Logger;
	Modules::ModuleMain Modules;

	void* CreateInterface(std::string name, int *returnCode);
	void Initialize();

	static ElDorito& Instance()
	{
		static ElDorito inst;
		return inst;
	}

private:
	bool inited = false;
	std::map<std::string, HMODULE> plugins;

	void loadPlugins();

protected:
	ElDorito(); // Prevent construction
	ElDorito(const ElDorito&); // Prevent construction by copying
	ElDorito& operator=(const ElDorito&); // Prevent assignment
	~ElDorito(); // Prevent unwanted destruction
};
