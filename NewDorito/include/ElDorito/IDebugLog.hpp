#pragma once
#include <string>

/*
if you want to make changes to this interface create a new IDebugLog002 class and make them there, then edit DebugLog class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

enum class LogLevel
{
	Debug,
	Info,
	Warning,
	Error,
	Fatal
};

class IDebugLog001
{
public:
	virtual void Log(LogLevel level, std::string module, std::string format, ...) = 0;
};

#define DEBUGLOG_INTERFACE_VERSION001 "DebugLog001"
