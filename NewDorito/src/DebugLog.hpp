#pragma once
#include <ElDorito/IDebugLog.hpp>

// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class DebugLog : public IDebugLog001
{
public:
	void Log(LogLevel level, std::string module, std::string format, ...);
};