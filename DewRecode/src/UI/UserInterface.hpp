#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include "imgui/imgui.h"
#include <ElDorito/ElDorito.hpp>
#include "ConsoleWindow.hpp"

struct CUSTOMVERTEX
{
	D3DXVECTOR3 pos;
	D3DCOLOR    col;
	D3DXVECTOR2 uv;
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

namespace UI
{
	class UserInterface
	{
	private:
		IEngine* engine = nullptr;
		HWND hwnd;
		IDirect3DDevice9* device = nullptr;
		IDirect3DVertexBuffer9* vertexBuf = nullptr;
		IDirect3DIndexBuffer9* indexBuf = nullptr;
		IDirect3DTexture9* fontTexture = nullptr;
		int vertexBufferSize = 5000;
		int indexBufferSize = 10000;
		uint64_t time = 0;
		uint64_t ticksPerSecond = 0;

		bool createDeviceObjects();
		bool createFontsTexture();
		void invalidateDeviceObjects();
		void newFrame();
		void renderDrawLists(ImDrawData* draw_data);
		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void EndScene(void* param);

	public:
		std::shared_ptr<UI::ConsoleWindow> Console;

		UserInterface(IEngine* engine);
		void Init(void* param);
		void Shutdown();
	};
}