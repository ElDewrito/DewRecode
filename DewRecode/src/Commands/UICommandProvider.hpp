#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>
#include "../Patches/UIPatchProvider.hpp"

namespace UI
{
	class UICommandProvider : public CommandProvider
	{
	private:
		std::shared_ptr<UIPatchProvider> uiPatches;

	public:
		explicit UICommandProvider(std::shared_ptr<UIPatchProvider> uiPatches);

		virtual std::vector<Command> GetCommands() override;

		bool CommandShowChat(const std::vector<std::string>& Arguments, CommandContext& context);
		void ShowChat(bool show);

		bool CommandShowConsole(const std::vector<std::string>& Arguments, CommandContext& context);
		void ShowConsole(bool show);

		bool CommandShowH3UI(const std::vector<std::string>& Arguments, CommandContext& context);
		void ShowH3UI(uint32_t stringId, int32_t arg1, int32_t flags, uint32_t parentStringId);

		bool CommandSettingsMenu(const std::vector<std::string>& Arguments, CommandContext& context);
		bool ShowSettingsMenu(const std::string& menuName);

		void CallbackMsgBox(const std::string& boxTag, const std::string& result);
		void CallbackSettingsMsgBox(const std::string& boxTag, const std::string& result);
		void CallbackInitialSetup(const std::string& boxTag, const std::string& result);
		void CallbackSettingChanged(const std::string& boxTag, const std::string& result);
	};
}