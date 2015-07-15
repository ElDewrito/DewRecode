#include "ModuleConsole.hpp"
#include "../ElDorito.hpp"

namespace
{
	void OnEndScene(void* param)
	{
		ElDorito::Instance().Modules.Console.Draw(reinterpret_cast<IDirect3DDevice9*>(param));
	}

	void UIConsoleInput(void* param)
	{
		std::string* string = reinterpret_cast<std::string*>(param);
		ElDorito::Instance().Modules.Console.PrintMultiLineStringToConsole(ElDorito::Instance().Commands.Execute(*string, true));
	}

	LRESULT __stdcall ConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ElDorito::Instance().Modules.Console.WndProc(hWnd, msg, wParam, lParam);
	}

	bool CommandConsoleShow(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		ElDorito::Instance().Modules.Console.Show();
		return true;
	}

	bool CommandConsoleHide(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		ElDorito::Instance().Modules.Console.Hide();
		return true;
	}
}

namespace Modules
{
	ModuleConsole::ModuleConsole() : ModuleBase("Console")
	{
		engine->OnEvent("Core", "Direct3D.EndScene", OnEndScene);
		engine->OnWndProc(ConsoleWndProc);

		AddCommand("Show", "show_console", "Shows the console/chat UI", eCommandFlagsNone, CommandConsoleShow);
		AddCommand("Hide", "hide_console", "Hides the console/chat UI", eCommandFlagsNone, CommandConsoleHide);

		ConsoleBuffer consoleBuff("Console", UIConsoleInput, true);
		consoleBuff.Focused = true;

		consoleBuffer = AddBuffer(consoleBuff);
		consoleBuffer->Messages.push_back("msg1");
		consoleBuffer->Messages.push_back("msg2");

		Show();
	}

	void ModuleConsole::Show()
	{
		if (visible)
			return;

		capsLockToggled = GetKeyState(VK_CAPITAL) & 1;

		visible = true;
		buffers.at(selectedBufferIdx).ScrollIndex = 0;
		buffers.at(selectedBufferIdx).Focused = true;

		// Disables game keyboard input and enables our keyboard hook
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01;
		Rid.usUsage = 0x06;
		Rid.dwFlags = RIDEV_NOLEGACY; // adds HID keyboard and also ignores legacy keyboard messages
		Rid.hwndTarget = 0;

		if (!RegisterRawInputDevices(&Rid, 1, sizeof(Rid)))
			PrintToConsole("Registering keyboard failed");
	}

	void ModuleConsole::Hide()
	{
		if (!visible)
			return;

		inputBox.Clear();
		visible = false;

		// Enables game keyboard input and disables our keyboard hook
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01;
		Rid.usUsage = 0x06;
		Rid.dwFlags = RIDEV_REMOVE;
		Rid.hwndTarget = 0;

		if (!RegisterRawInputDevices(&Rid, 1, sizeof(Rid)))
			PrintToConsole("Unregistering keyboard failed");
	}

	void ModuleConsole::PrintToConsole(std::string str)
	{
		this->consoleBuffer->Messages.push_back(str);
	}

	void ModuleConsole::PrintMultiLineStringToConsole(std::string str)
	{
		std::stringstream ss(str);
		std::string item;
		while (std::getline(ss, item, '\n'))
			if (!item.empty())
				PrintToConsole(item);
	}

	ConsoleBuffer* ModuleConsole::AddBuffer(ConsoleBuffer buffer)
	{
		buffers.push_back(buffer);

		return &buffers.back();
	}

