#include "Engine.hpp"
#include "ElDorito.hpp"
#include <d3d9.h>
#include <algorithm>

namespace
{
	void GameTickHook(int frames, float* deltaTimeInfo)
	{
		// Tick ElDorito
		float deltaTime = *deltaTimeInfo;
		ElDorito::Instance().Engine.Tick(std::chrono::duration<double>(deltaTime));

		// Tick the game
		typedef void(*GameTickPtr)(int frames, float* deltaTimeInfo);
		auto GameTick = reinterpret_cast<GameTickPtr>(0x5336F0);
		GameTick(frames, deltaTimeInfo);
	}

	void TagsLoadedHookImpl()
	{
		ElDorito::Instance().Engine.Event("Core", "Engine.TagsLoaded");
	}

	__declspec(naked) void TagsLoadedHook()
	{
		__asm
		{
			call TagsLoadedHookImpl
			push 0x6D617467
			push 0x5030EF
			ret
		}
	}

	LRESULT __stdcall EngineWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ElDorito::Instance().Engine.WndProc(hWnd, msg, wParam, lParam);
	}

	DWORD __cdecl Network_managed_session_create_session_internalHook(int a1, int a2)
	{
		DWORD isOnline = *(DWORD*)a2;
		bool isHost = (*(uint16_t *)(a2 + 284) & 1);

		typedef DWORD(__cdecl *Network_managed_session_create_session_internalPtr)(int a1, int a2);
		auto Network_managed_session_create_session_internal = reinterpret_cast<Network_managed_session_create_session_internalPtr>(0x481550);
		auto retval = Network_managed_session_create_session_internal(a1, a2);

		if (isHost)
			ElDorito::Instance().Engine.Event("Core", isOnline == 1 ? "Server.Start" : "Server.Stop");

		return retval;
	}

	bool __fastcall Network_leader_request_boot_machineHook(void* thisPtr, void* unused, void* playerAddr, int reason)
	{
		uint32_t base = 0x1A4DC98;
		uint32_t playerOffset = (uint32_t)playerAddr - base;
		uint32_t playerIndex = playerOffset / 0xF8; // 0xF8 = size of player entry in this struct

		uint32_t uidBase = 0x1A4ED18;
		uint32_t uidOffset = uidBase + (0x1648 * playerIndex) + 0x50;
		uint64_t uid = Pointer(uidOffset).Read<uint64_t>();

		wchar_t playerName[0x10];
		memcpy(playerName, (char*)uidOffset + 0x8, 0x10 * sizeof(wchar_t));

		typedef bool(__thiscall *Network_leader_request_boot_machinePtr)(void* thisPtr, void* playerAddr, int reason);
		auto Network_leader_request_boot_machine = reinterpret_cast<Network_leader_request_boot_machinePtr>(0x45D4A0);
		bool retVal = Network_leader_request_boot_machine(thisPtr, playerAddr, reason);
		PlayerInfo info = { ElDorito::Instance().Utils.ThinString(playerName), uid };
		if (retVal)
			ElDorito::Instance().Engine.Event("Core", "Server.PlayerKick", &info);

		return retVal;
	}

	char __fastcall Network_state_end_game_write_stats_enterHook(void* thisPtr, int unused, int a2, int a3, int a4)
	{
		ElDorito::Instance().Engine.Event("Core", "Game.End");

		typedef char(__thiscall *Network_state_end_game_write_stats_enterPtr)(void* thisPtr, int a2, int a3, int a4);
		auto Network_state_end_game_write_stats_enter = reinterpret_cast<Network_state_end_game_write_stats_enterPtr>(0x492B50);
		return Network_state_end_game_write_stats_enter(thisPtr, a2, a3, a4);
	}

	char __fastcall Network_state_leaving_enterHook(void* thisPtr, int unused, int a2, int a3, int a4)
	{
		ElDorito::Instance().Engine.Event("Core", "Game.Leave");

		typedef char(__thiscall *Network_state_leaving_enterPtr)(void* thisPtr, int a2, int a3, int a4);
		auto Network_state_leaving_enter = reinterpret_cast<Network_state_leaving_enterPtr>(0x4933E0);
		return Network_state_leaving_enter(thisPtr, a2, a3, a4);
	}

	HRESULT __stdcall D3D9Device_EndSceneHook(IDirect3DDevice9* device)
	{
		ElDorito::Instance().Engine.EndScene(device);
		return device->EndScene();
	}

	void D3D9Device_InitHookImpl()
	{
		ElDorito::Instance().Engine.Event("Core", "Direct3D.Init", nullptr);
	}

	__declspec(naked) void D3D9Device_InitHook()
	{
		__asm
		{
			mov eax, 0xA20370
			call eax
			add esp, 8
			call D3D9Device_InitHookImpl
			push 0x42E5A0
			ret
		}
	}

	void PongReceivedHookImpl(const Blam::Network::NetworkAddress &from, const Blam::Network::PongPacket &pong, uint32_t latency)
	{
		auto* data = new std::tuple<const Blam::Network::NetworkAddress&, uint32_t, uint16_t, uint32_t>(from, pong.Timestamp, pong.ID, latency);
		ElDorito::Instance().Engine.Event("Core", "Server.PongReceived", data);
		delete data;
	}

	__declspec(naked) void PongReceivedHook()
	{
		__asm
		{
			push esi // Latency
			push edi // Pong packet
			push dword ptr[ebp + 8] // Sender
			call PongReceivedHookImpl
			add esp, 12
			push 0x49D9FA
			ret
		}
	}

	void LifeCycleStateChangedHookImpl(Blam::Network::LifeCycleState newState)
	{
		ElDorito::Instance().Engine.Event("Core", "Server.LifeCycleStateChanged", (void*)&newState);
	}

	__declspec(naked) void LifeCycleStateChangedHook()
	{
		__asm
		{
			pop esi // HACK: esi = return address

			// Execute replaced code
			mov ecx, edi // ecx = New lifecycle state object
			call dword ptr[eax + 8] // lifecycle->enter()

			push dword ptr[ebx] // Lifecycle state type
			call LifeCycleStateChangedHookImpl
			add esp, 4
			jmp esi
		}
	}
}

