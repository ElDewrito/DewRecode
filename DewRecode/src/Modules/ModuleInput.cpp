#include "ModuleInput.hpp"
#include <sstream>
#include <algorithm>
#include "../ElDorito.hpp"

namespace
{
	int controllerIndex = 0;

	bool VariableInputRawInputUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = ElDorito::Instance().Modules.Input.VarInputRawInput->ValueInt;

		std::string status = "disabled.";
		bool statusBool = value != 0;
		if (statusBool)
			status = "enabled.";

		returnInfo = "Raw input " + status;
		return true;
	}

	bool VariableInputControllerIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		unsigned long value = ElDorito::Instance().Modules.Input.VarInputControllerIndex->ValueInt;
		controllerIndex = value;
		return true;
	}


	__declspec(naked) void Input_ControllerIndexHook()
	{
		__asm
		{
			mov edi, controllerIndex
			mov esi, eax
			mov ebx, 0x238E714
			push 0x5128F4
			ret
		}
	}

	// Maps key names to key code values
	extern std::map<std::string, Blam::KeyCodes> keyCodes;

	bool CommandBind(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		if (Arguments.size() < 1)
		{
			returnInfo =  "Usage: Bind <key> [[+]command] [arguments]\n";
			returnInfo += "If the command starts with a +, then it will be ";
			returnInfo += "passed an argument of 1 on key down and 0 on key ";
			returnInfo += "up. Omit the command to unbind the key.";
			return false;
		}

		// Get the key, convert it to lowercase, and translate it to a key code
		auto key = ElDorito::Instance().Utils.ToLower(Arguments[0]);
		auto it = keyCodes.find(key);
		if (it == keyCodes.end())
		{
			returnInfo = "Unrecognized key name: " + key;
			return false;
		}
		auto keyCode = it->second;
		auto binding = &bindings[keyCode];

		// If no command was specified, unset the binding
		if (Arguments.size() == 1)
		{
			binding->command.clear();
			returnInfo = "Binding cleared.";
			return true;
		}

		// Get command information
		auto command = Arguments[1];
		if (command.empty())
		{
			returnInfo = "Invalid command";
			return false;
		}
		auto isHold = false;
		if (command[0] == '+')
		{
			command.erase(0, 1);
			isHold = true;
		}

		// Set the binding
		binding->key = key;
		binding->isHold = isHold;
		binding->command.clear();
		binding->command.push_back(command);
		binding->command.insert(binding->command.end(), Arguments.begin() + 2, Arguments.end());
		returnInfo = "Binding set.";
		return true;
	}

	void KeyboardUpdated(void* param)
	{
		auto& dorito = ElDorito::Instance();
		for (auto i = 0; i < Blam::eKeyCodes_Count; i++)
		{
			const auto binding = &bindings[i];
			if (binding->command.size() == 0)
				continue; // Key is not bound

			// Read the key and swallow it
			auto keyCode = static_cast<Blam::KeyCodes>(i);
			auto keyTicks = dorito.Modules.InputPatches.GetKeyTicks(keyCode, Blam::eInputTypeSpecial);
			dorito.Modules.InputPatches.Swallow(keyCode);

			// We're only interested in the key if it was just pressed or if
			// this is a hold binding and it was just released
			if (keyTicks > 1 || (keyTicks == 0 && !(binding->isHold && binding->active)))
				continue;

			auto command = binding->command;
			if (binding->isHold)
			{
				// The command is a hold binding - append an argument depending
				// on whether it was pressed or released
				if (keyTicks == 1)
					command.push_back("1");
				else if (keyTicks == 0)
					command.push_back("0");
				binding->active = (keyTicks > 0);
			}
			
			// Execute the command and print its result
			dorito.Modules.Console.PrintToConsole(dorito.Commands.Execute(command, true));
		}
	}
}

namespace Modules
{
	ModuleInput::ModuleInput() : ModuleBase("Input")
	{
		VarInputRawInput = AddVariableInt("RawInput", "rawinput", "Enables raw mouse input with no acceleration applied", eCommandFlagsArchived, 1, VariableInputRawInputUpdate);
		VarInputRawInput->ValueIntMin = 0;
		VarInputRawInput->ValueIntMax = 1;

		VarInputControllerIndex = AddVariableInt("ControllerIndex", "controllerindex", "Which controller index should be used for player 1", eCommandFlagsArchived, 0, VariableInputControllerIndexUpdate);
		VarInputControllerIndex->ValueIntMin = 0;
		VarInputControllerIndex->ValueIntMax = 3;

		AddModulePatches(
		{
			Patch("ControllerIndexNop", 0x5128F0, 0x90, 4)
		},
		{
			Hook("ControllerIndex", 0x5128EB, Input_ControllerIndexHook, HookType::Jmp)
		});

		AddCommand("Bind", "bind", "Binds a command to a key", eCommandFlagsNone, CommandBind, { "key", "[+]command", "arguments" });
		engine->OnEvent("Core", "Input.KeyboardUpdate", KeyboardUpdated);
	}
}

