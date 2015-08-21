#pragma once
#include <map>
#include <Windows.h>
#include <memory>

#include "CommandManager.hpp"
#include "PatchManager.hpp"
#include "Engine.hpp"
#include "DebugLog.hpp"
#include "Utils.hpp"
#include "Utils/Utils.hpp"

#include "Network/PlayerPropertiesExtension.hpp"

#include <ElDorito/Pointer.hpp>
#include <ElDorito/Blam/ArrayGlobal.hpp>

class IPatchProvider;

#include "Camera/CameraCommandProvider.hpp"
#include "Debug/DebugCommandProvider.hpp"
#include "Forge/ForgeCommandProvider.hpp"
#include "Game/GameCommandProvider.hpp"
#include "Input/InputCommandProvider.hpp"
#include "Player/PlayerCommandProvider.hpp"
#include "Server/ServerCommandProvider.hpp"
#include "Time/TimeCommandProvider.hpp"
#include "UI/UICommandProvider.hpp"

class ElDorito
{
private:
	bool inited = false;
	std::map<std::string, HMODULE> plugins;

	void loadPlugins();

	void initClasses();
	void initPatchProviders();
	void initCommandProvider(std::shared_ptr<ICommandProvider> command);

public:
	DebugLog Logger;
	PatchManager PatchManager;
	CommandManager CommandManager;
	PublicUtils Utils;
	Engine Engine;
	Network::PlayerPropertiesExtender PlayerPropertiesExtender;

	std::vector<std::shared_ptr<IPatchProvider>> Patches;

	std::shared_ptr<Camera::CameraCommandProvider> CameraCommands;
	std::shared_ptr<Debug::DebugCommandProvider> DebugCommands;
	std::shared_ptr<Forge::ForgeCommandProvider> ForgeCommands;
	std::shared_ptr<Game::GameCommandProvider> GameCommands;
	std::shared_ptr<Input::InputCommandProvider> InputCommands;
	std::shared_ptr<Player::PlayerCommandProvider> PlayerCommands;
	std::shared_ptr<Server::ServerCommandProvider> ServerCommands;
	std::shared_ptr<Time::TimeCommandProvider> TimeCommands;
	std::shared_ptr<UI::UICommandProvider> UICommands;

	void Initialize();

	static ElDorito& Instance()
	{
		static ElDorito inst;
		return inst;
	}

protected:
	ElDorito(); // Prevent construction
	ElDorito(const ElDorito&); // Prevent construction by copying
	ElDorito& operator=(const ElDorito&); // Prevent assignment
	~ElDorito(); // Prevent unwanted destruction
};