/// <summary>
/// Initializes a new instance of the <see cref="Engine"/> class.
/// </summary>
Engine::Engine()
{
	auto& patches = ElDorito::Instance().PatchManager;

	// hook our engine events
	enginePatchSet = patches.AddPatchSet("Engine",
	{
		Patch("WndProc", 0x42EB63, Utils::Misc::ConvertToVector<void*>(EngineWndProc)),
		Patch("Network_EndGameWriteStats", 0x16183A0, Utils::Misc::ConvertToVector<void*>(Network_state_end_game_write_stats_enterHook)),
		Patch("Network_Leaving", 0x16183BC, Utils::Misc::ConvertToVector<void*>(Network_state_leaving_enterHook)),
		Patch("D3DEndScene1", 0xA2179B, { 0x90 })
	},
	{
		Hook("GameTick", 0x505E64, GameTickHook, HookType::Call),
		Hook("TagsLoaded", 0x5030EA, TagsLoadedHook, HookType::Jmp),
		Hook("ServerSessionInfo", 0x482AAC, Network_managed_session_create_session_internalHook, HookType::Call),
		Hook("PlayerKick", 0x437E17, Network_leader_request_boot_machineHook, HookType::Call),
		Hook("D3DEndScene2", 0xA21796, D3D9Device_EndSceneHook, HookType::Call),
		Hook("D3DInit", 0x42E598, D3D9Device_InitHook, HookType::Jmp),
		Hook("OnPong", 0x49D9DB, PongReceivedHook, HookType::Jmp),
		Hook("LifeCycleStateChanged1", 0x48E527, LifeCycleStateChangedHook, HookType::Call),
		Hook("LifeCycleStateChanged2", 0x48E10F, LifeCycleStateChangedHook, HookType::Call),

	});
	patches.TogglePatchSet(enginePatchSet);
}

Engine::~Engine()
{
	ElDorito::Instance().PatchManager.EnablePatchSet(enginePatchSet, false);
}

/// <summary>
/// Registers a callback which is called when the game ticks, tick callbacks use a seperate path to normal events so we can process them in less cycles.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnTick(TickCallback callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

