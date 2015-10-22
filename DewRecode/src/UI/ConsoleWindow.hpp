#pragma once
#include "imgui/imgui.h"
#include <ElDorito/ElDorito.hpp>

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

typedef std::function<void(void* userdata)> GuiHandlerCallback;

struct PanelTab
{
	std::string tabName;
	void* textBuffer;
	
};

/*template<typename T>
T clamp(const T &val, const T &min, const T &max)
{
	return std::max(min, std::min(max, val));
}*/

namespace UI
{
	class ConsoleWindow
	{
	private:
		char inputBuf[256];
		std::vector<std::string> items;
		bool scrollToBottom;
		std::vector<std::string> history;
		int historyPos;    // -1: new line, 0..History.Size-1 browsing history.

		int selectedTab = -1;
		std::vector<PanelTab> tabs;

		std::string title;
		ICommandContext& context;

	public:
		ConsoleWindow(std::string title, ICommandContext& context);
		~ConsoleWindow();

		void AddToLog(const std::string& text);
		void ClearLog();
		void Draw(bool* opened);
		int TextEditCallback(ImGuiTextEditCallbackData* data);

		bool SetSelectedTab(const std::string& tabName);
		bool IsSelectedTab(const std::string& tabName);

	};
}