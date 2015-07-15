#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>

// handles game events and callbacks for different modules/plugins
// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class Engine : public IEngine001
{
public:
	// registers a callback which is called when the game ticks
	bool OnTick(TickCallbackFunc callback);

	// you can use any eventModule/eventName here, the callback will belong to this combination
	// and calling Engine::Event with the same eventModule/eventName will call each of the registered callbacks for this event
	// (in essense this not only registers callbacks for events but also registers events too)
	// the only restricted eventModule is "Core", this module is reserved for events created by ElDorito
	// in case your wondering, eventModule and eventName are seperate so that plugin authors have to provide a module name for their event, making it "unique"
	bool OnEvent(std::string eventModule, std::string eventName, EventCallbackFunc callback);

	// called when an event occurs, calls each registered callback for the event
	void Event(std::string eventModule, std::string eventName, void* param = 0);

	// registers an interface, plugins can use this to share classes across plugins
	bool RegisterInterface(std::string interfaceName, void* ptrToInterface);

	// returns the interface by the interface name
	void* CreateInterface(std::string interfaceName, int* returnCode);

	bool HasMainMenuShown() { return mainMenuHasShown; }

	HWND GetGameHWND() { return Pointer(0x199C014).Read<HWND>(); }
	Pointer GetMainTls(size_t offset = 0);
	size_t GetMainThreadID() { return mainThreadID; }
	void SetMainThreadID(size_t threadID) { mainThreadID = threadID; }

	HMODULE GetDoritoModule() { return doritoModule; }
	void SetDoritoModule(HMODULE module) { doritoModule = module; }

	// not exposed with IEngine interface
	void Tick(const std::chrono::duration<double>& deltaTime);

	Engine();
private:
	HMODULE doritoModule;
	size_t mainThreadID;
	bool mainMenuHasShown = false;
	bool hasFirstTickTocked = false;
	std::vector<TickCallbackFunc> tickCallbacks;
	std::map<std::string, std::vector<EventCallbackFunc>> eventCallbacks;
	std::map<std::string, void*> interfaces;
};