/// <summary>
/// Registers a callback which is called when the game calls D3DDevice::EndScene, these callbacks use a seperate path to normal events so we can process them in less cycles.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnEndScene(EventCallback callback)
{
	endSceneCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

/// <summary>
/// Registers a callback which is called when a WM message is received (registers another WNDPROC)
/// If the callback returns 1 then the games WNDPROC won't be called.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnWndProc(WndProcCallback callback)
{
	wndProcCallbacks.push_back(callback);
	return true;
}

/// <summary>
/// Adds a callback to be called when the specified event occurs. If the eventNamespace/eventName combination doesn't exist a new event will be created.
/// Note that the "Core" namespace is restricted to events created by ElDorito.
/// </summary>
/// <param name="eventNamespace">The namespace the event belongs to.</param>
/// <param name="eventName">The name of the event.</param>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnEvent(const std::string& eventNamespace, const std::string& eventName, EventCallback callback)
{
	std::string eventId = eventNamespace + "." + eventName;
	auto it = eventCallbacks.find(eventId);
	if (it != eventCallbacks.end())
	{
		// TODO: check if callback is already registered for this event, bad plugin coders might have put their OnEvent in a loop by accident or something
		(*it).second.push_back(callback);
		return true;
	}

	// callback wasn't found, create a new one!
	ElDorito::Instance().Logger.Log(LogSeverity::Debug, "EngineEvent", "%s event created", eventId.c_str());
	eventCallbacks.insert(std::pair<std::string, std::vector<EventCallback>>(eventId, std::vector<EventCallback>{ callback }));
	return true;
}

/// <summary>
/// Unregisters a TickCallback.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was removed.</returns>
bool Engine::RemoveOnTick(TickCallback callback)
{
	//TODO2:
	//tickCallbacks.erase(std::remove(tickCallbacks.begin(), tickCallbacks.end(), callback), tickCallbacks.end());
	return false;// true;
}

/// <summary>
/// Unregisters a WNDPROC callback.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was removed.</returns>
bool Engine::RemoveOnWndProc(WndProcCallback callback)
{
	//TODO2:
	//wndProcCallbacks.erase(std::remove(wndProcCallbacks.begin(), wndProcCallbacks.end(), callback), wndProcCallbacks.end());
	return true;
}

/// <summary>
/// Unregisters an EventCallback.
/// </summary>
/// <param name="eventNamespace">The namespace the event belongs to.</param>
/// <param name="eventName">The name of the event.</param>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was removed.</returns>
bool Engine::RemoveOnEvent(const std::string& eventNamespace, const std::string& eventName, EventCallback callback)
{
	//TODO2:
	/*std::string eventId = eventNamespace + "." + eventName;
	auto it = eventCallbacks.find(eventId);
	if (it != eventCallbacks.end())
		(*it).second.erase(std::remove((*it).second.begin(), (*it).second.end(), callback), (*it).second.end());
		*/
	return false;// true;
}

/// <summary>
/// Calls each of the registered tick callbacks.
/// </summary>
/// <param name="deltaTime">The delta time.</param>
void Engine::Tick(const std::chrono::duration<double>& deltaTime)
{
	if (!hasFirstTickTocked)
	{
		hasFirstTickTocked = true;
		this->Event("Core", "Engine.FirstTick");
	}
	for (auto callback : tickCallbacks)
		callback(deltaTime);
}

/// <summary>
/// Calls each of the registered EndScene callbacks.
/// </summary>
/// <param name="d3dDevice">The pointer to the d3d device.</param>
void Engine::EndScene(void* d3dDevice)
{
	for (auto callback : endSceneCallbacks)
		callback(d3dDevice);
}

/// <summary>
/// Calls each of the registered tick callbacks.
/// </summary>
LRESULT Engine::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool callGame = true;
	for (auto callback : wndProcCallbacks)
	{
		if (callback(hWnd, msg, wParam, lParam) != 0)
			callGame = false;
	}

	if (!callGame)
		return 0;

	typedef int(__stdcall *Game_WndProcPtr)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	auto Game_WndProc = reinterpret_cast<Game_WndProcPtr>(0x42E6A0);
	return Game_WndProc(hWnd, msg, wParam, lParam);
}

