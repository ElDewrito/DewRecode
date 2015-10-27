#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include "imgui/imgui.h"
#include <ElDorito/ElDorito.hpp>
#include "Windows/ChatWindow.hpp"
#include "Windows/ConsoleWindow.hpp"

struct CUSTOMVERTEX
{
	D3DXVECTOR3 pos;
	D3DCOLOR    col;
	D3DXVECTOR2 uv;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

namespace UI
{
	// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
	class UserInterface : public IUserInterface
	{
		HWND hwnd;
		IDirect3DDevice9* device = nullptr;
		IDirect3DVertexBuffer9* vertexBuf = nullptr;
		IDirect3DIndexBuffer9* indexBuf = nullptr;
		IDirect3DTexture9* fontTexture = nullptr;
		int vertexBufferSize = 5000;
		int indexBufferSize = 10000;
		uint64_t time = 0;
		uint64_t ticksPerSecond = 0;
		std::vector<std::shared_ptr<UIWindow>> windows;
		bool uiShown = false;
		std::shared_ptr<ChatWindow> chat;
		std::shared_ptr<ConsoleWindow> console;
	
		bool createFontsTexture();
		bool createDeviceObjects();
		void invalidateDeviceObjects();
		void newFrame();
		void renderDrawLists(ImDrawData* draw_data);
		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void EndScene(void* param);
		void Init(void* param);
		void Shutdown();

		bool keyDownCallBack(const Blam::Input::KeyEvent& key);

	public:
		bool IsShown() { return uiShown; }
		bool ShowUI(bool show);
		bool ShowChat(bool show);
		bool ShowConsole(bool show);
		bool ShowMessageBox(const std::string& title, const std::string& message, std::vector<std::string> choices, MsgBoxCallback callback);
		void WriteToConsole(const std::string& text);
		void AddToChat(const std::string& text, bool globalChat);

		UserInterface();
	};
}