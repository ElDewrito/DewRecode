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
		ElDorito::Instance().Engine.Event(EngineEvent::TagsLoaded);
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
/// <param name="evt">The event.</param>
/// <param name="callback">The callback.</param>
/// <returns>True if the callback was added, false if the callback is already registered.</returns>
bool Engine::OnEvent(EngineEvent evt, EventCallbackFunc callback)
{
	eventCallbacks.insert(std::pair<EventCallbackFunc, EngineEvent>(callback, evt));
	return true; // todo: check if this callback is already registered
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
		this->Event(EngineEvent::FirstTick);
	}
	for (auto callback : tickCallbacks)
		callback(deltaTime);
}

/// <summary>
/// Calls each of the registered MainMenuShown callbacks.
/// </summary>
void Engine::MainMenuShown()
{
	if (this->mainMenuHasShown)
		return; // this event should only occur once during the lifecycle of the game

	this->mainMenuHasShown = true;
	this->Event(EngineEvent::MainMenuShown);
}

/// <summary>
/// Calls each of the registered callbacks for the specified event
/// </summary>
/// <param name="evt">The event.</param>
/// <param name="param">The parameter to pass to the callbacks.</param>
void Engine::Event(EngineEvent evt, void* param)
{
	ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%d", evt); // TODO: string event names

	for (auto kvp : eventCallbacks)
	{
		if (kvp.second != evt)
			continue;
		kvp.first(param);
	}
}