#pragma once
#include <ElDorito/ElDorito.hpp>
#include "../imgui/imgui.h"

namespace UI
{
	class PlayerSettingsWindow : public UIWindow
	{
		bool isVisible = false;

		char username[256];

	public:
		PlayerSettingsWindow();

		void Draw();
		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }

		void SetUsername(const std::string& name) { ZeroMemory(username, 256); strcpy_s(username, 256, name.c_str()); }
	};
}