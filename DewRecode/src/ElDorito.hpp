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

#include "Patches/PlayerPropertiesExtension.hpp"
#include "Patches/NetworkPatchProvider.hpp"

#include <ElDorito/Pointer.hpp>
#include <ElDorito/Blam/ArrayGlobal.hpp>

class PatchProvider;

#include "Commands/CameraCommandProvider.hpp"
#include "Commands/DebugCommandProvider.hpp"
#include "Commands/ElDewritoCommandProvider.hpp"
#include "Commands/ForgeCommandProvider.hpp"
#include "Commands/GameCommandProvider.hpp"
#include "Commands/GameRulesCommandProvider.hpp"
#include "Commands/GraphicsCommandProvider.hpp"
#include "Commands/InputCommandProvider.hpp"
#include "Commands/IRCCommandProvider.hpp"
#include "Commands/PlayerCommandProvider.hpp"
#include "Commands/ServerCommandProvider.hpp"
#include "Commands/UICommandProvider.hpp"
#include "Commands/UpdaterCommandProvider.hpp"

#include "UI/UserInterface.hpp"
#include "Chat/ChatCommandManager.hpp"

#include "ModPacks/ModPackage.hpp"

class ElDorito
{
private:
	bool inited = false;

	void loadPlugins();
	void loadModPackages();

	void initClasses();
	void initPatchProviders();
	void initCommandProvider(std::shared_ptr<CommandProvider> command);

	void onTagsLoaded(void* param);

public:
	std::map<std::string, HMODULE> Plugins;
	std::vector<ModPackage*> ModPackages;
	std::vector<std::string> DisabledModIds;

	DebugLog Logger;
	PatchManager PatchManager;
	CommandManager CommandManager;
	PublicUtils Utils;
	Engine Engine;
	Network::PlayerPropertiesExtender PlayerPropertiesExtender;
	UI::UserInterface UserInterface;

	std::shared_ptr<Chat::ChatCommandManager> ChatCommandManager;

	std::vector<std::shared_ptr<PatchProvider>> Patches;
	std::shared_ptr<Input::InputPatchProvider> InputPatches;
	std::shared_ptr<Network::NetworkPatchProvider> NetworkPatches;

	std::shared_ptr<Camera::CameraCommandProvider> CameraCommands;
	std::shared_ptr<Debug::DebugCommandProvider> DebugCommands;
	std::shared_ptr<ElDewrito::ElDewritoCommandProvider> ElDewritoCommands;
	std::shared_ptr<Forge::ForgeCommandProvider> ForgeCommands;
	std::shared_ptr<Game::GameCommandProvider> GameCommands;
	std::shared_ptr<GameRules::GameRulesCommandProvider> GameRulesCommands;
	std::shared_ptr<Graphics::GraphicsCommandProvider> GraphicsCommands;
	std::shared_ptr<Input::InputCommandProvider> InputCommands;
	std::shared_ptr<IRC::IRCCommandProvider> IRCCommands;
	std::shared_ptr<Player::PlayerCommandProvider> PlayerCommands;
	std::shared_ptr<Server::ServerCommandProvider> ServerCommands;
	std::shared_ptr<UI::UICommandProvider> UICommands;
	std::shared_ptr<Updater::UpdaterCommandProvider> UpdaterCommands;

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
