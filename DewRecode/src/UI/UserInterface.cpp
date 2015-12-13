#include "UserInterface.hpp"
#include "../ElDorito.hpp"
#include <mutex>

std::mutex m;

namespace UI
{
	class UIInputContext : public InputContext
	{
	public:
		explicit UIInputContext(UserInterface* ui) : ui(ui) {}

		virtual void InputActivated() override
		{
			// TODO: Allow input contexts to block input more easily without
			// potentially overriding blocks the game has set

			// Block UI input
			ElDorito::Instance().Engine.BlockInput(Blam::Input::eInputTypeUi, true);
		}

		virtual void InputDeactivated() override
		{
			// Unblock UI input
			ElDorito::Instance().Engine.BlockInput(Blam::Input::eInputTypeUi, false);
		}

		virtual bool GameInputTick() override
		{
			// The "done" state is delayed a tick in order to prevent input
			// replication, since the UI can get new keys before the game does
			return done;
		}

		virtual bool UiInputTick() override
		{
			auto& dorito = ElDorito::Instance();
			dorito.Engine.BlockInput(Blam::Input::eInputTypeUi, false); // UI input needs to be unblocked
			done = ui->IsShown();
			dorito.Engine.BlockInput(Blam::Input::eInputTypeUi, true);
			return true;
		}

	private:
		UserInterface* ui;
		bool done = true;
	};

	UserInterface::UserInterface()
	{
		auto& engine = ElDorito::Instance().Engine;
		engine.OnWndProc(BIND_WNDPROC(this, &UserInterface::WndProc));
		engine.OnEndScene(BIND_CALLBACK(this, &UserInterface::EndScene));
		engine.OnEvent("Core", "Direct3D.Init", BIND_CALLBACK(this, &UserInterface::Init));

		engine.PushInputContext(std::make_shared<UIInputContext>(this));
	}

	bool UserInterface::createFontsTexture()
	{
		ImGuiIO& io = ImGui::GetIO();

		// Build
		unsigned char* pixels;
		int width, height, bytes_per_pixel;
		io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height, &bytes_per_pixel);

