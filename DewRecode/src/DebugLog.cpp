#include "DebugLog.hpp"

#include <cstdarg>
#include <fstream>
#include <time.h>
#include <iomanip>

#include "ElDorito.hpp"

/// <summary>
/// Logs a line to the log file.
/// </summary>
/// <param name="level">The severity of the message.</param>
/// <param name="module">The module the message originated from.</param>
/// <param name="format">The format of the message.</param>
/// <param name="">Additional formatting.</param>
void DebugLog::Log(LogSeverity severity, std::string module, std::string format, ...)
{
	// TODO1: LogSeverity

	if (module.length() + format.length() > 4096)
		return;

	va_list ap;
	va_start(ap, format);

	char buff[4096];
	vsprintf_s(buff, 4096, format.c_str(), ap);
	va_end(ap);

	auto& gameModule = ElDorito::Instance().Modules.Game;
	for (auto filter : gameModule.FiltersExclude)
		if (strstr(buff, filter.c_str()) != NULL)
			return; // string contains an excluded string

	for (auto filter : gameModule.FiltersInclude)
		if (strstr(buff, filter.c_str()) == NULL)
			return; // string doesn't contain an included string

	std::ofstream outfile;
	outfile.open(gameModule.VarLogName->ValueString, std::ios_base::app);
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