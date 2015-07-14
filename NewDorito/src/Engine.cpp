#include "Engine.hpp"

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