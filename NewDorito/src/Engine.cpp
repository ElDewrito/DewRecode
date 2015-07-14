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

	void TagsLoadedHook()
	{
		ElDorito::Instance().Engine.TagsLoaded();
	}

	__declspec(naked) void TagsLoadedHookAsm()
	{
		__asm
		{
			call TagsLoadedHook
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
		Hook("TagsLoaded", 0x5030EA, TagsLoadedHookAsm, HookType::Jmp)
	}));
}

bool Engine::OnTick(TickCallbackFunc callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

bool Engine::OnFirstTick(EngineCallbackFunc callback)
{
	firstTickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

bool Engine::OnMainMenuShown(EngineCallbackFunc callback)
{
	mainMenuShownCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

bool Engine::OnTagsLoaded(EngineCallbackFunc callback)
{
	tagsLoadedCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

void Engine::Tick(const std::chrono::duration<double>& deltaTime)
{
	if (!hasFirstTickTocked)
	{
		for (auto callback : firstTickCallbacks)
			callback();

		hasFirstTickTocked = true;
	}
	for (auto callback : tickCallbacks)
		callback(deltaTime);
}

void Engine::MainMenuShown()
{
	if (this->mainMenuHasShown)
		return; // this event should only occur once during the lifecycle of the game

	this->mainMenuHasShown = true;

	for (auto callback : mainMenuShownCallbacks)
		callback();
}

void Engine::TagsLoaded()
{
	for (auto callback : tagsLoadedCallbacks)
		callback();
}