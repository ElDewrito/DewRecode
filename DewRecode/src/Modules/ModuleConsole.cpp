#include "ModuleConsole.hpp"
#include "../ElDorito.hpp"

namespace
{
	void OnEndScene(void* param)
	{
		ElDorito::Instance().Modules.Console.Draw(reinterpret_cast<IDirect3DDevice9*>(param));
	}

	void UIConsoleInput(const std::string& input, ConsoleBuffer* buffer)
	{
		auto& console = ElDorito::Instance().Modules.Console;
		console.PrintToConsole(">" + input);
		console.PrintToConsole(ElDorito::Instance().Commands.Execute(input, true));
	}

	LRESULT __stdcall ConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return ElDorito::Instance().Modules.Console.WndProc(hWnd, msg, wParam, lParam);
	}

	bool CommandConsoleShow(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		std::string group = "Console";
		if (Arguments.size() > 0)
			group = Arguments[0];

		ElDorito::Instance().Modules.Console.Show(group);
		return true;
	}

	bool CommandConsoleHide(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		ElDorito::Instance().Modules.Console.Hide();
		return true;
	}

	void TestMsgBoxResult(std::string buttonChoice)
	{
		ElDorito::Instance().Modules.Console.PrintToConsole("You chose: " + buttonChoice);
	}

	bool CommandConsoleTestMsgBox(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() <= 0)
			return false;

		std::vector<std::string> choices;
		for (size_t i = 1; i < Arguments.size(); i++)
			choices.push_back(Arguments.at(i));

		ElDorito::Instance().Modules.Console.ShowMessageBox(Arguments.at(0), choices, TestMsgBoxResult);

		return true;
	}

	std::string msgBoxCommand;

	void MsgBoxResult(std::string buttonChoice)
	{
		ElDorito::Instance().Commands.Execute(msgBoxCommand + " " + buttonChoice);
	}

	bool CommandConsoleMsgBox(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() <= 1)
			return false;

		msgBoxCommand = Arguments.at(1);

		std::vector<std::string> choices;
		for (size_t i = 2; i < Arguments.size(); i++)
			choices.push_back(Arguments.at(i));

		ElDorito::Instance().Modules.Console.ShowMessageBox(Arguments.at(0), choices, MsgBoxResult);

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

		AddCommand("TestMsgBox", "testmsgbox", "Opens a test message box, result is printed into the console", eCommandFlagsNone, CommandConsoleTestMsgBox, { "text(string) The text to show on the message box", "choices(string)... The choices to show on the message box" });
		AddCommand("MsgBox", "msgbox", "Opens a message box, result is passed to the command specified", eCommandFlagsNone, CommandConsoleMsgBox, { "text(string) The text to show on the message box", "command(string) The command to run, with the result passed to it", "choices(string)... The choices to show on the message box" });
		ConsoleBuffer consoleBuff("Console", "Console", UIConsoleInput, true);
		consoleBuff.Focused = true;

		consoleBuffer = AddBuffer(consoleBuff);
		PrintToConsole("ElDewrito Version: " + Utils::Version::GetVersionString() + " Build Date: " + __DATE__ + " " + __TIME__);
	}

	void ModuleConsole::Show(std::string group)
	{
		group = utils->ToLower(group);
		if (activeGroup.compare(group) && getNumBuffersInGroup(group) > 0)
			activeGroup = group;

		if (visible || getNumBuffersInGroup(group) <= 0)
			return;

		capsLockToggled = GetKeyState(VK_CAPITAL) & 1;

		buffers.at(getSelectedIdx()).ScrollIndex = 0;
		buffers.at(getSelectedIdx()).Focused = true;

		visible = true;
		hookRawInput();
	}

	void ModuleConsole::Hide()
	{
		if (!visible)
			return;

		inputBox.Clear();
		buffers.at(getSelectedIdx()).TimeLastShown = GetTickCount();
		visible = false;

		if (!msgBoxVisible)
			unhookRawInput();
	}

	void ModuleConsole::ShowMessageBox(std::string text, const StringArrayInitializerType& choices, MessageBoxCallback callback)
	{
		std::vector<std::string> choicesVect = choices;
		ShowMessageBox(text, choicesVect, callback);
	}

	void ModuleConsole::ShowMessageBox(std::string text, std::vector<std::string>& choices, MessageBoxCallback callback)
	{
		// TODO: support having multiple msg boxes at once
		msgBoxText = text;
		msgBoxChoices = choices;
		msgBoxCallback = callback;
		msgBoxSelectedButton = 0;
		msgBoxVisible = true;

		hookRawInput();
	}

	void ModuleConsole::hookRawInput()
	{
		if (rawInputHooked)
			return;

		// Disables game keyboard input and enables our keyboard hook
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01;
		Rid.usUsage = 0x06;
		Rid.dwFlags = RIDEV_NOLEGACY; // adds HID keyboard and also ignores legacy keyboard messages
		Rid.hwndTarget = 0;

		if (!RegisterRawInputDevices(&Rid, 1, sizeof(Rid)))
			PrintToConsole("Registering keyboard failed");

		rawInputHooked = true;
	}

	void ModuleConsole::unhookRawInput()
	{
		if (!rawInputHooked)
			return;

		// Enables game keyboard input and disables our keyboard hook
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01;
		Rid.usUsage = 0x06;
		Rid.dwFlags = RIDEV_REMOVE;
		Rid.hwndTarget = 0;

		if (!RegisterRawInputDevices(&Rid, 1, sizeof(Rid)))
			PrintToConsole("Unregistering keyboard failed");

		rawInputHooked = false;
	}

	void ModuleConsole::PrintToConsole(std::string str)
	{
		logger->Log(LogSeverity::Debug, "Console", str); // log everything

		if (str.find('\n') == std::string::npos)
		{
			this->consoleBuffer->PushLine(str);
			return;
		}

		std::stringstream ss(str);
		std::string item;
		while (std::getline(ss, item, '\n'))
			if (!item.empty())
				this->consoleBuffer->PushLine(item);
	}

	ConsoleBuffer* ModuleConsole::AddBuffer(ConsoleBuffer buffer)
	{
		buffer.Group = utils->ToLower(buffer.Group);
		buffers.push_back(buffer);
		if (activeBufferIdx.find(buffer.Group) == activeBufferIdx.end())
			activeBufferIdx.insert(std::pair<std::string, int>(buffer.Group, buffers.size() - 1));

		return &buffers.back();
	}

	bool ModuleConsole::SetActiveBuffer(ConsoleBuffer* buffer)
	{
		buffer->Group = utils->ToLower(buffer->Group);
		int bufferIdx = -1;
		for (size_t i = 0; i < buffers.size(); i++)
		{
			if (buffer != &buffers.at(i))
				continue;
			bufferIdx = i;
			break;
		}
		if (bufferIdx == -1)
			return false;

		auto it = this->activeBufferIdx.find(buffer->Group);
		if (it == this->activeBufferIdx.end())
			this->activeBufferIdx.insert(std::pair<std::string, int>(buffer->Group, bufferIdx));
		else
			(*it).second = bufferIdx;

		return true;
	}

	void ModuleConsole::Draw(IDirect3DDevice9* device)
	{
		auto& res = engine->GetGameResolution();

		initFonts(device);

		int x = (int)(0.05 * res.first);
		int y = (int)(0.65 * res.second);
		int inputTextBoxWidth = (int)(0.5 * res.first);
		int inputTextBoxHeight = normalSizeFontHeight + (int)(0.769 * normalSizeFontHeight);
		int horizontalSpacing = (int)(0.012 * inputTextBoxWidth);
		int verticalSpacingBetweenEachLine = (int)(0.154 * normalSizeFontHeight);
		int verticalSpacingBetweenLinesAndInputBox = (int)(1.8 * normalSizeFontHeight);
		int verticalSpacingBetweenTopOfInputBoxAndFont = (inputTextBoxHeight - normalSizeFontHeight) / 2;
		size_t maxCharsPerLine = 105;

		auto& selectedBuffer = buffers.at(getSelectedIdx());
		bool consoleVisible = !(GetTickCount() - selectedBuffer.TimeLastShown > 10000 && !visible);

		if (consoleVisible)
		{

			if (visible) // only show the input box / tab selection if the console is actually open
			{
				int tempX = x;
				for (size_t i = 0; i < buffers.size(); i++)
				{
					auto& buffer = buffers.at(i);
					if (!buffer.Visible || utils->ToLower(buffer.Group).compare(activeGroup))
						continue;

					std::string displayName = buffer.Name;
					if (i == getSelectedIdx() && getNumBuffersInGroup(activeGroup) > 1)
						displayName = ">" + displayName + "<";

					drawBox(device, tempX, y, getTextWidth(displayName.c_str(), normalSizeFont) + 2 * horizontalSpacing, inputTextBoxHeight, COLOR_WHITE, COLOR_BLACK);
					drawText(displayName.c_str(), tempX + horizontalSpacing, y + verticalSpacingBetweenTopOfInputBoxAndFont, COLOR_WHITE, normalSizeFont);
					tempX += getTextWidth(displayName.c_str(), normalSizeFont) + 2 * horizontalSpacing;
				}

				// TODO4: make this text fade out after a while
				if (getNumBuffersInGroup(activeGroup) > 1)
					drawText("Press tab to switch tabs. Press ` or F1 to open the console.", x, y + verticalSpacingBetweenTopOfInputBoxAndFont + verticalSpacingBetweenLinesAndInputBox, COLOR_WHITE, normalSizeFont);

				y -= verticalSpacingBetweenLinesAndInputBox;

				// Display current input (TODO4: scroll input thats longer than the input box)
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
			}

			y -= verticalSpacingBetweenLinesAndInputBox;

			// Draw text from selected buffer
			for (int i = (int)selectedBuffer.Messages.size() - 1 - selectedBuffer.ScrollIndex; i >= 0; i--)
			{
				if (i <= (int)(selectedBuffer.Messages.size() - 1 - selectedBuffer.ScrollIndex) - selectedBuffer.MaxDisplayLines)
					break;

				std::string& line = selectedBuffer.Messages.at(i);
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

		if (msgBoxVisible)
		{
			int msgBoxX = (int)(0.25 * res.first);
			int msgBoxY = (int)(0.25 * res.second);
			msgBoxY += (int)(0.5 * msgBoxY);
			int msgBoxWidth = (int)(0.5 * res.first);
			int msgBoxHeight = (int)(0.5 * res.second);
			msgBoxHeight -= (int)(0.5 * msgBoxHeight);

			drawBox(device, msgBoxX, msgBoxY, msgBoxWidth, msgBoxHeight, COLOR_WHITE, COLOR_BLACK);
			int height = 1;
			size_t numLines = std::count(msgBoxText.begin(), msgBoxText.end(), '\n');
			height += numLines;

			int heightPixels = normalSizeFontHeight * height; // this is a little off, but should be fine

			drawText(msgBoxText.c_str(), centerTextHorizontally(msgBoxText.c_str(), msgBoxX, msgBoxWidth, normalSizeFont), msgBoxY + (int)(0.5 * msgBoxHeight) - (int)(0.5 * heightPixels), COLOR_WHITE, normalSizeFont);

			int largestButtonWidth = 0;
			for (auto choice : msgBoxChoices)
			{
				int width = getTextWidth(choice.c_str(), normalSizeFont) + 2 * horizontalSpacing;
				if (width > largestButtonWidth)
					largestButtonWidth = width;
			}

			int totalBtnWidth = (largestButtonWidth * msgBoxChoices.size());
			int spaceBetweenButtons = (int)(0.04 * msgBoxWidth);
			if (msgBoxChoices.size() > 1)
				totalBtnWidth += (spaceBetweenButtons * (msgBoxChoices.size() - 1)); // spaces between each button
			int buttonX = msgBoxX + (int)(0.5 * (msgBoxWidth - totalBtnWidth));

			for (size_t i = 0; i < msgBoxChoices.size(); i++)
			{
				std::string& choice = msgBoxChoices.at(i);
				int buttonY = msgBoxY + msgBoxHeight - ((int)(0.075 * msgBoxHeight)) - inputTextBoxHeight;

				drawBox(device, buttonX, buttonY, largestButtonWidth, inputTextBoxHeight, COLOR_WHITE, i == msgBoxSelectedButton ? COLOR_GREEN : COLOR_BLACK);

				int textX = centerTextHorizontally(choice.c_str(), buttonX, largestButtonWidth, normalSizeFont);
				drawText(choice.c_str(), textX, buttonY + verticalSpacingBetweenTopOfInputBoxAndFont, COLOR_WHITE, normalSizeFont);
				buttonX += largestButtonWidth + spaceBetweenButtons;
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
		if ((!visible && !msgBoxVisible) || msg != WM_INPUT)
			return 0;

		UINT uiSize = 40;
		static unsigned char lpb[40];
		RAWINPUT* rwInput;

		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &uiSize, sizeof(RAWINPUTHEADER)) != -1)
		{
			rwInput = (RAWINPUT*)lpb;

			if (rwInput->header.dwType == RIM_TYPEKEYBOARD && (rwInput->data.keyboard.Flags == RI_KEY_MAKE || rwInput->data.keyboard.Flags == RI_KEY_E0))
			{
				if (msgBoxVisible)
					messageBoxKeyCallback(rwInput->data.keyboard.VKey);
				else
					consoleKeyCallBack(rwInput->data.keyboard.VKey);
			}
			else if (rwInput->header.dwType == RIM_TYPEMOUSE)
			{
				//console.mouseCallBack(rwInput->data.mouse);
			}
		}

		return 1;
	}

	void ModuleConsole::messageBoxKeyCallback(USHORT vKey)
	{
		switch (vKey)
		{
		case VK_LEFT:
			if (msgBoxSelectedButton <= 0)
				msgBoxSelectedButton = msgBoxChoices.size() - 1;
			else
				msgBoxSelectedButton--;

			if (msgBoxSelectedButton < 0)
				msgBoxSelectedButton = 0;

			break;
		case VK_RIGHT:
			if ((msgBoxSelectedButton + 1) < (int)msgBoxChoices.size())
				msgBoxSelectedButton++;
			else
				msgBoxSelectedButton = 0;

			if (msgBoxSelectedButton >= (int)msgBoxChoices.size())
				msgBoxSelectedButton = msgBoxChoices.size() - 1;

			if (msgBoxSelectedButton < 0)
				msgBoxSelectedButton = 0;

			break;
		case VK_RETURN:
		case VK_SPACE:
			if (msgBoxCallback && msgBoxSelectedButton >= 0 && msgBoxSelectedButton < (int)msgBoxChoices.size())
				msgBoxCallback(msgBoxChoices.at(msgBoxSelectedButton));
			msgBoxVisible = false;
			unhookRawInput();
			break;
		}
	}

	void ModuleConsole::consoleKeyCallBack(USHORT vKey)
	{
		auto& buffer = buffers.at(getSelectedIdx());
		auto& dorito = ElDorito::Instance();

		switch (vKey)
		{
		case VK_RETURN:
			if (!this->inputBox.Text.empty())
			{
				auto& text = this->inputBox.Text;
				buffer.InputHistory.push_back(text);
				if (buffer.InputCallback != nullptr)
					buffer.InputCallback(text, &buffer);

				buffer.ScrollIndex = 0;
				this->inputBox.Clear();
			}

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
			if (getNumBuffersInGroup(activeGroup) > 1)
				switchToNextIdx();
			else
			{
				if (inputBox.Text.find_first_of(" ") != std::string::npos || inputBox.Text.empty())
					break;

				if (tabHitLast)
				{
					if (currentCommandList.size() > 0)
						inputBox.Set(currentCommandList.at((++tryCount) % currentCommandList.size()));
				}
				else
				{
					tryCount = 0;
					currentCommandList.clear();
					commandPriorComplete = inputBox.Text;

					auto currentLine = utils->ToLower(inputBox.Text);

					for (auto cmd : dorito.Commands.List)
					{
						auto commandName = utils->ToLower(cmd.Name);

						if (commandName.compare(0, currentLine.length(), currentLine) == 0)
							currentCommandList.push_back(cmd.Name);
					}
					buffers.at(getSelectedIdx()).PushLine(std::to_string(currentCommandList.size()) + " commands found starting with \"" + currentLine + ".\"");
					buffers.at(getSelectedIdx()).PushLine("Press tab to go through them.");
				}
			}
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
							if (inputBox.Text.size() <= INPUT_MAX_CHARS)
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

		tabHitLast = vKey == VK_TAB;
	}

	void ModuleConsole::handleDefaultKeyInput(USHORT vKey)
	{
		if (inputBox.Text.size() > INPUT_MAX_CHARS)
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

	int ModuleConsole::getSelectedIdxForGroup(std::string group)
	{
		auto it = activeBufferIdx.find(group);
		if (it == activeBufferIdx.end())
			return 0;
		return (*it).second;
	}

	int ModuleConsole::getSelectedIdx()
	{
		return getSelectedIdxForGroup(activeGroup);
	}

	int ModuleConsole::getNumBuffersInGroup(std::string group)
	{
		int count = 0;
		for (auto buff : buffers)
		{
			if (!buff.Group.compare(group))
				count++;
		}
		return count;
	}

	void ModuleConsole::switchToNextIdx()
	{
		// first try looking for a buffer in the same group starting from the current buffer
		int nextIdx = 0;
		bool foundBuff = false;
		auto it = activeBufferIdx.find(activeGroup);
		if (it == activeBufferIdx.end()) // if it don't exist create it
			activeBufferIdx.insert(std::pair<std::string, int>(activeGroup, nextIdx));
		else
			nextIdx = (*it).second + 1;

		auto it2 = activeBufferIdx.find(activeGroup);
		for (size_t i = nextIdx; i < buffers.size(); i++)
		{
			if (buffers.at(i).Group.compare(activeGroup))
				continue;
			nextIdx = i;
			foundBuff = true;
			break;
		}

		if (foundBuff)
			(*it2).second = nextIdx;
		else // couldn't find one in the same group, try starting from the beginning
			for (size_t i = 0; i < buffers.size(); i++)
			{
				if (buffers.at(i).Group.compare(activeGroup))
					continue;
				(*it2).second = i;
				break;
			}

		buffers.at(getSelectedIdx()).InputHistoryIndex = 0;
	}
}