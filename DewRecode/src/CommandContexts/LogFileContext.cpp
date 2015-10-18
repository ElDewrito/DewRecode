#include "LogFileContext.hpp"
#include "../ElDorito.hpp"

void LogFileContext::HandleInput(const std::string& input)
{
	// logfile shouldn't have any input
}

void LogFileContext::WriteOutput(const std::string& output)
{
	ElDorito::Instance().Logger.Log(LogSeverity::Info, "Console", output);
}