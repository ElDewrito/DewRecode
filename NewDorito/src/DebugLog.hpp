#pragma once
#include <ElDorito/IDebugLog.hpp>

class DebugLog : public IDebugLog001
{
public:
	void Log(LogLevel level, std::string module, std::string format, ...);
};