namespace
{
	// Key codes table
	std::map<std::string, Blam::KeyCodes> keyCodes =
	{
		{ "escape", Blam::eKeyCodesEscape },
		{ "f1", Blam::eKeyCodesF1 },
		{ "f2", Blam::eKeyCodesF2 },
		{ "f3", Blam::eKeyCodesF3 },
		{ "f4", Blam::eKeyCodesF4 },
		{ "f5", Blam::eKeyCodesF5 },
		{ "f6", Blam::eKeyCodesF6 },
		{ "f7", Blam::eKeyCodesF7 },
		{ "f8", Blam::eKeyCodesF8 },
		{ "f9", Blam::eKeyCodesF9 },
		{ "f10", Blam::eKeyCodesF10 },
		{ "f11", Blam::eKeyCodesF11 },
		{ "f12", Blam::eKeyCodesF12 },
		{ "printscreen", Blam::eKeyCodesPrintScreen },
		{ "f14", Blam::eKeyCodesF14 },
		{ "f15", Blam::eKeyCodesF15 },
		{ "tilde", Blam::eKeyCodesTilde },
		{ "1", Blam::eKeyCodes1 },
		{ "2", Blam::eKeyCodes2 },
		{ "3", Blam::eKeyCodes3 },
		{ "4", Blam::eKeyCodes4 },
		{ "5", Blam::eKeyCodes5 },
		{ "6", Blam::eKeyCodes6 },
		{ "7", Blam::eKeyCodes7 },
		{ "8", Blam::eKeyCodes8 },
		{ "9", Blam::eKeyCodes9 },
		{ "0", Blam::eKeyCodes0 },
		{ "minus", Blam::eKeyCodesMinus },
		{ "plus", Blam::eKeyCodesPlus },
		{ "back", Blam::eKeyCodesBack },
		{ "tab", Blam::eKeyCodesTab },
		{ "q", Blam::eKeyCodesQ },
		{ "w", Blam::eKeyCodesW },
		{ "e", Blam::eKeyCodesE },
		{ "r", Blam::eKeyCodesR },
		{ "t", Blam::eKeyCodesT },
		{ "y", Blam::eKeyCodesY },
		{ "u", Blam::eKeyCodesU },
		{ "i", Blam::eKeyCodesI },
		{ "o", Blam::eKeyCodesO },
		{ "p", Blam::eKeyCodesP },
		{ "lbracket", Blam::eKeyCodesLBracket },
		{ "rbracket", Blam::eKeyCodesRBracket },
		{ "pipe", Blam::eKeyCodesPipe },
		{ "capital", Blam::eKeyCodesCapital },
		{ "a", Blam::eKeyCodesA },
		{ "s", Blam::eKeyCodesS },
		{ "d", Blam::eKeyCodesD },
		{ "f", Blam::eKeyCodesF },
		{ "g", Blam::eKeyCodesG },
		{ "h", Blam::eKeyCodesH },
		{ "j", Blam::eKeyCodesJ },
		{ "k", Blam::eKeyCodesK },
		{ "l", Blam::eKeyCodesL },
		{ "colon", Blam::eKeyCodesColon },
		{ "quote", Blam::eKeyCodesQuote },
		{ "enter", Blam::eKeyCodesEnter },
		{ "lshift", Blam::eKeyCodesLShift },
		{ "z", Blam::eKeyCodesZ },
		{ "x", Blam::eKeyCodesX },
		{ "c", Blam::eKeyCodesC },
		{ "v", Blam::eKeyCodesV },
		{ "b", Blam::eKeyCodesB },
		{ "n", Blam::eKeyCodesN },
		{ "m", Blam::eKeyCodesM },
		{ "comma", Blam::eKeyCodesComma },
		{ "period", Blam::eKeyCodesPeriod },
		{ "question", Blam::eKeyCodesQuestion },
		{ "rshift", Blam::eKeyCodesRShift },
		{ "lcontrol", Blam::eKeyCodesLControl },
		{ "lmenu", Blam::eKeyCodesLmenu },
		{ "space", Blam::eKeyCodesSpace },
		{ "rmenu", Blam::eKeyCodesRmenu },
		{ "apps", Blam::eKeyCodesApps },
		{ "rcontrol", Blam::eKeyCodesRcontrol },
		{ "up", Blam::eKeyCodesUp },
		{ "down", Blam::eKeyCodesDown },
		{ "left", Blam::eKeyCodesLeft },
		{ "right", Blam::eKeyCodesRight },
		{ "insert", Blam::eKeyCodesInsert },
		{ "home", Blam::eKeyCodesHome },
		{ "pageup", Blam::eKeyCodesPageUp },
		{ "delete", Blam::eKeyCodesDelete },
		{ "end", Blam::eKeyCodesEnd },
		{ "pagedown", Blam::eKeyCodesPageDown },
		{ "numlock", Blam::eKeyCodesNumLock },
		{ "divide", Blam::eKeyCodesDivide },
		{ "multiply", Blam::eKeyCodesMultiply },
		{ "numpad0", Blam::eKeyCodesNumpad0 },
		{ "numpad1", Blam::eKeyCodesNumpad1 },
		{ "numpad2", Blam::eKeyCodesNumpad2 },
		{ "numpad3", Blam::eKeyCodesNumpad3 },
		{ "numpad4", Blam::eKeyCodesNumpad4 },
		{ "numpad5", Blam::eKeyCodesNumpad5 },
		{ "numpad6", Blam::eKeyCodesNumpad6 },
		{ "numpad7", Blam::eKeyCodesNumpad7 },
		{ "numpad8", Blam::eKeyCodesNumpad8 },
		{ "numpad9", Blam::eKeyCodesNumpad9 },
		{ "subtract", Blam::eKeyCodesSubtract },
		{ "add", Blam::eKeyCodesAdd },
		{ "numpadenter", Blam::eKeyCodesNumpadEnter },
		{ "decimal", Blam::eKeyCodesDecimal },
		{ "shift", Blam::eKeyCodesShift },
		{ "ctrl", Blam::eKeyCodesCtrl },
		{ "menu", Blam::eKeyCodesMenu },
	};
}