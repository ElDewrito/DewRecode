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
}

Engine::Engine()
{
	auto& patches = ElDorito::Instance().Patches;

	patches.TogglePatchSet(patches.AddPatchSet("Engine", {},
	{
		{ "GameTick", 0x505E64, GameTickHook, HookType::Call, {}, false }
	}));
}

bool Engine::OnTick(TickCallbackFunc callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

bool Engine::OnMainMenuShown(MainMenuShownCallbackFunc callback)
{
	mainMenuShownCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

void Engine::Tick(const std::chrono::duration<double>& deltaTime)
{
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