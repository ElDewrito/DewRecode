#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../Patches/InputPatchProvider.hpp"

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

		bool VariableRawInputUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableControllerIndexUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);

		bool CommandBind(const std::vector<std::string>& Arguments, ICommandContext& context);

		bool CommandUIButtonPress(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool UIButtonPress(int controllerIdx, Blam::Input::ButtonCodes button);
	};
}