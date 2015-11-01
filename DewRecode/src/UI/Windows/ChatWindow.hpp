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
		std::vector<std::string> rconItems;

		bool scrollToBottom;
		std::vector<std::string> history;
		int historyPos;    // -1: new line, 0..History.Size-1 browsing history.
		ChatWindowTab selectedTab = ChatWindowTab::GlobalChat;

	public:
		ChatWindow();

		void Draw();

		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }

		ChatWindowTab SwitchToTab(ChatWindowTab tab) { selectedTab = tab; return selectedTab; }
		void AddToChat(const std::string& text, ChatWindowTab tab);
		void ClearChat(ChatWindowTab tab);

		int TextEditCallback(ImGuiTextEditCallbackData* data);
	};
}