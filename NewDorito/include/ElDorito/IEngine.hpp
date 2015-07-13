#pragma once
#include <chrono>

/*
if you want to make changes to this interface create a new IEngine002 class and make them there, then edit Engine class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

typedef bool(__cdecl* TickCallbackFunc)(const std::chrono::duration<double>& deltaTime);

class IEngine001
{
public:
	virtual bool RegisterTickCallback(TickCallbackFunc callback) = 0;
};

#define ENGINE_INTERFACE_VERSION001 "Engine001"