/// <summary>
/// Calls each of the registered callbacks for the specified event.
/// </summary>
/// <param name="eventNamespace">The namespace the event belongs to.</param>
/// <param name="eventName">The name of the event.</param>
/// <param name="param">The parameter to pass to the callbacks.</param>
void Engine::Event(const std::string& eventNamespace, const std::string& eventName, void* param)
{
	// TODO: find a way to optimize this?

	std::string eventId = eventNamespace + "." + eventName;
	if (eventId.compare("Core.Input.KeyboardUpdate") && eventId.compare("Core.Direct3D.EndScene")) // don't show keyboard update spam
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "EngineEvent", "%s event triggered", eventId.c_str());

	if (!eventId.compare("Core.Engine.MainMenuShown"))
	{
		if (this->mainMenuHasShown)
			return; // this event should only occur once during the lifecycle of the game
		this->mainMenuHasShown = true;
	}

	auto it = eventCallbacks.find(eventId);
	if (it == eventCallbacks.end())
	{
		if (eventId.compare("Core.Input.KeyboardUpdate") && eventId.compare("Core.Direct3D.EndScene"))
			ElDorito::Instance().Logger.Log(LogSeverity::Debug, "EngineEvent", "%s event not created (nobody is listening for this event!)", eventId.c_str());
		return;
	}

	for (auto callback : (*it).second)
		callback(param);
}

/// <summary>
/// Registers an interface, plugins can use this to share classes across plugins.
/// </summary>
/// <param name="interfaceName">Name of the interface.</param>
/// <param name="ptrToInterface">Pointer to an instance of the interface.</param>
/// <returns>true if the interface was registered, false if an interface already exists with this name</returns>
bool Engine::RegisterInterface(const std::string& interfaceName, void* ptrToInterface)
{
	auto& dorito = ElDorito::Instance();

	if (!interfaceName.compare(COMMANDMANAGER_INTERFACE_VERSION001) ||
		!interfaceName.compare(ENGINE_INTERFACE_VERSION001) ||
		!interfaceName.compare(DEBUGLOG_INTERFACE_VERSION001) ||
		!interfaceName.compare(PATCHMANAGER_INTERFACE_VERSION001) ||
		!interfaceName.compare(UTILS_INTERFACE_VERSION001))
	{
		dorito.Logger.Log(LogSeverity::Error, "Engine", "Tried registering built-in interface %s!", interfaceName.c_str());
		return false; // can't register these
	}

	auto it = interfaces.find(interfaceName);
	if (it != interfaces.end())
	{
		dorito.Logger.Log(LogSeverity::Error, "Engine", "Failed to register interface %s as it already exists!", interfaceName.c_str());
		return false;
	}

	interfaces.insert(std::pair<std::string, void*>(interfaceName, ptrToInterface));
	dorito.Logger.Log(LogSeverity::Debug, "Engine", "Registered interface %s", interfaceName.c_str());
	return true;
}

/// <summary>
/// Gets an instance of the specified interface, if its been registered.
/// </summary>
/// <param name="interfaceName">Name of the interface.</param>
/// <param name="returnCode">Returns 0 if the interface was found, otherwise 1 if it couldn't.</param>
/// <returns>A pointer to the requested interface.</returns>
void* Engine::CreateInterface(const std::string& interfaceName, int* returnCode)
{
	auto& dorito = ElDorito::Instance();

	*returnCode = 0;
	if (!interfaceName.compare(COMMANDMANAGER_INTERFACE_VERSION001))
		return &dorito.CommandManager;
	if (!interfaceName.compare(ENGINE_INTERFACE_VERSION001))
		return &dorito.Engine;
	if (!interfaceName.compare(DEBUGLOG_INTERFACE_VERSION001))
		return &dorito.Logger;
	if (!interfaceName.compare(PATCHMANAGER_INTERFACE_VERSION001))
		return &dorito.PatchManager;
	if (!interfaceName.compare(USERINTERFACE_INTERFACE_VERSION001))
		return &dorito.UserInterface;
	if (!interfaceName.compare(UTILS_INTERFACE_VERSION001))
		return &dorito.Utils;

	auto it = interfaces.find(interfaceName);
	if (it != interfaces.end())
		return (*it).second;

	*returnCode = 1;
	return nullptr;
}

