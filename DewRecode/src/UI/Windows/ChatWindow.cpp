#include "ChatWindow.hpp"
#include "../../ElDorito.hpp"
#include "../../Packets/ServerChat.hpp"

namespace UI
{
	class GameChatHandler : public Server::Chat::ChatHandler
	{
	public:
		explicit GameChatHandler(ChatWindow* chatWindow)
			: chatWindow(chatWindow)
		{
		}

		virtual void MessageSent(int senderPeer, Server::Chat::ChatMessage *message, bool *ignore) override
		{
			Censor(message, "dodger"); // ayyyyy
		}

		virtual void MessageReceived(const Server::Chat::ChatMessage &message) override
		{
			auto& dorito = ElDorito::Instance();

			std::string sender;
			if (message.Type == Server::Chat::ChatMessageType::Server)
				sender = "SERVER";
			else
				sender = dorito.Utils.ThinString(message.Sender);

			std::string line;
			if (message.Type == Server::Chat::ChatMessageType::Team)
				line += "(TEAM) ";
			line += "<" + sender + "> " + dorito.Utils.Trim(std::string(message.Body));
			dorito.Utils.RemoveCharsFromString(line, "\t\r\n");
			chatWindow->AddToChat(line, false);
		}

	private:
		void Censor(Server::Chat::ChatMessage *message, const std::string &str)
		{
			std::string body = message->Body;
			ElDorito::Instance().Utils.ReplaceString(body, str, "BLAM!");
			strncpy_s(message->Body, body.c_str(), sizeof(message->Body) - 1);
			message->Body[sizeof(message->Body) - 1] = 0;
		}

		ChatWindow* chatWindow;
	};

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	{
		ChatWindow* chat = (ChatWindow*)data->UserData;
		return chat->TextEditCallback(data);
	}

	ChatWindow::ChatWindow()//CommandContext& context) : context(context)
	{
		ClearChat(false);
		ClearChat(true);
		historyPos = -1;
		ZeroMemory(inputBuf, IM_ARRAYSIZE(inputBuf));

		Server::Chat::AddHandler(std::make_shared<GameChatHandler>(this));
	}

	void ChatWindow::AddToChat(const std::string& text, bool globalChat)
	{
		if (globalChat)
			globalChatItems.push_back(text);
		else
			gameChatItems.push_back(text);

		scrollToBottom = true;
	}

	void ChatWindow::ClearChat(bool globalChat)
	{
		if (globalChat)
			globalChatItems.clear();
		else
			gameChatItems.clear();

		scrollToBottom = true;
	}

	int ChatWindow::TextEditCallback(ImGuiTextEditCallbackData* data)
	{
		return 0;
	}

	void ChatWindow::Draw()
	{
		if (!isVisible)
			return;

		ImGui::SetNextWindowSize(ImVec2(520, 300), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin("Chat", &isVisible))
		{
			ImGui::End();
			return;
		}

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient. You can seek and display only the lines that are visible - CalcListClipping() is a helper to compute this information.
		// If your items are of variable size you may want to implement code similar to what CalcListClipping() does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -(ImGui::GetItemsLineHeightWithSpacing() * 2)));
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearChat(globalChatSelected);
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		size_t numItems = globalChatSelected ? globalChatItems.size() : gameChatItems.size();

		for (size_t i = 0; i < numItems; i++)
		{
			const std::string& item = globalChatSelected ? globalChatItems[i] : gameChatItems[i];
			ImVec4 col = ImColor(255, 255, 255); // A better implementation may store a type per-item. For the sample let's just parse the text.
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted(item.c_str());
			ImGui::PopStyleColor();
		}
		if (scrollToBottom)
			ImGui::SetScrollHere();

		scrollToBottom = false;

		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		auto& dorito = ElDorito::Instance();

		// Command-line
		if (ImGui::InputText("", inputBuf, IM_ARRAYSIZE(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* input_end = inputBuf + strlen(inputBuf);
			while (input_end > inputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
			if (inputBuf[0])
			{
				std::string input = std::string(inputBuf);
				if (!globalChatSelected)
				{
					// Messages beginning with !t or !team are team messages
					if (input.substr(0, 3) == "!t " || input.substr(0, 6) == "!team ")
					{
						input = dorito.Utils.Trim(input.substr(input.find(' ') + 1));
						if (input.length() && !Server::Chat::SendTeamMessage(input))
							AddToChat("(Failed to send message! Are you in a game with teams enabled?)", false);
					}
					else
					{
						input = dorito.Utils.Trim(input);
						if (input.length() && !Server::Chat::SendGlobalMessage(input))
							AddToChat("(Failed to send message! Are you in a game?)", false);
					}
				}
				else
				{
					// send to global chat
					if(dorito.IRCCommands->SendMsg(input))
						AddToChat("<" + dorito.PlayerCommands->VarName->ValueString + "> " + input, true);
				}

				ZeroMemory(inputBuf, 256);
				scrollToBottom = true;
			}
			else
			{
				// pressed enter with no text - hide the window
				isVisible = false;
			}
		}

		// focus on the input box
		if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1);

		static int e = 0;
		if (ImGui::RadioButton("Global Chat", &e, 0))
			globalChatSelected = true;

		ImGui::SameLine();
		if (ImGui::RadioButton("Game Chat", &e, 1))
			globalChatSelected = false;


		ImGui::End();
	}
}