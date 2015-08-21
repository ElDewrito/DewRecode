#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "InputPatchProvider.hpp"

namespace Input
{
	class InputCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<InputPatchProvider> inputPatches;

	public:
		Command* VarRawInput;
		Command* VarControllerIndex;

		explicit InputCommandProvider(std::shared_ptr<InputPatchProvider> inputPatches);

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;

		bool VariableRawInputUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableControllerIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);

		bool CommandBind(const std::vector<std::string>& Arguments, std::string& returnInfo);

		bool CommandUIButtonPress(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool UIButtonPress(int controllerIdx, Blam::Input::ButtonCodes button);
	};
}