#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>

// handles game events and callbacks for different modules/plugins
class Engine : public IEngine001
{
public:
	// used to register callbacks for these events
	bool OnTick(TickCallbackFunc callback);
	bool OnEvent(EngineEvent evt, EventCallbackFunc callback);

	// called when an event occurs, calls each registered callback for the event
	void Tick(const std::chrono::duration<double>& deltaTime);
	void MainMenuShown();
	void Event(EngineEvent evt, void* param = 0);

	bool HasMainMenuShown()
	{
		return mainMenuHasShown;
	}

	Engine();
private:
	bool mainMenuHasShown = false;
	bool hasFirstTickTocked = false;
	std::vector<TickCallbackFunc> tickCallbacks;
	std::map<EventCallbackFunc, EngineEvent> eventCallbacks;
};
