#pragma once
#include <ElDorito/ElDorito.hpp>

#define WM_RCON WM_USER + 1337
#define WM_INFOSERVER WM_RCON + 1

namespace Modules
{
	class PatchModuleServer : public ModuleBase
	{
	public:
		PatchModuleServer();

		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void RemoteConsoleStart();

		void InfoServerStart();
		void InfoServerStop();

	private:
		SOCKET rconSocket;
		bool rconSocketOpen = false;

		SOCKET infoSocket;
		bool infoSocketOpen = false;

		time_t lastAnnounce = 0;
		const time_t serverContactTimeLimit = 30 + (2 * 60);
	};
}
