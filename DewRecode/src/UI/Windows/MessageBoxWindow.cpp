#include "MessageBoxWindow.hpp"

namespace UI
{
	MessageBoxWindow::MessageBoxWindow(const std::string& title, const std::string& message, std::vector<std::string> choices, MsgBoxCallback callback)
	{
		this->title = title;
		this->message = message;
		this->choices = choices;
		this->callback = callback;
	}

	void MessageBoxWindow::Draw()
	{
		if (!isVisible)
			return;

		//ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
		if (!ImGui::Begin(title.c_str(), &isVisible))
		{
			ImGui::End();
			return;
		}

		ImGui::Text(message.c_str());
		for (auto i : choices)
		{
			if (ImGui::Button(i.c_str()))
			{
				isVisible = false;
				callback(i, nullptr);
			}
			ImGui::SameLine();
		}

		ImGui::End();
	}
}