	void ModuleConsole::Draw(IDirect3DDevice9* device)
	{
		if (!visible)
			return;

		initFonts(device);
		auto& res = engine->GetGameResolution();

		// TODO3:
		/*if ((console.getMsSinceLastConsoleOpen() > 10000 && !console.showChat && !console.showConsole) || (GetAsyncKeyState(VK_TAB) & 0x8000 && !console.showChat && !console.showConsole))
		{
			return;
		}*/

		int x = (int)(0.05 * res.first);
		int y = (int)(0.65 * res.second);
		int inputTextBoxWidth = (int)(0.5 * res.first);
		int inputTextBoxHeight = normalSizeFontHeight + (int)(0.769 * normalSizeFontHeight);
		int horizontalSpacing = (int)(0.012 * inputTextBoxWidth);
		int verticalSpacingBetweenEachLine = (int)(0.154 * normalSizeFontHeight);
		int verticalSpacingBetweenLinesAndInputBox = (int)(1.8 * normalSizeFontHeight);
		int verticalSpacingBetweenTopOfInputBoxAndFont = (inputTextBoxHeight - normalSizeFontHeight) / 2;
		size_t maxCharsPerLine = 105;

		int tempX = x;
		for (int i = 0; i < buffers.size(); i++)
		{
			auto& buffer = buffers.at(i);
			std::string displayName = buffer.Name;
			if (i == selectedBufferIdx)
				displayName = ">" + displayName + "<";

			drawBox(device, tempX, y, getTextWidth(displayName.c_str(), normalSizeFont) + 2 * horizontalSpacing, inputTextBoxHeight, COLOR_WHITE, COLOR_BLACK);
			drawText(displayName.c_str(), tempX + horizontalSpacing, y + verticalSpacingBetweenTopOfInputBoxAndFont, COLOR_WHITE, normalSizeFont);
			tempX += getTextWidth(displayName.c_str(), normalSizeFont) + 2 * horizontalSpacing;
		}

		// TODO4: make this text fade out after a while
		drawText("Press tab to switch tabs. Press ` or F1 to open the console.", x, y + verticalSpacingBetweenTopOfInputBoxAndFont + verticalSpacingBetweenLinesAndInputBox, COLOR_WHITE, normalSizeFont);

		y -= verticalSpacingBetweenLinesAndInputBox;

		// Display current input (TODO4: allow input thats longer than the input box)
		drawBox(device, x, y, inputTextBoxWidth, inputTextBoxHeight, COLOR_WHITE, COLOR_BLACK);
		drawText(inputBox.Text.c_str(), x + horizontalSpacing, y + verticalSpacingBetweenTopOfInputBoxAndFont, COLOR_WHITE, normalSizeFont);

		// Line showing where the user currently is in the input field.
		{
			if (getMsSinceLastConsoleBlink() > 300)
			{
				consoleBlinking = !consoleBlinking;
				lastTimeConsoleBlink = GetTickCount();
			}

			if (!consoleBlinking)
			{
				std::string currentInput = inputBox.Text;
				char currentChar;
				int width = 0;
				if (currentInput.length() > 0) {
					currentChar = currentInput[inputBox.CursorIndex];
					width = getTextWidth(currentInput.substr(0, inputBox.CursorIndex).c_str(), normalSizeFont) - 3;
				}
				else
				{
					width = -3;
				}
				drawText("|", x + horizontalSpacing + width, y + verticalSpacingBetweenTopOfInputBoxAndFont, COLOR_WHITE, normalSizeFont);
			}
		}

		y -= verticalSpacingBetweenLinesAndInputBox;

		// Draw text from selected buffer
		auto& selectedBuffer = buffers.at(selectedBufferIdx);
		for (int i = (int)selectedBuffer.Messages.size() - 1 - selectedBuffer.ScrollIndex; i >= 0; i--)
		{
			if (i <= (int)(selectedBuffer.Messages.size() - 1 - selectedBuffer.ScrollIndex) - selectedBuffer.MaxDisplayLines)
				break;

			std::string line = selectedBuffer.Messages.at(i);
			if (line.size() > maxCharsPerLine)
			{
				std::vector<std::string> linesWrapped = std::vector < std::string > {};

				for (size_t i = 0; i < line.size(); i += maxCharsPerLine)
				{
					linesWrapped.push_back(line.substr(i, maxCharsPerLine));
				}

				for (int i = linesWrapped.size() - 1; i >= 0; i--)
				{
					drawText(linesWrapped.at(i).c_str(), x, y, COLOR_WHITE, normalSizeFont);
					y -= normalSizeFontHeight + verticalSpacingBetweenEachLine;
				}
			}
			else
			{
				drawText(line.c_str(), x, y, COLOR_WHITE, normalSizeFont);
				y -= normalSizeFontHeight + verticalSpacingBetweenEachLine;
			}
		}
	}

