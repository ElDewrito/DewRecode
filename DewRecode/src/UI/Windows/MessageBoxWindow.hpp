#pragma once
#include <ElDorito/ElDorito.hpp>
#include "../imgui/imgui.h"

namespace UI
{
	class MessageBoxWindow : public UIWindow
	{
		bool isVisible = false;

		MsgBoxCallback callback;
		std::vector<std::string> choices;
		std::string message;
		std::string title;
		std::string tag;

	public:
		MessageBoxWindow(const std::string& title, const std::string& message, const std::string& tag, std::vector<std::string> choices, MsgBoxCallback callback);

		void Draw();
		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }
	};
}