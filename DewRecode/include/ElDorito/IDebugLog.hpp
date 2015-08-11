#pragma once
#include <string>

/*
if you want to make changes to this interface create a new IDebugLog002 class and make them there, then edit DebugLog class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

enum class LogSeverity
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
	/// <summary>
	/// Logs a line to the log file.
	/// </summary>
	/// <param name="severity">The severity of the message.</param>
	/// <param name="module">The module the message originated from.</param>
	/// <param name="format">The format of the message.</param>
	/// <param name="">Additional formatting.</param>
	virtual void Log(LogSeverity severity, const std::string& module, std::string format, ...) = 0;
};

#define DEBUGLOG_INTERFACE_VERSION001 "DebugLog001"

/* use this class if you're updating IDebugLog after we've released a build
also update the IDebugLog typedef and DEBUGLOG_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class IDebugLog002 : public IDebugLog001
{

};

#define DEBUGLOG_INTERFACE_VERSION002 "DebugLog002"*/

typedef IDebugLog001 IDebugLog;
#define DEBUGLOG_INTERFACE_LATEST DEBUGLOG_INTERFACE_VERSION001
