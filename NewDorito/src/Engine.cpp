#include "Engine.hpp"

bool Engine::RegisterTickCallback(TickCallbackFunc callback)
{
	tickCallbacks.push_back(callback);
	return true; // todo: check if this callback is already registered
}

void Engine::Tick(const std::chrono::duration<double>& deltaTime)
{
	for (auto callback : tickCallbacks)
		callback(deltaTime);
}