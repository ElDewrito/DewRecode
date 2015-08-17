#include "ModuleMenu.hpp"

Modules::ModuleMenu Menu;

namespace
{
	bool shouldShow = false;
	bool shouldInit = false;

	void OnEndScene(void* param)
	{
		Menu.Draw(reinterpret_cast<IDirect3DDevice9*>(param));
	}

	void OnMainMenuShown(void* param)
	{
		shouldInit = true;
	}

	bool CommandMenuShow(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		shouldShow = !shouldShow;
		return true;
	}
}

namespace Modules
{
	ModuleMenu::ModuleMenu() : ModuleBase("Menu"), texture(0)
	{
		engine->OnEvent("Core", "Direct3D.EndScene", OnEndScene);
		engine->OnEvent("Core", "Engine.MainMenuShown", OnMainMenuShown);
		VarMenuURL = AddVariableString("URL", "menu_url", "The URL for the menu", eCommandFlagsArchived, "https://google.com/");
		AddCommand("Show", "menu_show", "Starts drawing the menu", eCommandFlagsNone, CommandMenuShow);
	}

	void ModuleMenu::Draw(IDirect3DDevice9* device)
	{
		if (initFailed)
			return;

		if (!browser && shouldInit)
			Initialize(device);

		if (!shouldShow)
			return;

		POINT p;
		if (GetCursorPos(&p))
		{
			if (ScreenToClient(engine->GetGameHWND(), &p))
			{
				CefMouseEvent mouse_event;
				mouse_event.x = p.x;
				mouse_event.y = p.y;
				browser->GetHost()->SendMouseMoveEvent(mouse_event, false);
			}
		}

		CefDoMessageLoopWork();

		device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0),
			1.0f, 0);

		if (!sprite)
			D3DXCreateSprite(device, &sprite);

		sprite->Begin(D3DXSPRITE_ALPHABLEND);
		sprite->Draw(texture, NULL, NULL, &D3DXVECTOR3(0, 0, 0), 0xFFFFFFFF);
		sprite->Flush();
		sprite->End();
	}

	void ModuleMenu::Initialize(IDirect3DDevice9* device)
	{
		logger->Log(LogSeverity::Debug, "ModuleMenu", "Initing CEF...");
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
		//settings.multi_threaded_message_loop = true;

		logger->Log(LogSeverity::Debug, "ModuleMenu", "Initing browser...");
		bool init = CefInitialize(mainArgs, settings, NULL, sandboxInfo);

		CefBrowserSettings browserSettings;
		CefWindowInfo windowInfo;
		windowInfo.SetAsWindowless(engine->GetGameHWND(), true); // Enable offscreen rendering

		browser = CefBrowserHost::CreateBrowserSync(windowInfo, this, VarMenuURL->ValueString.c_str(), browserSettings, NULL);

		auto res = engine->GetGameResolution();
		if (!texture)
			device->CreateTexture(res.first, res.second, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
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

	void ModuleMenu::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame)
	{

	}

	void ModuleMenu::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
	{

	}

	void ModuleMenu::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
	{
		// Todo: Add a Lua event
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
		if (!texture)
			return;

		D3DSURFACE_DESC desc;
		texture->GetLevelDesc(0, &desc);

		D3DLOCKED_RECT lr;
		texture->LockRect(0, &lr, nullptr, 0);
		memcpy(lr.pBits, buffer, width * height * 4);
		texture->UnlockRect(0);
	}
}