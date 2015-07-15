#pragma once
#include "Pointer.hpp"
#include <chrono>

enum class EngineEvent
{
	FirstTick,
	MainMenuShown,
	TagsLoaded,
	KeyboardUpdate,

	PluginCustomEvent1,
	PluginCustomEvent2,
	PluginCustomEvent3,
	PluginCustomEvent4,
	PluginCustomEvent5,
	PluginCustomEvent6,
	PluginCustomEvent7,
	PluginCustomEvent8,
	PluginCustomEvent9,
	PluginCustomEvent10,

	PluginCustomEvent11,
	PluginCustomEvent12,
	PluginCustomEvent13,
	PluginCustomEvent14,
	PluginCustomEvent15,
	PluginCustomEvent16,
	PluginCustomEvent17,
	PluginCustomEvent18,
	PluginCustomEvent19,
	PluginCustomEvent20,

	PluginCustomEvent21,
	PluginCustomEvent22,
	PluginCustomEvent23,
	PluginCustomEvent24,
	PluginCustomEvent25,
	PluginCustomEvent26,
	PluginCustomEvent27,
	PluginCustomEvent28,
	PluginCustomEvent29,
	PluginCustomEvent30,
	PluginCustomEvent31,
	PluginCustomEvent32,

	Count
};

typedef void(__cdecl* TickCallbackFunc)(const std::chrono::duration<double>& deltaTime);
typedef void(__cdecl* EventCallbackFunc)(void* param);

/*
if you want to make changes to this interface create a new IEngine002 class and make them there, then edit Engine class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IEngine001
{
public:
	// used to register callbacks for these events
	virtual bool OnTick(TickCallbackFunc callback) = 0;
	virtual bool OnEvent(EngineEvent evt, EventCallbackFunc callback) = 0;

	// called when an event occurs, calls each registered callback for the event
	virtual void Event(EngineEvent evt, void* param = 0) = 0;

	// registers an interface, plugins can use this to share classes across plugins
	virtual bool RegisterInterface(std::string interfaceName, void* ptrToInterface) = 0;

	/// <summary>
	/// Creates an interface to an exported class.
	/// </summary>
	/// <param name="name">The name of the interface.</param>
	/// <param name="returnCode">0 if the interface was found successfully, otherwise the error code.</param>
	/// <returns>The requested interface, if found.</returns>
	virtual void* CreateInterface(std::string interfaceName, int* returnCode) = 0;

	// returns an EngineEvent that is unique to the plugin and can be used for custom plugin events
	// eg. plugin uses this to create an event, registers a callback with OnEvent and makes a hook that calls Event(returnedEvent) when a player is stuck with a plasma grenade
	// it'd probably be better to just call the callback directly in that case, but using EngineEvents can let you share your events with other plugins (via interfaces)
	virtual EngineEvent RegisterCustomEvent() = 0;

	virtual bool HasMainMenuShown() = 0;

	virtual HWND GetGameHWND() = 0;
	virtual Pointer GetMainTls(size_t offset = 0) = 0;
	virtual size_t GetMainThreadID() = 0;
	virtual void SetMainThreadID(size_t threadID) = 0;

	virtual HMODULE GetDoritoModule() = 0;
	virtual void SetDoritoModule(HMODULE module) = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"
