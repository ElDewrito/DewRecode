#include "InputCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace Input
{
	InputCommandProvider::InputCommandProvider(std::shared_ptr<InputPatchProvider> inputPatches)
	{
		this->inputPatches = inputPatches;
	}

	std::vector<Command> InputCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Input", "Bind", "bind", "Binds a command to a key", eCommandFlagsNone, BIND_COMMAND(this, &InputCommandProvider::CommandBind), { "key", "[+]command", "arguments" }),
			Command::CreateCommand("Input", "UIButtonPress", "ui_btn_press", "Emulates a gamepad button press on UI menus", eCommandFlagsNone, BIND_COMMAND(this, &InputCommandProvider::CommandUIButtonPress), { "btnCode The code of the button to press" })
		};

		return commands;
	}

	void InputCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarRawInput = manager->Add(Command::CreateVariableInt("Input", "RawInput", "rawinput", "Enables raw mouse input with no acceleration applied", eCommandFlagsArchived, 1, BIND_COMMAND(this, &InputCommandProvider::VariableRawInputUpdate)));
		VarRawInput->ValueIntMin = 0;
		VarRawInput->ValueIntMax = 1;

		VarControllerIndex = manager->Add(Command::CreateVariableInt("Input", "ControllerIndex", "controllerindex", "Which controller index should be used for player 1", eCommandFlagsArchived, 0, BIND_COMMAND(this, &InputCommandProvider::VariableControllerIndexUpdate)));
		VarControllerIndex->ValueIntMin = 0;
		VarControllerIndex->ValueIntMax = 3;
	}

	bool InputCommandProvider::VariableRawInputUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = VarRawInput->ValueInt;

		std::string status = "disabled.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "enabled.";

		returnInfo = "Raw input " + status;
		return true;
	}

	bool InputCommandProvider::VariableControllerIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = VarControllerIndex->ValueInt;
		inputPatches->SetControllerIndex(value);
		return true;
	}

	bool InputCommandProvider::CommandBind(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() < 1)
		{
			returnInfo = "Usage: Bind <key> [[+]command] [arguments]\n";
			returnInfo += "If the command starts with a +, then it will be ";
			returnInfo += "passed an argument of 1 on key down and 0 on key ";
			returnInfo += "up. Omit the command to unbind the key.";
			return false;
		}

		std::string command = "";

		for (size_t i = 1; i < Arguments.size(); i++)
		{
			command += Arguments.at(i) + " ";
		}

		auto& dorito = ElDorito::Instance();

		command = dorito.Utils.Trim(command);

		auto retVal = dorito.CommandManager.AddBinding(Arguments[0], command);
		if (retVal == BindingReturnValue::Success)
		{
			returnInfo = "Binding set";
			return true;
		}
		if (retVal == BindingReturnValue::ClearedBinding)
		{
			returnInfo = "Binding cleared";
			return true;
		}

		if (retVal == BindingReturnValue::UnknownKey)
			returnInfo = "Unrecognized key name: " + Arguments[0];
		else if (retVal == BindingReturnValue::InvalidArgument)
			returnInfo = "Invalid command";

		return false;
	}

	bool InputCommandProvider::CommandUIButtonPress(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() < 1)
		{
			returnInfo = "Usage: Input.UIButtonPress <btnCode>";
			return false;
		}

		return UIButtonPress(0, (Blam::Input::ButtonCodes)std::stoul(Arguments[0], 0, 0));
	}

	bool InputCommandProvider::UIButtonPress(int controllerIdx, Blam::Input::ButtonCodes button)
	{
		// taken from sub_A935C0

		// alt?A936B0

		uint32_t keyPressData[4];
		keyPressData[0] = 0xD;
		keyPressData[1] = controllerIdx;
		keyPressData[2] = (uint32_t)button;
		keyPressData[3] = 0xFF;

		typedef void*(__cdecl *sub_AAD930Ptr)();
		auto sub_AAD930 = reinterpret_cast<sub_AAD930Ptr>(0xAAD930);
		void* classPtr = sub_AAD930();

		typedef int(__thiscall *sub_AAB7D0Ptr)(void* thisPtr, void* a2);
		auto sub_AAB7D0 = reinterpret_cast<sub_AAB7D0Ptr>(0xAAB7D0);
		int retVal = sub_AAB7D0(classPtr, keyPressData);

		return retVal != 0;
	}
}