		// Create DX9 texture
		fontTexture = NULL;
		if (D3DXCreateTexture(device, width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8, D3DPOOL_DEFAULT, &fontTexture) < 0)
			return false;
		D3DLOCKED_RECT tex_locked_rect;
		if (fontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
			return false;
		for (int y = 0; y < height; y++)
			memcpy((unsigned char *)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
		fontTexture->UnlockRect(0);

		// Store our identifier
		io.Fonts->TexID = (void *)fontTexture;

		// Cleanup (don't clear the input data if you want to append new fonts later)
		io.Fonts->ClearInputData();
		io.Fonts->ClearTexData();
		return true;
	}

	bool UserInterface::createDeviceObjects()
	{
		if (!device)
			return false;
		auto retVal = createFontsTexture();
		return retVal;
	}

	void UserInterface::invalidateDeviceObjects()
	{
		if (!device)
			return;
		if (vertexBuf)
		{
			vertexBuf->Release();
			vertexBuf = NULL;
		}
		if (indexBuf)
		{
			indexBuf->Release();
			indexBuf = NULL;
		}
		if (LPDIRECT3DTEXTURE9 tex = (LPDIRECT3DTEXTURE9)ImGui::GetIO().Fonts->TexID)
		{
			tex->Release();
			ImGui::GetIO().Fonts->TexID = 0;
		}
	}

	void UserInterface::newFrame()
	{
		if (!fontTexture)
			createDeviceObjects();

		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		RECT rect;
		GetClientRect(hwnd, &rect);
		io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

		// Setup time step
		INT64 current_time;
		QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
		io.DeltaTime = (float)(current_time - time) / ticksPerSecond;
		time = current_time;

		// Read keyboard modifiers inputs
		io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
		// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
		// io.MousePos : filled by WM_MOUSEMOVE events
		// io.MouseDown : filled by WM_*BUTTON* events
		// io.MouseWheel : filled by WM_MOUSEWHEEL events

		// Hide OS mouse cursor if ImGui is drawing it
		SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

		// Start the frame
		ImGui::NewFrame();
	}

	void UserInterface::renderDrawLists(ImDrawData* draw_data)
	{
		// Create and grow buffers if needed
		if (!vertexBuf || vertexBufferSize < draw_data->TotalVtxCount)
		{
			if (vertexBuf) { vertexBuf->Release(); vertexBuf = NULL; }
			vertexBufferSize = draw_data->TotalVtxCount + 5000;
			if (device->CreateVertexBuffer(vertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &vertexBuf, NULL) < 0)
				return;
		}
		if (!indexBuf || indexBufferSize < draw_data->TotalIdxCount)
		{
			if (indexBuf) { indexBuf->Release(); indexBuf = NULL; }
			indexBufferSize = draw_data->TotalIdxCount + 10000;
			if (device->CreateIndexBuffer(indexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &indexBuf, NULL) < 0)
				return;
		}

		// Copy and convert all vertices into a single contiguous buffer
		CUSTOMVERTEX* vtx_dst;
		ImDrawIdx* idx_dst;
		if (vertexBuf->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, D3DLOCK_DISCARD) < 0)
			return;
		if (indexBuf->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, D3DLOCK_DISCARD) < 0)
			return;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			const ImDrawVert* vtx_src = &cmd_list->VtxBuffer[0];
			for (int i = 0; i < cmd_list->VtxBuffer.size(); i++)
			{
				vtx_dst->pos.x = vtx_src->pos.x;
				vtx_dst->pos.y = vtx_src->pos.y;
				vtx_dst->pos.z = 0.0f;
				vtx_dst->col = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000) >> 16) | ((vtx_src->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
				vtx_dst->uv.x = vtx_src->uv.x;
				vtx_dst->uv.y = vtx_src->uv.y;
				vtx_dst++;
				vtx_src++;
			}
			memcpy(idx_dst, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
			idx_dst += cmd_list->IdxBuffer.size();
		}
		vertexBuf->Unlock();
		indexBuf->Unlock();
		device->SetStreamSource(0, vertexBuf, 0, sizeof(CUSTOMVERTEX));
		device->SetIndices(indexBuf);
		device->SetFVF(D3DFVF_CUSTOMVERTEX);

		// Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing
		device->SetPixelShader(NULL);
		device->SetVertexShader(NULL);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_LIGHTING, false);
		device->SetRenderState(D3DRS_ZENABLE, false);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
		device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

		// Setup orthographic projection matrix
		D3DXMATRIXA16 mat;
		D3DXMatrixIdentity(&mat);
		device->SetTransform(D3DTS_WORLD, &mat);
		device->SetTransform(D3DTS_VIEW, &mat);
		D3DXMatrixOrthoOffCenterLH(&mat, 0.5f, ImGui::GetIO().DisplaySize.x + 0.5f, ImGui::GetIO().DisplaySize.y + 0.5f, 0.5f, -1.0f, +1.0f);
		device->SetTransform(D3DTS_PROJECTION, &mat);

		// Render command lists
		int vtx_offset = 0;
		int idx_offset = 0;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback)
				{
					pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					const RECT r = { (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y, (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w };
					device->SetTexture(0, (LPDIRECT3DTEXTURE9)pcmd->TextureId);
					device->SetScissorRect(&r);
					device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, vtx_offset, 0, (UINT)cmd_list->VtxBuffer.size(), idx_offset, pcmd->ElemCount / 3);
				}
				idx_offset += pcmd->ElemCount;
			}
			vtx_offset += cmd_list->VtxBuffer.size();
		}
	}

	LRESULT UserInterface::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!uiShown)
			return 0;

		ImGuiIO& io = ImGui::GetIO();
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			io.MouseDown[0] = true;
			return true;
		case WM_LBUTTONUP:
			io.MouseDown[0] = false;
			return true;
		case WM_RBUTTONDOWN:
			io.MouseDown[1] = true;
			return true;
		case WM_RBUTTONUP:
			io.MouseDown[1] = false;
			return true;
		case WM_MOUSEWHEEL:
			io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
			return true;
		case WM_MOUSEMOVE:
			io.MousePos.x = (signed short)(lParam);
			io.MousePos.y = (signed short)(lParam >> 16);
			return true;
		case WM_KEYDOWN:
			if (wParam < 256)
				io.KeysDown[wParam] = 1;
			return true;
		case WM_KEYUP:
			if (wParam == VK_ESCAPE && io.KeysDown[wParam] == 1)
				ShowUI(false);
			if (wParam < 256)
				io.KeysDown[wParam] = 0;
			return true;
		case WM_CHAR:
			// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
			if (wParam > 0 && wParam < 0x10000)
				io.AddInputCharacter((unsigned short)wParam);
			return true;
		default:
			return 0;
		}
		return 1;
	}

	void UserInterface::EndScene(void* param)
	{
		if (ElDorito::Instance().NetworkPatches->GetD3DDisabled())
			return;

		if (!uiShown || (windows.size() <= 0 && !console->GetVisible() && !chat->GetVisible() && !playerSettings->GetVisible()))
		{
			ShowUI(false);
			return;
		}

		newFrame();
		std::lock_guard<std::mutex> lock(m);

		for (auto window = windows.begin(); window != windows.end();)
		{
			if (!*window)
				continue;
			(*window)->Draw();
			if (!*window)
				continue;
			if (!(*window)->GetVisible())
			{
				if (!*window)
					continue;
				(*window).reset();
				if (!*window)
					continue;
				window = windows.erase(window);
			}
			else
				++window;
		}
		if (console)
			console->Draw();
		if (chat)
			chat->Draw();
		if (playerSettings)
			playerSettings->Draw();

		ImGui::Render();
	}


	void UserInterface::Init(void* param)
	{
		if (ElDorito::Instance().NetworkPatches->GetD3DDisabled())
			return;

		this->hwnd = ElDorito::Instance().Engine.GetGameHWND();
		this->device = Pointer(0x50DADDC).Read<IDirect3DDevice9*>();

		if (!QueryPerformanceFrequency((LARGE_INTEGER *)&ticksPerSecond))
			return;
		if (!QueryPerformanceCounter((LARGE_INTEGER *)&time))
			return;

		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;                              // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';

		io.RenderDrawListsFn = BIND_CALLBACK(this, &UserInterface::renderDrawLists);
		io.ImeWindowHandle = hwnd;

		this->chat = std::make_shared<ChatWindow>();
		this->console = std::make_shared<ConsoleWindow>(ElDorito::Instance().CommandManager.ConsoleContext);
		this->playerSettings = std::make_shared<PlayerSettingsWindow>();
	}

	bool UserInterface::ShowChat(bool show)
	{
		if (!chat)
			return false;

		chat->SetVisible(show);
		if (show)
			ShowUI(true);
		return show;
	}

	ChatWindowTab UserInterface::SwitchToTab(ChatWindowTab tab)
	{
		if (!chat)
			return ChatWindowTab::GlobalChat;

		return chat->SwitchToTab(tab);
	}

	bool UserInterface::ShowConsole(bool show)
	{
		if (!console)
			return false;

		console->SetVisible(show);
		if (show)
			ShowUI(true);
		return show;
	}

	bool UserInterface::ShowPlayerSettings(bool show)
	{
		if (!playerSettings)
			return false;

		if (show)
			playerSettings->SetPlayerData();

		playerSettings->SetVisible(show);
		if (show)
			ShowUI(true);
		return show;
	}


	void UserInterface::Shutdown()
	{
		if (ElDorito::Instance().NetworkPatches->GetD3DDisabled())
			return;
		invalidateDeviceObjects();
		ImGui::Shutdown();
		device = NULL;
		hwnd = 0;
	}

	bool UserInterface::ShowMessageBox(const std::string& title, const std::string& message, const std::string& tag, std::vector<std::string> choices, MsgBoxCallback callback)
	{
		if (ElDorito::Instance().NetworkPatches->GetD3DDisabled())
			return true;

		auto msgBox = std::make_shared<MessageBoxWindow>(title, message, tag, choices, callback);

		std::lock_guard<std::mutex> lock(m);
		windows.push_back(msgBox);
		msgBox->SetVisible(true);
		ShowUI(true);
		return true;
	}

	void UserInterface::WriteToConsole(const std::string& text)
	{
		if (console)
			console->AddToLog(text);
	}

	void UserInterface::AddToChat(const std::string& text, ChatWindowTab tab)
	{
		if (chat)
			chat->AddToChat(text, tab);
	}

	bool UserInterface::ShowUI(bool show)
	{
		if (ElDorito::Instance().NetworkPatches->GetD3DDisabled())
			return false;

		if (show != uiShown)
		{
			ImGuiIO& io = ImGui::GetIO();
			for (int i = 0; i < 512; i++)
				io.KeysDown[i] = 0;
		}

		if (!uiShown && show) // being shown again, so push a new context
			ElDorito::Instance().Engine.PushInputContext(std::make_shared<UIInputContext>(this));

		uiShown = show;
		return uiShown;
	}
}