#include "ConsoleWindow.hpp"
#include "../../ElDorito.hpp"

namespace UI
{

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
	{
		ConsoleWindow* console = (ConsoleWindow*)data->UserData;
		return console->TextEditCallback(data);
	}

	ConsoleWindow::ConsoleWindow(CommandContext& context) : context(context)
	{
		ClearLog();
		historyPos = -1;
		ZeroMemory(inputBuf, IM_ARRAYSIZE(inputBuf));
	}

	void ConsoleWindow::AddToLog(const std::string& text)
	{
		items.push_back(text);
		scrollToBottom = true;
	}

	void ConsoleWindow::ClearLog()
	{
		items.clear();
		scrollToBottom = true;
	}

	int ConsoleWindow::TextEditCallback(ImGuiTextEditCallbackData* data)
	{
		return 0;
	}

	void ConsoleWindow::Draw()
	{
		if (!isVisible)
			return;

		ImGui::SetNextWindowSize(ImVec2(520, 300), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin("Console", &isVisible))
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
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		for (size_t i = 0; i < items.size(); i++)
		{
			const std::string& item = items[i];
			ImVec4 col = ImColor(255, 255, 255); // A better implementation may store a type per-item. For the sample let's just parse the text.
			if (item.find("[error]") != std::string::npos) col = ImColor(255, 100, 100);
			else if (strncmp(item.c_str(), "# ", 2) == 0) col = ImColor(255, 200, 150);
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

		// Command-line
		if (ImGui::InputText("", inputBuf, IM_ARRAYSIZE(inputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
		{
			char* input_end = inputBuf + strlen(inputBuf);
			while (input_end > inputBuf && input_end[-1] == ' ') input_end--; *input_end = 0;
			if (inputBuf[0])
			{
				std::string input = std::string(inputBuf);
				AddToLog(">" + input);
				context.HandleInput(input);
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

		ImGui::End();
	}
}