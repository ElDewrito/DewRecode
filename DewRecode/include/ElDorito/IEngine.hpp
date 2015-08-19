#pragma once
#include "Pointer.hpp"
#include <chrono>
namespace Blam
{
	class ArrayGlobal;

	namespace Network
	{
		struct Session;
		struct PacketTable;
	}
}

/* 
List of events registered by ED (eventNamespace/eventName seperated by a period, parameter in parens):
	Core.Engine.FirstTick - signalled when the game engine loop starts ticking, only signals once
	Core.Engine.MainMenuShown - when the mainmenu is first being shown, only signals once after the game has inited etc
	Core.Engine.TagsLoaded - when the tags have been reloaded
	Core.Input.KeyboardUpdate - when a key is pressed (i think? haven't looked into keyboard code much)

	Core.Server.Start - when the user has started a server
	Core.Server.Stop - when the user has stopped the server
	Core.Server.PlayerKick(PlayerInfo) - when a user has been kicked (host only)

	Core.Game.Joining - when the user is joining a game (direct connect cmd issued)
	Core.Game.Leave - when the user leaves a game
	Core.Game.End - when a game has finished (ez)

	Core.Direct3D.EndScene - when the game is about to call D3DDevice::EndScene

	Core.Player.ChangeName - when the user successfully changes their name

(soon):
    Core.Direct3D.Present - when the game is about to call D3DDevice::Present
	Core.Round.Start - when a round has started
	Core.Round.End - when a round has finished
	Core.Game.Join - when the user has joined a game successfully
	Core.Game.Start - when a game has started
	Core.Player.Join(PlayerInfo) - when a user joins the game (signals for all users, not just host)
	Core.Player.Leave(PlayerInfo) - when a user leaves the game (signals for all users, not just host) (ez)
	Fore.Twenty - when the kush hits you

later:
	Core.Lobby.ChangeMap
	Core.Lobby.ChangeGameMode
	Core.Game.Event(EventID) - stuff like double kills etc, might include normal kills too
*/

struct ConsoleBuffer;
typedef void(__cdecl *TickCallback)(const std::chrono::duration<double>& deltaTime);
typedef void(__cdecl *EventCallback)(void* param);
typedef void(__cdecl *ConsoleInputCallback)(const std::string& input, ConsoleBuffer* buffer);
typedef void(__cdecl *UserInputBoxCallback)(const std::string& boxTag, const std::string& result);
typedef std::initializer_list<std::string> StringArrayInitializerType;

