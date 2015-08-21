#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "UIPatchProvider.hpp"

namespace UI
{
	class UICommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<UIPatchProvider> uiPatches;

	public:
		explicit UICommandProvider(std::shared_ptr<UIPatchProvider> uiPatches);

		virtual std::vector<Command> GetCommands() override;

		bool CommandShowH3UI(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void ShowH3UI(uint32_t stringId, int32_t arg1, int32_t flags, uint32_t parentStringId);

		bool CommandSettingsMenu(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool ShowSettingsMenu(const std::string& menuName);

		void CallbackMsgBox(const std::string& boxTag, const std::string& result);
		void CallbackSettingsMsgBox(const std::string& boxTag, const std::string& result);
		void CallbackInitialSetup(const std::string& boxTag, const std::string& result);
		void CallbackSettingChanged(const std::string& boxTag, const std::string& result);
	};
}