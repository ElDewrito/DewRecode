#pragma once

// STL
#include <string>
#include <vector>

// Windows
#include <Windows.h>

namespace Utils
{
	namespace Version
	{
		DWORD GetVersionInt();
		std::string GetVersionString();
		std::string GetInfo(const std::string &csEntry);
	}
}