	void ModuleConsole::initFonts(IDirect3DDevice9* device)
	{
		auto& res = engine->GetGameResolution();
		normalSizeFontHeight = (int)(0.017 * res.first);
		largeSizeFontHeight = (int)(0.034 * res.second);

		if (!normalSizeFont || normalSizeFontHeight != normalSizeCurrentFontHeight)
		{
			if (normalSizeFont)
				normalSizeFont->Release();

			D3DXCreateFont(device, normalSizeFontHeight, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Verdana", &normalSizeFont);
			normalSizeCurrentFontHeight = normalSizeFontHeight;
		}

		if (!largeSizeFont || largeSizeFontHeight != largeSizeCurrentFontHeight)
		{
			if (largeSizeFont)
				largeSizeFont->Release();

			D3DXCreateFont(device, largeSizeFontHeight, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Tahoma", &largeSizeFont);
			largeSizeCurrentFontHeight = largeSizeFontHeight;
		}
	}

	void ModuleConsole::drawText(const char* text, int x, int y, DWORD color, LPD3DXFONT pFont)
	{
		RECT shadow1;
		RECT shadow2;
		RECT shadow3;
		RECT shadow4;
		SetRect(&shadow1, x + 1, y + 1, x + 1, y + 1);
		SetRect(&shadow2, x - 1, y + 1, x - 1, y + 1);
		SetRect(&shadow3, x + 1, y - 1, x + 1, y - 1);
		SetRect(&shadow4, x - 1, y - 1, x - 1, y - 1);
		pFont->DrawTextA(NULL, text, -1, &shadow1, DT_LEFT | DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
		pFont->DrawTextA(NULL, text, -1, &shadow2, DT_LEFT | DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
		pFont->DrawTextA(NULL, text, -1, &shadow3, DT_LEFT | DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));
		pFont->DrawTextA(NULL, text, -1, &shadow4, DT_LEFT | DT_NOCLIP, D3DCOLOR_XRGB(0, 0, 0));

		RECT rect;
		SetRect(&rect, x, y, x, y);
		pFont->DrawTextA(NULL, text, -1, &rect, DT_LEFT | DT_NOCLIP, color);
	}

	void ModuleConsole::drawRect(IDirect3DDevice9* device, int x, int y, int width, int height, DWORD Color)
	{
		D3DRECT rec = { x, y, x + width, y + height };
		device->Clear(1, &rec, D3DCLEAR_TARGET, Color, 0, 0);
	}

	void ModuleConsole::drawHorizontalLine(IDirect3DDevice9* device, int x, int y, int width, D3DCOLOR Color)
	{
		drawRect(device, x, y, width, 1, Color);
	}

	void ModuleConsole::drawVerticalLine(IDirect3DDevice9* device, int x, int y, int height, D3DCOLOR Color)
	{
		drawRect(device, x, y, 1, height, Color);
	}

	void ModuleConsole::drawBox(IDirect3DDevice9* device, int x, int y, int width, int height, D3DCOLOR BorderColor, D3DCOLOR FillColor)
	{
		drawRect(device, x, y, width, height, FillColor);
		drawHorizontalLine(device, x, y, width, BorderColor);
		drawVerticalLine(device, x, y, height, BorderColor);
		drawVerticalLine(device, x + width, y, height, BorderColor);
		drawHorizontalLine(device, x, y + height, width, BorderColor);
	}

	int ModuleConsole::centerTextHorizontally(const char* text, int x, int width, LPD3DXFONT pFont)
	{
		return x + (width - getTextWidth(text, pFont)) / 2;
	}

	int ModuleConsole::getTextWidth(const char *szText, LPD3DXFONT pFont)
	{
		RECT rcRect = { 0, 0, 0, 0 };
		if (pFont)
		{
			pFont->DrawTextA(NULL, szText, strlen(szText), &rcRect, DT_CALCRECT, D3DCOLOR_XRGB(0, 0, 0));
		}
		int width = rcRect.right - rcRect.left;
		std::string text(szText);
		std::reverse(text.begin(), text.end());

		text = text.substr(0, text.find_first_not_of(' ') != std::string::npos ? text.find_first_not_of(' ') : 0);
		for (char c : text)
		{
			width += getSpaceCharacterWidth(pFont);
		}
		return width;
	}

	int ModuleConsole::getSpaceCharacterWidth(LPD3DXFONT pFont)
	{
		return getTextWidth("i i", pFont) - ((getTextWidth("i", pFont)) * 2);
	}

	int ModuleConsole::getMsSinceLastConsoleBlink()
	{
		return GetTickCount() - lastTimeConsoleBlink;
	}

	LRESULT ModuleConsole::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (!visible || msg != WM_INPUT)
			return 0;

		UINT uiSize = 40;
		static unsigned char lpb[40];
		RAWINPUT* rwInput;

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &uiSize, sizeof(RAWINPUTHEADER)) != -1)
		{
			rwInput = (RAWINPUT*)lpb;

			if (rwInput->header.dwType == RIM_TYPEKEYBOARD && (rwInput->data.keyboard.Flags == RI_KEY_MAKE || rwInput->data.keyboard.Flags == RI_KEY_E0))
			{
				consoleKeyCallBack(rwInput->data.keyboard.VKey);
			}
			else if (rwInput->header.dwType == RIM_TYPEMOUSE)
			{
				//console.mouseCallBack(rwInput->data.mouse);
			}
		}

		return 1;
	}

