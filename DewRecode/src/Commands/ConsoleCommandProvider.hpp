#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../UI/ConsoleWindow.hpp"

namespace Console
{
	class ConsoleCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<UI::ConsoleWindow> consoleWindow;

	public:
		explicit ConsoleCommandProvider(std::shared_ptr<UI::ConsoleWindow> consoleWindow);

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		bool CommandDeleteItem(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void DeleteItem();
	};
}