bool Engine::IsDedicated()
{
	return ElDorito::Instance().NetworkPatches->GetDedicatedServerMode();
}

/// <summary>
/// Gets the IP of the server we're connected to (only works if connected through Server.Connect!)
/// </summary>
/// <returns>The IP in network endian.</returns>
uint32_t Engine::GetServerIP()
{
	// TODO1: Use BlamNetwork to get the IP
	return *(uint32_t*)(ElDorito::Instance().ServerCommands->SyslinkData + 0x170);
}

/// <summary>
/// Gets the name of the player.
/// </summary>
/// <returns>The name of the player.</returns>
std::string Engine::GetPlayerName()
{
	return ElDorito::Instance().PlayerCommands->VarName->ValueString;
}

/// <summary>
/// Gets a pointer to the active network session.
/// Can be null!
/// </summary>
/// <returns>A pointer to the active network session.</returns>
Blam::Network::Session* Engine::GetActiveNetworkSession()
{
	auto networkSessionPtr = Pointer(0x19AB848);
	return networkSessionPtr.Read<Blam::Network::Session*>();
}

/// <summary>
/// Gets a pointer to the active packet table.
/// Can be null!
/// </summary>
/// <returns>A pointer to the active packet table.</returns>
Blam::Network::PacketTable* Engine::GetPacketTable()
{
	auto packetTablePtr = Pointer(0x224A498);
	return packetTablePtr.Read<Blam::Network::PacketTable*>();
}

/// <summary>
/// Sets the active packet table.
/// Only use this if you know what you're doing!
/// </summary>
/// <param name="newTable">The new packet table.</param>
void Engine::SetPacketTable(const Blam::Network::PacketTable* newTable)
{
	auto packetTablePtr = Pointer(0x224A498);
	packetTablePtr.Write<const Blam::Network::PacketTable*>(newTable);
}

/// <summary>
/// Gets the main TLS address (optionally with an offset added)
/// </summary>
/// <param name="offset">The offset to add to the TLS address.</param>
/// <returns>A Pointer to the TLS.</returns>
Pointer Engine::GetMainTls(size_t offset)
{
	static Pointer ThreadLocalStorage;
	if (!ThreadLocalStorage && GetGameThreadID())
	{
		size_t MainThreadID = GetGameThreadID();

		HANDLE MainThreadHandle = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, false, MainThreadID);

		// Get thread context
		CONTEXT MainThreadContext;
		MainThreadContext.ContextFlags = CONTEXT_FULL;

		if (MainThreadID != GetCurrentThreadId())
			SuspendThread(MainThreadHandle);

		BOOL success = GetThreadContext(MainThreadHandle, &MainThreadContext);
		if (!success)
		{
			OutputDebugStringA(std::string("Error getting thread context: ").append(std::to_string(GetLastError())).c_str());
			std::exit(1);
		}
		ResumeThread(MainThreadHandle);

		// Get thread selector

		LDT_ENTRY MainThreadLdt;

		success = GetThreadSelectorEntry(MainThreadHandle, MainThreadContext.SegFs, &MainThreadLdt);
		if (!success)
		{
			OutputDebugStringA(std::string("Error getting thread context: ").append(std::to_string(GetLastError())).c_str());
		}
		size_t TlsPtrArrayAddress = (size_t)((size_t)(MainThreadLdt.HighWord.Bits.BaseHi << 24) | (MainThreadLdt.HighWord.Bits.BaseMid << 16) | MainThreadLdt.BaseLow) + 0x2C;
		size_t TlsPtrAddress = Pointer(TlsPtrArrayAddress).Read<uint32_t>();

		// Index has been consistantly 0. Keep a look out.
		ThreadLocalStorage = Pointer(TlsPtrAddress)[0];
	}

	return ThreadLocalStorage(offset);
}

