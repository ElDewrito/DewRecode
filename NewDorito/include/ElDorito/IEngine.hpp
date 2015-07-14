#pragma once
#include <chrono>

enum class EngineEvent
{
	FirstTick,
	MainMenuShown,
	TagsLoaded,
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
	virtual bool OnTick(TickCallbackFunc callback) = 0;
	virtual bool OnEvent(EngineEvent evt, EventCallbackFunc callback) = 0;

	virtual void Tick(const std::chrono::duration<double>& deltaTime) = 0;
	virtual void MainMenuShown() = 0;
	virtual void Event(EngineEvent evt, void* param = 0) = 0;

	virtual bool HasMainMenuShown() = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"
