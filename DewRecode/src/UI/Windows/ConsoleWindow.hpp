#pragma once
#include <ElDorito/ElDorito.hpp>
#include "../imgui/imgui.h"

namespace UI
{
	class ConsoleWindow : public UIWindow
	{
		bool isVisible = false;

		char inputBuf[256];
		std::vector<std::string> items;
		bool scrollToBottom;
		std::vector<std::string> history;
		int historyPos;    // -1: new line, 0..History.Size-1 browsing history.

		CommandContext& context;

	public:
		ConsoleWindow(CommandContext& context);

		void Draw();

		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }

		void AddToLog(const std::string& text);
		void ClearLog();
		int TextEditCallback(ImGuiTextEditCallbackData* data);
	};
}