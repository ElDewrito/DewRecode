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
		typedef void(*GameTickFunc)(int frames, float *deltaTimeInfo);
		auto GameTick = reinterpret_cast<GameTickFunc>(0x5336F0);
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
}

/// <summary>
/// Initializes a new instance of the <see cref="Engine"/> class.
/// </summary>
Engine::Engine()
{
	auto& patches = ElDorito::Instance().Patches;

	patches.TogglePatchSet(patches.AddPatchSet("Engine", {},
	{
		Hook("GameTick", 0x505E64, GameTickHook, HookType::Call),
		Hook("TagsLoaded", 0x5030EA, TagsLoadedHook, HookType::Jmp)
	}));
}

/// <summary>
/// Adds a callback to be called when the game ticks.
/// </summary>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnTick(TickCallbackFunc callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

/// <summary>
/// Adds a callback to be called when the specified event occurs.
/// </summary>
/// <param name="eventModule">The module that calls the event.</param>
/// <param name="eventName">The name of the event.</param>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnEvent(std::string eventModule, std::string eventName, EventCallbackFunc callback)
{
	std::string eventId = eventModule + "." + eventName;
	for (auto kvp : eventCallbacks)
	{
		if (kvp.first.compare(eventId))
		{
			// TODO: check if callback is already registered for this event, bad plugin coders might have put their OnEvent in a loop by accident or something
			kvp.second.push_back(callback);
			return true;
		}
	}

	// callback wasn't found, create a new one!
	ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%s event created", eventId.c_str());
	eventCallbacks.insert(std::pair<std::string, std::vector<EventCallbackFunc>>(eventId, std::vector<EventCallbackFunc>{ callback }));
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
/// Calls each of the registered callbacks for the specified event
/// </summary>
/// <param name="eventModule">The module that calls the event.</param>
/// <param name="eventName">The name of the event.</param>
/// <param name="param">The parameter to pass to the callbacks.</param>
void Engine::Event(std::string eventModule, std::string eventName, void* param)
{
	// TODO: find a way to optimize this?

	std::string eventId = eventModule + "." + eventName;
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
		ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%s event not created!", eventId.c_str());
}

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

// registers an interface, plugins can use this to share classes across plugins
bool Engine::RegisterInterface(std::string interfaceName, void* ptrToInterface)
{
	for (auto kvp : interfaces)
		if (!kvp.first.compare(interfaceName))
			return false; // interface with this name already exists

	interfaces.insert(std::pair<std::string, void*>(interfaceName, ptrToInterface));
	return true;
}

// returns the interface by the interface name
void* Engine::CreateInterface(std::string interfaceName, int* returnCode)
{
	*returnCode = 0;
	for (auto kvp : interfaces)
		if (!kvp.first.compare(interfaceName))
			return kvp.second;

	*returnCode = 1;
	return nullptr;
}