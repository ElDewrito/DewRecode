#include "VersionInfo.hpp"
#include <windows.h>
#include "../ElDorito.hpp"

namespace Utils
{
	namespace Version
	{
		DWORD GetVersionInt()
		{
			DWORD retVer = 0;

			HMODULE module = ElDorito::Instance().Engine.GetDoritoModule();

			HRSRC hVersion = FindResource(module,
				MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
			if (!hVersion)
				return retVer;

			HGLOBAL hGlobal = LoadResource(module, hVersion);
			if (!hGlobal)
				return retVer;

			LPVOID versionInfo = LockResource(hGlobal);
			if (!versionInfo)
				return retVer;

			DWORD versionSize = SizeofResource(module, hVersion);
			LPVOID versionCopy = LocalAlloc(LMEM_FIXED, versionSize);
			if (!versionCopy)
				return retVer;

			CopyMemory(versionCopy, versionInfo, versionSize);
			FreeResource(versionInfo);

			DWORD vLen;
			BOOL retVal;

			LPVOID retbuf = NULL;

			retVal = VerQueryValue(versionCopy, L"\\", &retbuf, (UINT *)&vLen);
			if (retVal && vLen == sizeof(VS_FIXEDFILEINFO))
			{
				VS_FIXEDFILEINFO* ffInfo = (VS_FIXEDFILEINFO*)retbuf;
				DWORD majorVer = HIWORD(ffInfo->dwFileVersionMS);
				DWORD minorVer = LOWORD(ffInfo->dwFileVersionMS);
				DWORD buildNum = HIWORD(ffInfo->dwFileVersionLS);
				DWORD buildQfe = LOWORD(ffInfo->dwFileVersionLS);

				retVer =
					((majorVer & 0xFF) << 24) |
					((minorVer & 0xFF) << 16) |
					((buildNum & 0xFF) << 8) |
					(buildQfe & 0xFF);
			}

			LocalFree(versionCopy);

			return retVer;
		}

		std::string GetVersionString()
		{
			static std::string versionStr;

			if (versionStr.empty())
			{
				char version[256];
				memset(version, 0, 256);
				DWORD versionInt = GetVersionInt();
				sprintf_s(version, 256, "%d.%d.%d.%d", ((versionInt >> 24) & 0xFF), ((versionInt >> 16) & 0xFF), ((versionInt >> 8) & 0xFF), (versionInt & 0xFF));
#ifdef _DEBUG
				versionStr = std::string(version) + "-debug";
#else
				versionStr = std::string(version);
#endif
			}
			return versionStr;
		}

		std::string GetInfo(const std::string &csEntry)
		{
			std::string csRet;

			HMODULE module = ElDorito::Instance().Engine.GetDoritoModule();

			HRSRC hVersion = FindResource(module,
				MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
			if (!hVersion)
				return csRet;

			HGLOBAL hGlobal = LoadResource(module, hVersion);
			if (!hGlobal)
				return csRet;

			LPVOID versionInfo = LockResource(hGlobal);
			if (!versionInfo)
				return csRet;

			DWORD versionSize = SizeofResource(module, hVersion);
			LPVOID versionCopy = LocalAlloc(LMEM_FIXED, versionSize);
			if (!versionCopy)
				return csRet;

			CopyMemory(versionCopy, versionInfo, versionSize);
			FreeResource(versionInfo);

			DWORD vLen, langD;
			BOOL retVal;

			LPVOID retbuf = NULL;

			static char fileEntry[256];

			retVal = VerQueryValue(versionCopy, L"\\VarFileInfo\\Translation", &retbuf, (UINT *)&vLen);
			if (retVal && vLen == 4)
			{
				memcpy(&langD, retbuf, 4);
				sprintf_s(fileEntry, "\\StringFileInfo\\%02X%02X%02X%02X\\%s",
					(langD & 0xff00) >> 8, langD & 0xff, (langD & 0xff000000) >> 24,
					(langD & 0xff0000) >> 16, csEntry.c_str());
			}
			else
			{
				unsigned long lang = GetUserDefaultLangID();
				sprintf_s(fileEntry, "\\StringFileInfo\\%04X04B0\\%s", lang, csEntry.c_str());
			}

			if (VerQueryValueA(versionCopy, fileEntry, &retbuf, (UINT *)&vLen))
			{
				strcpy_s(fileEntry, (const char*)retbuf);
				csRet = fileEntry;
			}

			LocalFree(versionCopy);

			return csRet;
		}
	}
}