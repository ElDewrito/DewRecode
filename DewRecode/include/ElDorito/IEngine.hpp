#pragma once
#include "Pointer.hpp"
#include <chrono>
#include <functional>
#include <vector>
#include <bitset>

namespace Blam
{
	class ArrayGlobal;

	namespace Network
	{
		struct Session;
		struct PacketTable;
	}
}


namespace Chat
{
	typedef std::bitset<17> PeerBitSet; // TODO: make this use Blam::Network::MaxPeers instead
	struct ChatMessage;
	class ChatHandler;
}

namespace Packets
{
	class RawPacketHandler;
	typedef uint32_t PacketGuid;
}

struct CustomPacket
{
	std::string Name;
	std::shared_ptr<Packets::RawPacketHandler> Handler;
};

#define EDEVENT_ENGINE_FIRSTTICK "Engine.FirstTick"
#define EDEVENT_ENGINE_MAINMENUSHOWN "Engine.MainMenuShown"
#define EDEVENT_ENGINE_TAGSLOADED "Engine.TagsLoaded"
#define EDEVENT_INPUT_KEYBOARDUPDATE "Input.KeyboardUpdate"

#define EDEVENT_SERVER_START "Server.Start"
#define EDEVENT_SERVER_STOP "Server.Stop"
#define EDEVENT_SERVER_PLAYERKICK "Server.PlayerKick"
#define EDEVENT_SERVER_LIFECYCLESTATECHANGED "Server.LifeCycleStateChanged"
#define EDEVENT_SERVER_PONGRECEIVED "Server.PongReceived"

#define EDEVENT_GAME_JOINING "Game.Joining"
#define EDEVENT_GAME_LEAVE "Game.Leave"
#define EDEVENT_GAME_END "Game.End"

#define EDEVENT_DIRECT3D_INIT "Direct3D.Init"

#define EDEVENT_PLAYER_CHANGENAME "Player.ChangeName"
/* 
List of events registered by ED (eventNamespace/eventName seperated by a period, parameter in parens):
	Core.Engine.FirstTick - signalled when the game engine loop starts ticking, only signals once
	Core.Engine.MainMenuShown - when the mainmenu is first being shown, only signals once after the game has inited etc
	Core.Engine.TagsLoaded - when the tags have been reloaded
	Core.Input.KeyboardUpdate - when a key is pressed (i think? haven't looked into keyboard code much)

	Core.Server.Start - when the user has started a server
	Core.Server.Stop - when the user has stopped the server
	Core.Server.PlayerKick(PlayerInfo) - when a user has been kicked (host only)
	Core.Server.LifeCycleStateChanged - when the servers lifecycle state has changed (param = new lifecycle state)
	Core.Server.PongReceived - when a response to a ping is received from the server (param = std::tuple<const Blam::Network::NetworkAddress& from, uint32_t timestamp, uint16_t ID, uint32_t latency>* )

	Core.Game.Joining - when the user is joining a game (direct connect cmd issued)
	Core.Game.Leave - when the user leaves a game
	Core.Game.End - when a game has finished

	Core.Player.ChangeName - when the user successfully changes their name

(soon):
    Core.Direct3D.Present - when the game is about to call D3DDevice::Present
	Core.Round.Start - when a round has started
	Core.Round.End - when a round has finished
	Core.Game.Join - when the user has joined a game successfully
	Core.Game.Start - when a game has started
	Core.Player.Join(PlayerInfo) - when a user joins the game (signals for all users, not just host)
	Core.Player.Leave(PlayerInfo) - when a user leaves the game (signals for all users, not just host)

later:
	Core.Lobby.ChangeMap
	Core.Lobby.ChangeGameMode
	Core.Game.Event(EventID) - stuff like double kills etc, might include normal kills too
*/

struct ConsoleBuffer;
/*typedef void(__cdecl *TickCallback)(const std::chrono::duration<double>& deltaTime);
typedef void(__cdecl *EventCallback)(void* param);*/

typedef std::function<void(const std::string& input, ConsoleBuffer* buffer)> ConsoleInputCallback;
typedef std::function<void(const std::string& boxTag, const std::string& result)> UserInputBoxCallback;
#define BIND_CALLBACK2(classPtr, funcPtr) std::bind(funcPtr, classPtr, std::placeholders::_1, std::placeholders::_2)

typedef std::function<void(const std::chrono::duration<double>& deltaTime)> TickCallback;
typedef std::function<void(void* param)> EventCallback;
#define BIND_CALLBACK(classPtr, funcPtr) std::bind(funcPtr, classPtr, std::placeholders::_1)

