#pragma once
#include <map>
#include <Windows.h>

#include "GameConsole.hpp"
#include "PatchManager.hpp"
#include "Engine.hpp"
#include "DebugLog.hpp"
#include "Modules/ModuleMain.hpp"
#include "Utils.hpp"

#include "Utils/Utils.hpp"
#include <ElDorito/Pointer.hpp>

class ElDorito
{
public:
	DebugLog Logger;
	PatchManager Patches;
	GameConsole Console;
	PublicUtils Utils;
	Engine Engine;
	Modules::ModuleMain Modules;

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
