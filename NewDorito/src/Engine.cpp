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

Engine::Engine()
{
	auto& patches = ElDorito::Instance().Patches;

	patches.TogglePatchSet(patches.AddPatchSet("Engine", {},
	{
		Hook("GameTick", 0x505E64, GameTickHook, HookType::Call),
		Hook("TagsLoaded", 0x5030EA, TagsLoadedHook, HookType::Jmp)
	}));
}

bool Engine::OnTick(TickCallbackFunc callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

bool Engine::OnEvent(EngineEvent evt, EventCallbackFunc callback)
{
	eventCallbacks.insert(std::pair<EventCallbackFunc, EngineEvent>(callback, evt));
	return true; // todo: check if this callback is already registered
}

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

void Engine::MainMenuShown()
{
	if (this->mainMenuHasShown)
		return; // this event should only occur once during the lifecycle of the game

	this->mainMenuHasShown = true;
	this->Event(EngineEvent::MainMenuShown);
}

void Engine::Event(EngineEvent evt, void* param)
{
	ElDorito::Instance().Logger.Log(LogLevel::Debug, "EngineEvent", "%d", evt);

	for (auto kvp : eventCallbacks)
	{
		if (kvp.second != evt)
			continue;
		kvp.first(param);
	}
}