#include "DebugLog.hpp"

#include <cstdarg>
#include <fstream>
#include <time.h>
#include <iomanip>

#include "ElDorito.hpp"

std::string startTime;

/// <summary>
/// Logs a line to the log file.
/// </summary>
/// <param name="level">The severity of the message.</param>
/// <param name="module">The module the message originated from.</param>
/// <param name="format">The format of the message.</param>
/// <param name="">Additional formatting.</param>
void DebugLog::Log(LogSeverity severity, const std::string& module, std::string format, ...)
{
	// TODO1: LogSeverity

	if (format.length() > 4096)
		return;

	if (startTime.empty())
	{

		std::time_t rawtime;
		std::tm timeinfo;
		char buffer[80];

		std::time(&rawtime);
		//timeinfo = std::localtime(&rawtime);
		localtime_s(&timeinfo, &rawtime);

		std::strftime(buffer, 80, "%Y%m%d_%H%M%S", &timeinfo);

		startTime = std::string(buffer);
	}


	va_list ap;
	va_start(ap, format);

	char buff[4096];
	vsprintf_s(buff, 4096, format.c_str(), ap);
	va_end(ap);

	auto debugCommands = ElDorito::Instance().DebugCommands;
	std::string outFileName = startTime + "_startup.log";

	if (debugCommands)
	{
		for (auto filter : debugCommands->FiltersExclude)
			if (strstr(buff, filter.c_str()) != NULL)
				return; // string contains an excluded string

		for (auto filter : debugCommands->FiltersInclude)
			if (strstr(buff, filter.c_str()) == NULL)
				return; // string doesn't contain an included string

		_wmkdir(L"logs");

		outFileName = "logs\\" + startTime + "_" + debugCommands->VarLogName->ValueString;
	}

	std::ofstream outfile;
	outfile.open(outFileName, std::ios_base::app);
	if (outfile.fail())
		return; // TODO: give output if the log stuff failed

	time_t t = time(NULL);
	tm ourLocalTime;
	if (localtime_s(&ourLocalTime, &t) != 0)
	{
		outfile.close(); // TODO: give output that localtime failed (when will it ever fail tho?)
		return;
	}

	outfile << '[' << std::put_time(&ourLocalTime, "%H:%M:%S") << "] " << module << " - " << buff << '\n';
	outfile.close();
}