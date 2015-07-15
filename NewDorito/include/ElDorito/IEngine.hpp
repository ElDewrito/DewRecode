#pragma once
#include "Pointer.hpp"
#include <chrono>

/* 
List of events registered by ED (eventNamespace/eventName seperated by a period):
	Core.Engine.FirstTick - signalled when the game engine loop starts ticking, only signals once
	Core.Engine.MainMenuShown - when the mainmenu is first being shown, only signals once after the game has inited etc
	Core.Engine.TagsLoaded - when the tags have been reloaded
	Core.Input.KeyboardUpdate - when a key is pressed (i think? haven't looked into keyboard code much)


	Core.Server.Start - when the user has started a server
	Core.Server.Stop - when the user has stopped the server
	Core.Server.PlayerKick - when a user has been kicked (host only)

	Core.Game.Joining - when the user is joining a game (direct connect cmd issued)
	Core.Game.Leave - when the user leaves a game
	Core.Game.End - when a game has finished (ez)


(soon):
    Core.Direct3D.Present - when the game is about to call D3DDevice::Present
	Core.Direct3D.EndScene - when the game is about to call D3DDevice::EndScene
	Core.Round.Start - when a round has started
	Core.Round.End - when a round has finished
	Core.Game.Join - when the user joins a game
	Core.Game.Start - when a game has started
	Core.Player.Join - when a user joins the game (signals for all users, not just host)
	Core.Player.Leave - when a user leaves the game (signals for all users, not just host) (ez)
	Fore.Twenty - when the kush hits you

later:
	Core.Lobby.ChangeMap
	Core.Lobby.ChangeGameMode
	Core.Medals.DoubleKill
	Core.Medals.TripleKill
	Core.Medals.Overkill
	(etc)
*/

typedef void(__cdecl* TickCallback)(const std::chrono::duration<double>& deltaTime);
typedef void(__cdecl* EventCallback)(void* param);

struct PlayerKickInfo
{
	std::string Name;
	uint64_t UID;
};

struct ConsoleBuffer
{
	std::string Name;
	std::string Group;
	std::vector<std::string> Messages;
	std::vector<std::string> InputHistory;
	int InputHistoryIndex = -1;
	unsigned int ScrollIndex;
	int MaxDisplayLines;
	EventCallback InputEventCallback;
	bool Visible;
	bool Focused;

	ConsoleBuffer(std::string name, std::string group, EventCallback inputEventCallback, bool visible = false)
	{
		Name = name;
		Group = group;
		InputEventCallback = inputEventCallback;
		Visible = visible;
		Focused = false;
		ScrollIndex = 0;
		MaxDisplayLines = 10;
	}
};
/*
if you want to make changes to this interface create a new IEngine002 class and make them there, then edit Engine class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IEngine001
{
public:
	/// <summary>
	/// Registers a callback which is called when the game ticks, tick callbacks use a seperate path to normal events so we can process them in less cycles.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was added, false if the callback is already registered.</returns>
	virtual bool OnTick(TickCallback callback) = 0;

	/// <summary>
	/// Registers a callback which is called when a WM message is received (registers another WNDPROC)
	/// If the callback returns 1 then the games WNDPROC won't be called.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was added, false if the callback is already registered.</returns>
	virtual bool OnWndProc(WNDPROC callback) = 0;

	/// <summary>
	/// Adds a callback to be called when the specified event occurs. If the eventNamespace/eventName combination doesn't exist a new event will be created.
	/// Note that the "Core" namespace is restricted to events created by ElDorito.
	/// </summary>
	/// <param name="eventNamespace">The namespace the event belongs to.</param>
	/// <param name="eventName">The name of the event.</param>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was added, false if the callback is already registered.</returns>
	virtual bool OnEvent(std::string eventNamespace, std::string eventName, EventCallback callback) = 0;

	/// <summary>
	/// Calls each of the registered callbacks for the specified event.
	/// </summary>
	/// <param name="eventNamespace">The namespace the event belongs to.</param>
	/// <param name="eventName">The name of the event.</param>
	/// <param name="param">The parameter to pass to the callbacks.</param>
	virtual void Event(std::string eventNamespace, std::string eventName, void* param = 0) = 0;

	/// <summary>
	/// Registers an interface, plugins can use this to share classes across plugins.
	/// </summary>
	/// <param name="interfaceName">Name of the interface.</param>
	/// <param name="ptrToInterface">Pointer to an instance of the interface.</param>
	/// <returns>true if the interface was registered, false if an interface already exists with this name</returns>
	virtual bool RegisterInterface(std::string interfaceName, void* ptrToInterface) = 0;

	/// <summary>
	/// Gets an instance of the specified interface, if its been registered.
	/// </summary>
	/// <param name="interfaceName">Name of the interface.</param>
	/// <param name="returnCode">Returns 0 if the interface was found, otherwise 1 if it couldn't.</param>
	/// <returns>A pointer to the requested interface.</returns>
	virtual void* CreateInterface(std::string interfaceName, int* returnCode) = 0;

	/// <summary>
	/// Adds a new buffer/queue to the console UI.
	/// </summary>
	/// <param name="buffer">The buffer to add.</param>
	/// <returns>A pointer to the added buffer.</returns>
	virtual ConsoleBuffer* AddConsoleBuffer(ConsoleBuffer buffer) = 0;

	/// <summary>
	/// Returns true if the main menu has been shown, signifying that the game has initialized.
	/// </summary>
	/// <returns>true if the main menu has been shown.</returns>
	virtual bool HasMainMenuShown() = 0;

	/// <summary>
	/// Returns the HWND for the game window.
	/// </summary>
	/// <returns>The HWND for the game window.</returns>
	virtual HWND GetGameHWND() = 0;

	/// <summary>
	/// Gets the main TLS address (optionally with an offset added)
	/// </summary>
	/// <param name="offset">The offset to add to the TLS address.</param>
	/// <returns>A Pointer to the TLS.</returns>
	virtual Pointer GetMainTls(size_t offset = 0) = 0;

	/// <summary>
	/// Gets the main thread ID for the game.
	/// </summary>
	/// <returns>The main thread ID for the game.</returns>
	virtual size_t GetMainThreadID() = 0;

	/// <summary>
	/// Sets the main thread ID for the game.
	/// </summary>
	virtual void SetMainThreadID(size_t threadID) = 0;

	/// <summary>
	/// Gets the module handle for the ElDorito dll.
	/// </summary>
	/// <returns>The module handle for the ElDorito dll.</returns>
	virtual HMODULE GetDoritoModule() = 0;

	/// <summary>
	/// Sets the module handle for the ElDorito dll.
	/// </summary>
	virtual void SetDoritoModule(HMODULE module) = 0;

	/// <summary>
	/// Gets the ElDorito dll version as a string.
	/// </summary>
	/// <returns>The ElDorito dll version as a string.</returns>
	virtual std::string GetDoritoVersionString() = 0;

	/// <summary>
	/// Gets the ElDorito dll version as an integer.
	/// </summary>
	/// <returns>The ElDorito dll version as an integer.</returns>
	virtual DWORD GetDoritoVersionInt() = 0;

	/// <summary>
	/// Gets the games resolution.
	/// </summary>
	/// <returns>The resolution.</returns>
	virtual std::pair<int, int> GetGameResolution() = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"
