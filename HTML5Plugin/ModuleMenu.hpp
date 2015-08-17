#pragma once
#include <ElDorito/ElDorito.hpp>
#include "include/cef_app.h"
#include "include/cef_client.h"
#include "include/cef_render_handler.h"
#include <d3d9.h>
#include <d3dx9.h>

namespace Modules
{
	class ModuleMenu : public ModuleBase, public CefClient, public CefLoadHandler, public CefRenderHandler
	{
	private:
		CefRefPtr<CefBrowser> browser;
		IDirect3DTexture9* texture;
		ID3DXSprite* sprite;
		bool initFailed = false;
	public:
		Command* VarMenuURL;

		void Draw(IDirect3DDevice9* device);
		void Initialize(IDirect3DDevice9* device);

		// CefClient methods
		virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override;
		virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override;

		// CefLoadHandler methods
		virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) override;
		virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
		virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override;

		// CefRenderHandler methods
		virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
		virtual void OnPaint(CefRefPtr<CefBrowser> browser, CefRenderHandler::PaintElementType paintType, const CefRenderHandler::RectList& dirtyRects, const void* buffer, int width, int height) override;

		ModuleMenu();

		// Implement smartpointer methods (all Cef-classes require that since they are derived from CefBase)
		IMPLEMENT_REFCOUNTING(ModuleMenu);
	};
}
