#pragma once
#include <ElDorito/IDebugLog.hpp>

class DebugLog : public IDebugLog001
{
public:
	void Log(DebugLogSeverity severity, std::string module, std::string format, ...);

	DebugLog();
};