/// <summary>
/// Returns an ArrayGlobal pointer to the specified TLS global.
/// </summary>
/// <param name="offset">The offset to the global in the TLS.</param>
/// <returns>A pointer to the ArrayGlobal.</returns>
Blam::ArrayGlobal* Engine::GetArrayGlobal(size_t offset)
{
	auto ptr = GetMainTls(offset)[0];
	return reinterpret_cast<Blam::ArrayGlobal*>((void*)ptr);
}

/// <summary>
/// Gets the number of ticks that a key has been held down for.
/// Will always be nonzero if the key is down.
/// </summary>
/// <param name="key">The key.</param>
/// <param name="type">The input type.</param>
/// <returns>The number of ticks that a key has been held down for.</returns>
uint8_t Engine::GetKeyTicks(Blam::Input::KeyCodes key, Blam::Input::InputType type)
{
	typedef uint8_t(*EngineGetKeyTicksPtr)(Blam::Input::KeyCodes, Blam::Input::InputType);
	auto EngineGetKeyTicks = reinterpret_cast<EngineGetKeyTicksPtr>(0x511B60);
	return EngineGetKeyTicks(key, type);
}

/// <summary>
/// Gets the number of milliseconds that a key has been held down for.
/// Will always be nonzero if the key is down.
/// </summary>
/// <param name="key">The key.</param>
/// <param name="type">The input type.</param>
/// <returns>The number of milliseconds that a key has been held down for.</returns>
uint16_t Engine::GetKeyMs(Blam::Input::KeyCodes key, Blam::Input::InputType type)
{
	typedef uint8_t(*EngineGetKeyMsPtr)(Blam::Input::KeyCodes, Blam::Input::InputType);
	auto EngineGetKeyMs = reinterpret_cast<EngineGetKeyMsPtr>(0x511CE0);
	return EngineGetKeyMs(key, type);
}

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
bool Engine::ReadKeyEvent(Blam::Input::KeyEvent* result, Blam::Input::InputType type)
{
	typedef bool(*EngineReadKeyEventPtr)(Blam::Input::KeyEvent*, Blam::Input::InputType);
	auto EngineReadKeyEvent = reinterpret_cast<EngineReadKeyEventPtr>(0x5118C0);
	return EngineReadKeyEvent(result, type);
}

/// <summary>
/// Blocks or unblocks an input type.
/// </summary>
/// <param name="type">The input type.</param>
void Engine::BlockInput(Blam::Input::InputType type, bool block)
{
	typedef uint8_t(*EngineBlockInputPtr)(Blam::Input::InputType, bool);
	auto EngineBlockInput = reinterpret_cast<EngineBlockInputPtr>(0x512530);
	EngineBlockInput(type, block);
}

void Engine::PushInputContext(std::shared_ptr<InputContext> context)
{
	ElDorito::Instance().InputPatches->PushContext(context);
}

void Engine::SendPacket(int targetPeer, const void* packet, int packetSize)
{
	auto session = GetActiveNetworkSession();
	if (!session)
		return;
	auto channelIndex = session->MembershipInfo.PeerChannels[targetPeer].ChannelIndex;
	if (channelIndex == -1)
		return;

	const int CustomPacketId = 0x27; // Last packet ID used by the game is 0x26
	session->Observer->ObserverChannelSendMessage(0, channelIndex, false, CustomPacketId, packetSize, packet);
}

Packets::PacketGuid Engine::RegisterPacketImpl(const std::string &name, std::shared_ptr<Packets::RawPacketHandler> handler)
{
	Packets::PacketGuid guid;
	auto& dorito = ElDorito::Instance();
	if (!dorito.Utils.Hash32(name, &guid))
		throw std::runtime_error("Failed to generate packet GUID");
	if (LookUpPacketType(guid))
		throw std::runtime_error("Duplicate packet GUID"); // TODO: Throwing an exception here might not be the best idea...
	CustomPacket packet;
	packet.Name = name;
	packet.Handler = handler;
	customPackets[guid] = packet;
	return guid;
}

CustomPacket* Engine::LookUpPacketType(Packets::PacketGuid guid)
{
	auto it = customPackets.find(guid);
	if (it == customPackets.end())
		return nullptr;
	return &it->second;
}