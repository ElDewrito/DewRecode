#pragma once
#include <ElDorito/ModuleBase.hpp>
#include <d3dx9.h>
#include <map>

class TextInput
{
public:
	std::string Text;
	size_t CursorIndex = 0;

	TextInput() { }
	~TextInput() { }

	void Entry(char c)
	{
		const std::string temp(1, c);
		Text.insert(CursorIndex, temp);
		CursorIndex++;
	}

	void Backspace()
	{
		if (CursorIndex > 0 && !Text.empty())
			Text.erase(--CursorIndex, 1);
	}

	void Delete()
	{
		if (CursorIndex < Text.size())
			Text.erase(CursorIndex, 1);
	}

	void Left()
	{
		if (CursorIndex > 0)
			CursorIndex--;
	}

	void Right()
	{
		if (CursorIndex < Text.size())
			CursorIndex++;
	}

	void Set(std::string value)
	{
		Text = value;
		CursorIndex = value.length();
	}

	void Clear()
	{
		Text.clear();
		CursorIndex = 0;
	}
};

struct UserInputBox
{
	std::string Text;
	std::string Tag; // a tag / identifier associated with this box, gets passed to the callback with the result
	std::vector<std::string> Choices;
	std::string DefaultText;
	UserInputBoxCallback Callback;
	bool IsMsgBox = false;
};

namespace Modules
{
	class ModuleConsole : public ModuleBase
	{
	public:
		ModuleConsole();

		void Show(std::string group = "Console");
		void Hide();
		bool IsVisible() { return visible; }
		void ShowMessageBox(std::string text, std::string tag, const StringArrayInitializerType& choices, UserInputBoxCallback callback);
		void ShowMessageBox(std::string text, std::string tag, std::vector<std::string>& choices, UserInputBoxCallback callback);
		void ShowInputBox(std::string text, std::string tag, std::string defaultText, UserInputBoxCallback callback);

		void PrintToConsole(std::string str);

		ConsoleBuffer* AddBuffer(ConsoleBuffer buffer);
		bool SetActiveBuffer(ConsoleBuffer* buffer);

		void Draw(IDirect3DDevice9* device);
		LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		static CONST D3DCOLOR COLOR_RED = D3DCOLOR_ARGB(255, 255, 000, 000);
		static CONST D3DCOLOR COLOR_GREEN = D3DCOLOR_ARGB(255, 127, 255, 000);
		static CONST D3DCOLOR COLOR_ORANGE = D3DCOLOR_ARGB(255, 255, 140, 000);
		static CONST D3DCOLOR COLOR_BLUE = D3DCOLOR_ARGB(255, 000, 000, 255);
		static CONST D3DCOLOR COLOR_YELLOW = D3DCOLOR_ARGB(255, 255, 255, 51);
		static CONST D3DCOLOR COLOR_BLACK = D3DCOLOR_ARGB(255, 000, 000, 000);
		static CONST D3DCOLOR COLOR_GREY = D3DCOLOR_ARGB(255, 112, 112, 112);
		static CONST D3DCOLOR COLOR_GOLD = D3DCOLOR_ARGB(255, 255, 215, 000);
		static CONST D3DCOLOR COLOR_PINK = D3DCOLOR_ARGB(255, 255, 192, 203);
		static CONST D3DCOLOR COLOR_PURPLE = D3DCOLOR_ARGB(255, 128, 000, 128);
		static CONST D3DCOLOR COLOR_CYAN = D3DCOLOR_ARGB(255, 000, 255, 255);
		static CONST D3DCOLOR COLOR_MAGNETA = D3DCOLOR_ARGB(255, 255, 000, 255);
		static CONST D3DCOLOR COLOR_WHITE = D3DCOLOR_ARGB(255, 255, 255, 249);

	private:
		const size_t INPUT_MAX_CHARS = 400;
		bool visible = false;
		bool userInputBoxVisible = false;
		bool rawInputHooked = false;

		UserInputBox currentBox;
		std::vector<UserInputBox> queuedBoxes;

		int msgBoxSelectedButton;
		TextInput userInputBoxText;

		TextInput inputBox;

		int normalSizeFontHeight = 0;
		int largeSizeFontHeight = 0;
		int normalSizeCurrentFontHeight = 0;
		int largeSizeCurrentFontHeight = 0;
		LPD3DXFONT normalSizeFont = 0;
		LPD3DXFONT largeSizeFont = 0;

		std::string activeGroup = "Console";
		std::deque<ConsoleBuffer> buffers;
		std::map<std::string, int> activeBufferIdx; // <GroupName, index>
		ConsoleBuffer* consoleBuffer;
		

		int lastTimeConsoleBlink = 0;
		bool consoleBlinking = false;

		bool capsLockToggled = false;

		bool tabHitLast = false;
		int tryCount = 0;
		std::string commandPriorComplete = "";
		std::vector<std::string> currentCommandList = std::vector < std::string > {};

		void initFonts(IDirect3DDevice9* device);

		void drawText(const char* text, int x, int y, DWORD color, LPD3DXFONT pFont);
		void drawRect(IDirect3DDevice9* device, int x, int y, int width, int height, DWORD Color);
		void drawHorizontalLine(IDirect3DDevice9* device, int x, int y, int width, D3DCOLOR Color);
		void drawVerticalLine(IDirect3DDevice9* device, int x, int y, int height, D3DCOLOR Color);
		void drawBox(IDirect3DDevice9* device, int x, int y, int width, int height, D3DCOLOR BorderColor, D3DCOLOR FillColor);
		int centerTextHorizontally(const char* text, int x, int width, LPD3DXFONT pFont);

		int getTextWidth(const char *szText, LPD3DXFONT pFont);
		int getSpaceCharacterWidth(LPD3DXFONT pFont);

		int getMsSinceLastConsoleBlink();

		void userInputBoxKeyCallback(USHORT vKey);
		void consoleKeyCallBack(USHORT vKey);

		void handleDefaultKeyInput(USHORT vKey, TextInput& inputBox);

		int getSelectedIdxForGroup(std::string group);
		int getNumBuffersInGroup(std::string group);
		int getSelectedIdx();
		void switchToNextIdx();

		void hookRawInput();
		void unhookRawInput();
	};
}