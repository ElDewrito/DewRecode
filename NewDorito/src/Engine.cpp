#include "Engine.hpp"
#include "ElDorito.hpp"

namespace
{
	void GameTickHook(int frames, float *deltaTimeInfo)
	{
		// Tick ElDorito
		float deltaTime = *deltaTimeInfo;
		ElDorito::Instance().Engine.Tick(std::chrono::duration<double>(deltaTime));

		// Tick the game
		typedef void(*GameTickPtr)(int frames, float *deltaTimeInfo);
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

		typedef DWORD(__cdecl *Network_managed_session_create_session_internalFunc)(int a1, int a2);
		Network_managed_session_create_session_internalFunc Network_managed_session_create_session_internal = (Network_managed_session_create_session_internalFunc)0x481550;
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

		typedef bool(__thiscall *Network_leader_request_boot_machineFunc)(void *thisPtr, void* playerAddr, int reason);
		const Network_leader_request_boot_machineFunc Network_leader_request_boot_machine = reinterpret_cast<Network_leader_request_boot_machineFunc>(0x45D4A0);
		bool retVal = Network_leader_request_boot_machine(thisPtr, playerAddr, reason);
		PlayerKickInfo info = { ElDorito::Instance().Utils.ThinString(playerName), uid };
		if (retVal)
			ElDorito::Instance().Engine.Event("Core", "Server.PlayerKick", &info);

		return retVal;
	}

}

/// <summary>
/// Initializes a new instance of the <see cref="Engine"/> class.
/// </summary>
Engine::Engine()
{
	auto& patches = ElDorito::Instance().Patches;

	// hook our engine events
	patches.TogglePatchSet(patches.AddPatchSet("Engine",
	{
		Patch("WndProc", 0x42EB63, Utils::Misc::ConvertToVector<void*>(EngineWndProc))
	},
	{
		Hook("GameTick", 0x505E64, GameTickHook, HookType::Call),
		Hook("TagsLoaded", 0x5030EA, TagsLoadedHook, HookType::Jmp),
		Hook("ServerSessionInfo", 0x482AAC, Network_managed_session_create_session_internalHook, HookType::Call),
		Hook("PlayerKick", 0x437E17, Network_leader_request_boot_machineHook, HookType::Call)
	}));
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
/// Registers a callback which is called when a WM message is received (registers another WNDPROC)
/// If the callback returns 1 then the games WNDPROC won't be called.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnWndProc(WNDPROC callback)
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
bool Engine::OnEvent(std::string eventNamespace, std::string eventName, EventCallback callback)
{
	std::string eventId = eventNamespace + "." + eventName;
	for (auto kvp : eventCallbacks)
	{
		if (!kvp.first.compare(eventId))
		{
			// TODO: check if callback is already registered for this event, bad plugin coders might have put their OnEvent in a loop by accident or something
			kvp.second.push_back(callback);
			return true;
		}
	}

	// callback wasn't found, create a new one!
	ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%s event created", eventId.c_str());
	eventCallbacks.insert(std::pair<std::string, std::vector<EventCallback>>(eventId, std::vector<EventCallback>{ callback }));
	return true;
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
void Engine::Event(std::string eventNamespace, std::string eventName, void* param)
{
	// TODO: find a way to optimize this?

	std::string eventId = eventNamespace + "." + eventName;
	if (eventId.compare("Core.Input.KeyboardUpdate")) // don't show keyboard update spam
		ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%s event triggered", eventId.c_str());

	if (!eventId.compare("Core.MainMenuShown"))
	{
		if (this->mainMenuHasShown)
			return; // this event should only occur once during the lifecycle of the game
		this->mainMenuHasShown = true;
	}

	bool found = false;
	for (auto kvp : eventCallbacks)
	{
		if (kvp.first.compare(eventId))
			continue;

		found = true;
		for (auto callback : kvp.second)
			callback(param);
	}
	if (!found)
		ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%s event not created (nobody is listening for this event!)", eventId.c_str());
}

/// <summary>
/// Registers an interface, plugins can use this to share classes across plugins.
/// </summary>
/// <param name="interfaceName">Name of the interface.</param>
/// <param name="ptrToInterface">Pointer to an instance of the interface.</param>
/// <returns>true if the interface was registered, false if an interface already exists with this name</returns>
bool Engine::RegisterInterface(std::string interfaceName, void* ptrToInterface)
{
	if (!interfaceName.compare(CONSOLE_INTERFACE_VERSION001) ||
		!interfaceName.compare(ENGINE_INTERFACE_VERSION001) ||
		!interfaceName.compare(DEBUGLOG_INTERFACE_VERSION001) ||
		!interfaceName.compare(PATCHMANAGER_INTERFACE_VERSION001) ||
		!interfaceName.compare(UTILS_INTERFACE_VERSION001))
		return false; // can't register these

	for (auto kvp : interfaces)
		if (!kvp.first.compare(interfaceName))
		{
			ElDorito::Instance().Logger.Log(LogLevel::Error, "Engine", "Failed to register interface %s as it already exists!", interfaceName.c_str());
			return false;
		}

	interfaces.insert(std::pair<std::string, void*>(interfaceName, ptrToInterface));
	ElDorito::Instance().Logger.Log(LogLevel::Debug, "Engine", "Registered interface %s", interfaceName.c_str());
	return true;
}

/// <summary>
/// Gets an instance of the specified interface, if its been registered.
/// </summary>
/// <param name="interfaceName">Name of the interface.</param>
/// <param name="returnCode">Returns 0 if the interface was found, otherwise 1 if it couldn't.</param>
/// <returns>A pointer to the requested interface.</returns>
void* Engine::CreateInterface(std::string interfaceName, int* returnCode)
{
	auto& dorito = ElDorito::Instance();

	*returnCode = 0;
	if (!interfaceName.compare(CONSOLE_INTERFACE_VERSION001))
		return &dorito.Console;
	if (!interfaceName.compare(ENGINE_INTERFACE_VERSION001))
		return &dorito.Engine;
	if (!interfaceName.compare(DEBUGLOG_INTERFACE_VERSION001))
		return &dorito.Logger;
	if (!interfaceName.compare(PATCHMANAGER_INTERFACE_VERSION001))
		return &dorito.Patches;
	if (!interfaceName.compare(UTILS_INTERFACE_VERSION001))
		return &dorito.Utils;

	for (auto kvp : interfaces)
		if (!kvp.first.compare(interfaceName))
			return kvp.second;

	*returnCode = 1;
	return nullptr;
}

/// <summary>
/// Gets the main TLS address (optionally with an offset added)
/// </summary>
/// <param name="offset">The offset to add to the TLS address.</param>
/// <returns>A Pointer to the TLS.</returns>
Pointer Engine::GetMainTls(size_t offset)
{
	static Pointer ThreadLocalStorage;
	if (!ThreadLocalStorage && GetMainThreadID())
	{
		size_t MainThreadID = GetMainThreadID();

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