#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>

// handles game events and callbacks for different modules/plugins
// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class Engine : public IEngine001
{
public:
	Pointer GetMainTls(size_t offset = 0);
	HWND GetHWND()
	{
		return Pointer(0x199C014).Read<HWND>();
	}

	HMODULE GetDoritoModule()
	{
		return doritoModule;
	}
	void SetDoritoModule(HMODULE module)
	{
		doritoModule = module;
	}

	size_t GetMainThreadID()
	{
		return mainThreadID;
	}
	void SetMainThreadID(size_t threadID)
	{
		mainThreadID = threadID;
	}

	bool HasMainMenuShown()
	{
		return mainMenuHasShown;
	}

	// used to register callbacks for these events
	bool OnTick(TickCallbackFunc callback);
	bool OnEvent(EngineEvent evt, EventCallbackFunc callback);

	// called when an event occurs, calls each registered callback for the event
	void Tick(const std::chrono::duration<double>& deltaTime);
	void Event(EngineEvent evt, void* param = 0);
	void MainMenuShown();

	Engine();
private:
	HMODULE doritoModule;
	size_t mainThreadID;
	bool mainMenuHasShown = false;
	bool hasFirstTickTocked = false;
	std::vector<TickCallbackFunc> tickCallbacks;
	std::map<EventCallbackFunc, EngineEvent> eventCallbacks;
};
