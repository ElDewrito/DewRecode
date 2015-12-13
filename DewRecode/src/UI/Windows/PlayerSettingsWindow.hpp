#pragma once
#include <ElDorito/ElDorito.hpp>
#include "../imgui/imgui.h"

namespace UI
{
	class PlayerSettingsWindow : public UIWindow
	{
		bool isVisible = false;

		char username[256];

		std::vector<const char*> armorNames;
		int helmetCurrent;
		int chestCurrent;
		int shouldersCurrent;
		int armsCurrent;
		int legsCurrent;

		float* primary;
		float* secondary;
		float* visor;
		float* lights;
		float* holo;

	public:
		PlayerSettingsWindow();

		void Draw();
		bool SetVisible(bool visible) { isVisible = visible; return visible; }
		bool GetVisible() { return isVisible; }

		void SetPlayerData();
	};
}