struct PlayerInfo
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
	ConsoleInputCallback InputCallback;
	bool Visible;
	bool Focused;
	int TimeLastShown = 0;

	ConsoleBuffer(const std::string& name, const std::string& group, ConsoleInputCallback inputCallback, bool visible = false)
	{
		Name = name;
		Group = group;
		InputCallback = inputCallback;
		Visible = visible;
		Focused = false;
		ScrollIndex = 0;
		MaxDisplayLines = 10;
	}

	void PushLine(const std::string& line)
	{
		Messages.push_back(line);
		TimeLastShown = GetTickCount();
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
	virtual bool OnEvent(const std::string& eventNamespace, const std::string& eventName, EventCallback callback) = 0;

	/// <summary>
	/// Unregisters a TickCallback.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was removed.</returns>
	virtual bool RemoveOnTick(TickCallback callback) = 0;

	/// <summary>
	/// Unregisters a WNDPROC callback.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was removed.</returns>
	virtual bool RemoveOnWndProc(WNDPROC callback) = 0;

	/// <summary>
	/// Unregisters an EventCallback.
	/// </summary>
	/// <param name="eventNamespace">The namespace the event belongs to.</param>
	/// <param name="eventName">The name of the event.</param>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was removed.</returns>
	virtual bool RemoveOnEvent(const std::string& eventNamespace, const std::string& eventName, EventCallback callback) = 0;

	/// <summary>
	/// Calls each of the registered callbacks for the specified event.
	/// </summary>
	/// <param name="eventNamespace">The namespace the event belongs to.</param>
	/// <param name="eventName">The name of the event.</param>
	/// <param name="param">The parameter to pass to the callbacks.</param>
	virtual void Event(const std::string& eventNamespace, const std::string& eventName, void* param = 0) = 0;

	/// <summary>
	/// Registers an interface, plugins can use this to share classes across plugins.
	/// </summary>
	/// <param name="interfaceName">Name of the interface.</param>
	/// <param name="ptrToInterface">Pointer to an instance of the interface.</param>
	/// <returns>true if the interface was registered, false if an interface already exists with this name</returns>
	virtual bool RegisterInterface(const std::string& interfaceName, void* ptrToInterface) = 0;

	/// <summary>
	/// Gets an instance of the specified interface, if its been registered.
	/// </summary>
	/// <param name="interfaceName">Name of the interface.</param>
	/// <param name="returnCode">Returns 0 if the interface was found, otherwise 1 if it couldn't.</param>
	/// <returns>A pointer to the requested interface.</returns>
	virtual void* CreateInterface(const std::string& interfaceName, int* returnCode) = 0;

	/// <summary>
	/// Prints a string to the console UI.
	/// </summary>
	/// <param name="str">The string to print.</param>
	virtual void PrintToConsole(const std::string& str) = 0;

	/// <summary>
	/// Adds a new buffer/queue to the console UI.
	/// </summary>
	/// <param name="buffer">The buffer to add.</param>
	/// <returns>A pointer to the added buffer.</returns>
	virtual ConsoleBuffer* AddConsoleBuffer(ConsoleBuffer buffer) = 0;

	/// <summary>
	/// Sets the active console UI buffer to this buffer (for the buffers group only)
	/// </summary>
	/// <param name="buffer">The buffer to set as active.</param>
	/// <returns>true if the buffer was set active.</returns>
	virtual bool SetActiveConsoleBuffer(ConsoleBuffer* buffer) = 0;

	/// <summary>
	/// Shows a message box to the user with choices specified by the vector, the result the user chose is given to the callback.
	/// </summary>
	/// <param name="text">The title of the box.</param>
	/// <param name="text">The text to show to the user.</param>
	/// <param name="tag">A tag/identifier value that gets passed to the callback function.</param>
	/// <param name="choices">The choices the user can choose from.</param>
	/// <param name="callback">The function to call after the user has made a selection.</param>
	virtual void ShowMessageBox(const std::string& title, const std::string& text, const std::string& tag, const std::vector<std::string>& choices, UserInputBoxCallback callback) = 0;

	/// <summary>
	/// Shows an input box to the user where the user can type in an answer, the answer is passed as a parameter to the callback.
	/// </summary>
	/// <param name="text">The title of the box.</param>
	/// <param name="text">The text to show to the user.</param>
	/// <param name="tag">A tag/identifier value that gets passed to the callback function.</param>
	/// <param name="defaultText">The default text to fill the inputbox with.</param>
	/// <param name="callback">The function to call after the user has answered.</param>
	virtual void ShowInputBox(const std::string& title, const std::string& text, const std::string& tag, const std::string& defaultText, UserInputBoxCallback callback) = 0;

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
	/// Returns an ArrayGlobal pointer to the specified TLS global.
	/// </summary>
	/// <param name="offset">The offset to the global in the TLS.</param>
	/// <returns>A pointer to the ArrayGlobal.</returns>
	virtual Blam::ArrayGlobal* GetArrayGlobal(size_t offset) = 0;

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
	
	// TODO2: change this into a struct which is filled with IP/Port/GamePort/VoIPPort from server info json
	/// <summary>
	/// Gets the IP of the server we're connected to (only works if connected through Server.Connect!)
	/// </summary>
	/// <returns>The IP in network endian.</returns>
	virtual uint32_t GetServerIP() = 0;
	
	/// <summary>
	/// Gets the name of the player.
	/// </summary>
	/// <returns>The name of the player.</returns>
	virtual std::string GetPlayerName() = 0;

	/// <summary>
	/// Gets a pointer to the active network session.
	/// Can be null!
	/// </summary>
	/// <returns>A pointer to the active network session.</returns>
	virtual Blam::Network::Session* GetActiveNetworkSession() = 0;

	/// <summary>
	/// Gets a pointer to the active packet table.
	/// Can be null!
	/// </summary>
	/// <returns>A pointer to the active packet table.</returns>
	virtual Blam::Network::PacketTable* GetPacketTable() = 0;

	/// <summary>
	/// Sets the active packet table.
	/// Only use this if you know what you're doing!
	/// </summary>
	/// <param name="newTable">The new packet table.</param>
	virtual void SetPacketTable(const Blam::Network::PacketTable* newTable) = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"

/* use this class if you're updating IEngine after we've released a build
also update the IEngine typedef and ENGINE_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class IEngine002 : public IEngine001
{

};

#define ENGINE_INTERFACE_VERSION002 "Engine002"*/

typedef IEngine001 IEngine;
#define ENGINE_INTERFACE_LATEST ENGINE_INTERFACE_VERSION001