	void ModuleConsole::consoleKeyCallBack(USHORT vKey)
	{
		auto& buffer = buffers.at(selectedBufferIdx);

		switch (vKey)
		{
		case VK_RETURN:
			if (!this->inputBox.Text.empty())
			{
				auto& text = this->inputBox.Text;
				buffer.Messages.push_back(text);
				buffer.InputHistory.push_back(text);
				if (buffer.InputEventCallback != nullptr)
					buffer.InputEventCallback(&text);

				buffer.ScrollIndex = 0;
				this->inputBox.Clear();
			}
			else
				Hide();
			break;

		case VK_ESCAPE:
			Hide();
			break;

		case VK_BACK:
			inputBox.Backspace();
			break;

		case VK_DELETE:
			inputBox.Delete();
			break;

		case VK_CAPITAL:
			capsLockToggled = !capsLockToggled;
			break;

		case VK_PRIOR: // PAGE UP
			if ((int)buffer.ScrollIndex < (int)(buffer.Messages.size() - buffer.MaxDisplayLines))
				buffer.ScrollIndex++;
			break;

		case VK_NEXT: // PAGE DOWN
			if (buffer.ScrollIndex > 0)
				buffer.ScrollIndex--;
			break;

		case VK_UP:
			buffer.InputHistoryIndex++;

			if (buffer.InputHistoryIndex > (int)buffer.InputHistory.size() - 1)
				buffer.InputHistoryIndex--;
			if (buffer.InputHistoryIndex >= 0)
				inputBox.Set(buffer.InputHistory.at(buffer.InputHistory.size() - 1 - buffer.InputHistoryIndex));

			break;

		case VK_DOWN:
			buffer.InputHistoryIndex--;

			if (buffer.InputHistoryIndex < 0)
			{
				buffer.InputHistoryIndex = -1;
				inputBox.Clear();
			}
			else
				inputBox.Set(buffer.InputHistory.at(buffer.InputHistory.size() - 1 - buffer.InputHistoryIndex));

			break;

		case VK_LEFT:
			inputBox.Left();
			break;

		case VK_RIGHT:
			inputBox.Right();
			break;

		case VK_TAB:
			if (selectedBufferIdx + 1 >= buffers.size())
				selectedBufferIdx = 0;
			else
				selectedBufferIdx++;

			buffers.at(selectedBufferIdx).InputHistoryIndex = 0;

			/* TAB COMPLETION TODO3:

			if (currentInput.currentInput.find_first_of(" ") == std::string::npos && currentInput.currentInput.length() > 0)
			{
			if (tabHitLast)
			{
			if (currentCommandList.size() > 0)
			{
			currentInput.set(currentCommandList.at((++tryCount) % currentCommandList.size()));
			}
			}
			else
			{
			tryCount = 0;
			currentCommandList.clear();
			commandPriorComplete = currentInput.currentInput;

			auto currentLine = currentInput.currentInput;
			std::transform(currentLine.begin(), currentLine.end(), currentLine.begin(), ::tolower);

			for (auto cmd : Modules::CommandMap::Instance().Commands)
			{
			auto commandName = cmd.Name;
			std::transform(commandName.begin(), commandName.end(), commandName.begin(), ::tolower);

			if (commandName.compare(0, currentLine.length(), currentLine) == 0)
			{
			currentCommandList.push_back(commandName);
			}
			}
			consoleQueue.pushLineFromGameToUI(std::to_string(currentCommandList.size()) + " commands found starting with \"" + currentLine + ".\"");
			consoleQueue.pushLineFromGameToUI("Press tab to go through them.");
			}
			}*/
			break;

		case 'V':
			if (GetAsyncKeyState(VK_LCONTROL) & 0x8000 || GetAsyncKeyState(VK_RCONTROL) & 0x8000) // CTRL+V pasting
			{
				if (OpenClipboard(nullptr))
				{
					HANDLE hData = GetClipboardData(CF_TEXT);
					if (hData)
					{
						char* textPointer = static_cast<char*>(GlobalLock(hData));
						std::string text(textPointer);
						std::string newInputLine = inputBox.Text + text;

						for (char c : text)
							if (true) // (currentInput.currentInput.size() <= INPUT_MAX_CHARS)
								inputBox.Entry(c);

						GlobalUnlock(hData);
					}
					CloseClipboard();
				}
			}
			else
			{
				handleDefaultKeyInput(vKey);
			}
			break;

		default:
			handleDefaultKeyInput(vKey);
			break;
		}

		//TODO3: tabHitLast = vKey == VK_TAB;
	}

	void ModuleConsole::handleDefaultKeyInput(USHORT vKey)
	{
		if (false) //inputBox.Text.size() > INPUT_MAX_CHARS)
			return;

		WORD buf;
		BYTE keysDown[256] = {};

		if (GetAsyncKeyState(VK_SHIFT) & 0x8000) // 0x8000 = 0b1000000000000000
			keysDown[VK_SHIFT] = 0x80; // sets highest-order bit to 1: 0b10000000

		if (capsLockToggled)
			keysDown[VK_CAPITAL] = 0x1; // sets lowest-order bit to 1: 0b00000001

		int retVal = ToAscii(vKey, 0, keysDown, &buf, 0);

		if (retVal == 1)
			inputBox.Entry(buf & 0x00ff);
		else if (retVal == 2)
		{
			inputBox.Entry(buf >> 8);
			inputBox.Entry(buf & 0x00ff);
		}
	}
}