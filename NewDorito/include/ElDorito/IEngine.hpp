#pragma once
#include "Pointer.hpp"
#include <chrono>

/* 
List of events registered by ED (eventModule/eventName seperated by a period):
	Core.Engine.FirstTick - signalled when the game engine loop starts ticking, only signals once
	Core.Engine.MainMenuShown - when the mainmenu is first being shown, only signals once after the game has inited etc
	Core.Engine.TagsLoaded - when the tags have been reloaded
	Core.Input.KeyboardUpdate - when a key is pressed (i think? haven't looked into keyboard code much)

(soon):
    Core.Direct3D.Present - when the game is about to call D3DDevice::Present
	Core.Direct3D.EndScene - when the game is about to call D3DDevice::EndScene
	Core.Round.Start - when a round has started
	Core.Round.End - when a round has finished
	Core.Game.Join - when the user joins a game
	Core.Game.Leave - when the user leaves a game (ez)
	Core.Game.Start - when a game has started
	Core.Game.End - when a game has finished (ez)
	Core.Server.Start - when the user has started a server (ez)
	Core.Server.Stop - when the user has stopped the server (ez)
	Core.Player.Join - when a user joins the game (signals for all users, not just host)
	Core.Player.Leave - when a user leaves the game (signals for all users, not just host) (ez)
	Core.Player.Kick - when a user has been kicked (host only) (ez)
	Fore.Twenty - when the kush hits you

later:
	Core.Lobby.ChangeMap
	Core.Lobby.ChangeGameMode
	Core.Medals.DoubleKill
	Core.Medals.TripleKill
	Core.Medals.Overkill
	(etc)
*/

typedef void(__cdecl* TickCallbackFunc)(const std::chrono::duration<double>& deltaTime);
typedef void(__cdecl* EventCallbackFunc)(void* param);

/*
if you want to make changes to this interface create a new IEngine002 class and make them there, then edit Engine class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IEngine001
{
public:
	// registers a callback which is called when the game ticks
	virtual bool OnTick(TickCallbackFunc callback) = 0;

	// you can use any eventModule/eventName here, the callback will belong to this combination
	// and calling Engine::Event with the same eventModule/eventName will call each of the registered callbacks for this event
	// (in essense this not only registers callbacks for events but also registers events too)
	// the only restricted eventModule is "Core", this module is reserved for events created by ElDorito
	// in case your wondering, eventModule and eventName are seperate so that plugin authors have to provide a module name for their event, making it "unique"
	virtual bool OnEvent(std::string eventModule, std::string eventName, EventCallbackFunc callback) = 0;

	// called when an event occurs, calls each registered callback for the event
	virtual void Event(std::string eventModule, std::string eventName, void* param = 0) = 0;

	// registers an interface, plugins can use this to share classes across plugins
	virtual bool RegisterInterface(std::string interfaceName, void* ptrToInterface) = 0;

	/// <summary>
	/// Creates an interface to an exported class.
	/// </summary>
	/// <param name="name">The name of the interface.</param>
	/// <param name="returnCode">0 if the interface was found successfully, otherwise the error code.</param>
	/// <returns>The requested interface, if found.</returns>
	virtual void* CreateInterface(std::string interfaceName, int* returnCode) = 0;

	virtual bool HasMainMenuShown() = 0;

	virtual HWND GetGameHWND() = 0;
	virtual Pointer GetMainTls(size_t offset = 0) = 0;
	virtual size_t GetMainThreadID() = 0;
	virtual void SetMainThreadID(size_t threadID) = 0;

	virtual HMODULE GetDoritoModule() = 0;
	virtual void SetDoritoModule(HMODULE module) = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"