typedef std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)> WndProcCallback;
#define BIND_WNDPROC(classPtr, funcPtr) std::bind(funcPtr, classPtr, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)

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
		TimeLastShown = timeGetTime();
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
	/// Registers a callback which is called when the game calls D3DDevice::EndScene, these callbacks use a seperate path to normal events so we can process them in less cycles.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was added, false if the callback is already registered.</returns>
	virtual bool OnEndScene(EventCallback callback) = 0;

	/// <summary>
	/// Registers a callback which is called when a WM message is received (registers another WNDPROC)
	/// If the callback returns 1 then the games WNDPROC won't be called.
	/// </summary>
	/// <param name="callback">The callback.</param>
	/// <returns>True if the callback was added, false if the callback is already registered.</returns>
	virtual bool OnWndProc(WndProcCallback callback) = 0;

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
	virtual bool RemoveOnWndProc(WndProcCallback callback) = 0;

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
	/// Returns true if the main menu has been shown, signifying that the game has initialized.
	/// </summary>
	/// <returns>true if the main menu has been shown.</returns>
	virtual bool HasMainMenuShown() = 0;

	/// <summary>
	/// Returns true if this is a dedicated server instance.
	/// </summary>
	/// <returns>true if this is a dedicated server instance.</returns>
	virtual bool IsDedicated() = 0;

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
	
	// TODO2: make another func which returns a struct filled with IP/Port/GamePort/VoIPPort from server info json
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

	virtual int GetNumPlayers() = 0;

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

	/// <summary>
	/// Gets the number of ticks that a key has been held down for.
	/// Will always be nonzero if the key is down.
	/// </summary>
	/// <param name="key">The key.</param>
	/// <param name="type">The input type.</param>
	/// <returns>The number of ticks that a key has been held down for.</returns>
	virtual uint8_t GetKeyTicks(Blam::Input::KeyCodes key, Blam::Input::InputType type) = 0;

	/// <summary>
	/// Gets the number of milliseconds that a key has been held down for.
	/// Will always be nonzero if the key is down.
	/// </summary>
	/// <param name="key">The key.</param>
	/// <param name="type">The input type.</param>
	/// <returns>The number of milliseconds that a key has been held down for.</returns>
	virtual uint16_t GetKeyMs(Blam::Input::KeyCodes key, Blam::Input::InputType type) = 0;

	/// <summary>
	/// Reads a raw keyboard input event. Returns false if nothing is
	/// available. You should call this in a loop to ensure that you process
	/// all available events. NOTE THAT THIS IS ONLY GUARANTEED TO WORK
	/// AFTER WINDOWS MESSAGES HAVE BEEN PUMPED IN THE UPDATE CYCLE. ALSO,
	/// THIS WILL NOT WORK IF UI INPUT IS DISABLED, REGARDLESS OF THE INPUT
	/// TYPE YOU SPECIFY.
	/// </summary>
	/// <param name="result">The resulting KeyEvent.</param>
	/// <param name="type">The input type.</param>
	/// <returns>false if nothing is available.</returns>
	virtual bool ReadKeyEvent(Blam::Input::KeyEvent* result, Blam::Input::InputType type) = 0;

	/// <summary>
	/// Blocks or unblocks an input type.
	/// </summary>
	/// <param name="type">The input type.</param>
	virtual void BlockInput(Blam::Input::InputType type, bool block) = 0;

	virtual void PushInputContext(std::shared_ptr<InputContext> context) = 0;

	virtual void SendPacket(int targetPeer, const void* packet, int packetSize) = 0;

	// Registers a packet handler under a particular name and returns the
	// GUID of the new packet type.
	virtual Packets::PacketGuid RegisterPacketImpl(const std::string &name, std::shared_ptr<Packets::RawPacketHandler> handler) = 0;
	virtual CustomPacket* LookUpPacketType(Packets::PacketGuid guid) = 0;

	/* CHAT COMMANDS */

	// Sends a message to every peer. Returns true if successful.
	virtual bool SendChatGlobalMessage(const std::string &body) = 0;

	// Sends a message to every player on the local player's team. Returns
	// true if successful.
	virtual bool SendChatTeamMessage(const std::string &body) = 0;

	// Sends a server message to specific peers. Only works if you are
	// host. Returns true if successful.
	virtual bool SendChatServerMessage(const std::string &body, Chat::PeerBitSet peers) = 0;

	virtual bool SendChatDirectedServerMessage(const std::string &body, int peer) = 0;

	// Registers a chat handler object.
	virtual void AddChatHandler(std::shared_ptr<Chat::ChatHandler> handler) = 0;

	virtual bool SetModEnabled(const std::string& guid, bool enabled) = 0;
	virtual bool GetModEnabled(const std::string& guid) = 0;
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
