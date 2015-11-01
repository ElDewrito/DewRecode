#pragma once
#include <ElDorito/ElDorito.hpp>
#include "../imgui/imgui.h"

namespace UI
{
	class ChatWindow : public UIWindow
	{
		bool isVisible = false;

		char inputBuf[256];
		std::vector<std::string> globalChatItems;
		std::vector<std::string> gameChatItems;
		bool scrollToBottom;
		std::vector<std::string> history;
		int historyPos;    // -1: new line, 0..History.Size-1 browsing history.
		bool globalChatSelected = true;

		//CommandContext& context;

	public:
		ChatWindow();// CommandContext& context);

		void Draw();

		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }
		bool SwitchChat(bool globalChat) { globalChatSelected = globalChat; return globalChat; }

		void AddToChat(const std::string& text, bool globalChat);
		void ClearChat(bool globalChat);
		int TextEditCallback(ImGuiTextEditCallbackData* data);
	};
}