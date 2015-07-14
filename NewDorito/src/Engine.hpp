#pragma once
#include <ElDorito/ElDorito.hpp>

// handles game events and callbacks for different modules/plugins
class Engine : public IEngine001
{
public:
	// used to register callbacks for these events
	bool OnTick(TickCallbackFunc callback);
	bool OnMainMenuShown(EngineCallbackFunc callback);
	bool OnTagsLoaded(EngineCallbackFunc callback);

	// called when an event occurs, calls each registered callback for the event
	void Tick(const std::chrono::duration<double>& deltaTime);
	void MainMenuShown();
	void TagsLoaded();

	bool HasMainMenuShown()
	{
		return mainMenuHasShown;
	}

	Engine();
private:
	bool mainMenuHasShown = false;
	std::vector<TickCallbackFunc> tickCallbacks;
	std::vector<EngineCallbackFunc> mainMenuShownCallbacks;
	std::vector<EngineCallbackFunc> tagsLoadedCallbacks;
};
