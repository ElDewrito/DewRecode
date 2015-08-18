#include "ModuleMenu.hpp"

Modules::ModuleMenu Menu;

namespace
{
	bool menuShouldShow = false;
	bool menuShouldInit = false;

	void OnEndScene(void* param)
	{
		Menu.Draw(reinterpret_cast<IDirect3DDevice9*>(param));
	}

	void OnMainMenuShown(void* param)
	{
		menuShouldInit = true;
	}

	bool CommandMenuShow(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		menuShouldShow = !menuShouldShow;
		return true;
	}

	LRESULT __stdcall MenuWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return Menu.WndProc(hWnd, msg, wParam, lParam);
	}
}

namespace Modules
{
	ModuleMenu::ModuleMenu() : ModuleBase("Menu"), texture(0)
	{
		engine->OnEvent("Core", "Direct3D.EndScene", OnEndScene);
		engine->OnEvent("Core", "Engine.MainMenuShown", OnMainMenuShown);
		engine->OnWndProc(MenuWndProc);
		VarMenuURL = AddVariableString("URL", "menu_url", "The URL for the menu", eCommandFlagsArchived, "http://dewmenu.halo.click/");
		AddCommand("Show", "menu_show", "Starts drawing the menu", eCommandFlagsNone, CommandMenuShow);
	}

	ModuleMenu::~ModuleMenu()
	{
		if (!browser)
			return;
		CefShutdown();
	}

	int lastPosX = 0;
	int lastPosY = 0;

	void ModuleMenu::Draw(IDirect3DDevice9* device)
	{
		if (menuShouldInit && !inited)
			Initialize(device);

		if (!browser)
			return;

		if (!menuShouldShow)
			return;

		sprite->Begin(D3DXSPRITE_ALPHABLEND);
		sprite->Draw(texture, NULL, NULL, &D3DXVECTOR3(0, 0, 0), 0xFFFFFFFF);
		sprite->Flush();
		sprite->End();
	}

	void ModuleMenu::Initialize(IDirect3DDevice9* device)
	{
		this->device = device;
		logger->Log(LogSeverity::Debug, "ModuleMenu", "Initing CEF...");

		if (!texture)
		{
			auto res = engine->GetGameResolution();
			device->CreateTexture(res.first, res.second, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
		}

		if (!sprite)
			D3DXCreateSprite(device, &sprite);

		// Start rendering process
		CefMainArgs mainArgs;
		int exitCode = CefExecuteProcess(mainArgs, NULL, NULL);

		void* sandboxInfo = NULL;
#if CEF_ENABLE_SANDBOX
		CefScopedSandboxInfo scopedSandbox;
		sandboxInfo = scopedSandbox.sandbox_info();
#endif

		// Initialize CEF
		if (exitCode >= 0)
		{
			logger->Log(LogSeverity::Debug, "ModuleMenu", "Failed to load CEF process with exit code %d...", exitCode);
			initFailed = true;
			return;
		}

		CefSettings settings;
#if !CEF_ENABLE_SANDBOX
		settings.no_sandbox = true;
#endif

		settings.single_process = true;
		//settings.log_severity = LOGSEVERITY_VERBOSE;
		settings.multi_threaded_message_loop = true;

		logger->Log(LogSeverity::Debug, "ModuleMenu", "Initing browser...");
		bool init = CefInitialize(mainArgs, settings, NULL, sandboxInfo);

		CefBrowserSettings browserSettings;
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(engine->GetGameHWND(), true); // Enable offscreen rendering

		wchar_t currentDir[256];
		memset(currentDir, 0, 256 * sizeof(wchar_t));
		GetCurrentDirectoryW(256, currentDir);

		swprintf_s(currentDir, 256, L"%ls\\mods\\menus\\temp\\", currentDir);

		std::wstring path(currentDir);

		CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager(NULL);
		manager->SetStoragePath(path, true, NULL);

		CefBrowserHost::CreateBrowser(windowInfo, this, VarMenuURL->ValueString.c_str(), browserSettings, NULL);

		inited = true;
	}
	
	LRESULT ModuleMenu::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!menuShouldShow)
			return 0;
		if (msg != WM_LBUTTONDOWN && msg != WM_LBUTTONUP && msg != WM_MOUSEMOVE)
			return 0;

		POINT p;
		if (!GetCursorPos(&p) || !ScreenToClient(engine->GetGameHWND(), &p))
			return 0;

		CefMouseEvent mouse_event;
		mouse_event.x = p.x;
		mouse_event.y = p.y;

		if (msg == WM_MOUSEMOVE)
			browser->GetHost()->SendMouseMoveEvent(mouse_event, false);
		else
			browser->GetHost()->SendMouseClickEvent(mouse_event, MBT_LEFT, msg == WM_LBUTTONUP, 1);
		return 1;
	}

	void ModuleMenu::OnAfterCreated(CefRefPtr<CefBrowser> browser)
	{
		this->browser = browser;
		CefLifeSpanHandler::OnAfterCreated(browser);
	}

	CefRefPtr<CefLoadHandler> ModuleMenu::GetLoadHandler()
	{
		// Well, we're a CefLoadHandler
		return this;
	}

	CefRefPtr<CefRenderHandler> ModuleMenu::GetRenderHandler()
	{
		// Well, we're a CefRenderHandler
		return this;
	}

	CefRefPtr<CefLifeSpanHandler> ModuleMenu::GetLifeSpanHandler()
	{
		// Well, we're a CefLifeSpanHandler
		return this;
	}

	void ModuleMenu::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
	{

	}

	void ModuleMenu::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
	{

	}

	void ModuleMenu::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
	{
		// Don't display an error for downloaded files.
		if (errorCode == ERR_ABORTED)
			return;

		// Display a load error message.
		std::stringstream ss;
		ss << "<html><body bgcolor=\"white\">"
			"<h2>Failed to load URL " << std::string(failedUrl) <<
			" with error " << std::string(errorText) << " (" << errorCode <<
			").</h2></body></html>";
		frame->LoadString(ss.str(), failedUrl);
	}

	bool ModuleMenu::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
	{
		auto res = engine->GetGameResolution();
		rect.x = 0;
		rect.y = 0;
		rect.width = res.first;
		rect.height = res.second;
		return true;
	}

	void ModuleMenu::OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType paintType, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int width, int height)
	{
		D3DLOCKED_RECT lr;
		texture->LockRect(0, &lr, nullptr, 0);
		memcpy(lr.pBits, buffer, width * height * 4);
		texture->UnlockRect(0);
	}
}