#pragma once
#include <ElDorito/ElDorito.hpp>
#include <map>

// handles game events and callbacks for different modules/plugins
// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class Engine : public IEngine001
{
public:
	bool OnTick(TickCallbackFunc callback);
	bool OnEvent(std::string eventNamespace, std::string eventName, EventCallbackFunc callback);

	void Event(std::string eventNamespace, std::string eventName, void* param = 0);

	bool RegisterInterface(std::string interfaceName, void* ptrToInterface);
	void* CreateInterface(std::string interfaceName, int* returnCode);

	bool HasMainMenuShown() { return mainMenuHasShown; }

	HWND GetGameHWND() { return Pointer(0x199C014).Read<HWND>(); }
	Pointer GetMainTls(size_t offset = 0);
	size_t GetMainThreadID() { return mainThreadID; }
	void SetMainThreadID(size_t threadID) { mainThreadID = threadID; }

	HMODULE GetDoritoModule() { return doritoModule; }
	void SetDoritoModule(HMODULE module) { doritoModule = module; }

	// not exposed over IEngine interface
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
