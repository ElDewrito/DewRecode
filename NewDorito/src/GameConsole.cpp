#include "GameConsole.hpp"
#include <algorithm>
#include <sstream>

/// <summary>
/// Finds a command based on its name.
/// </summary>
/// <param name="name">The name of the command.</param>
/// <returns>A pointer to the command, if found.</returns>
Command* GameConsole::FindCommand(const std::string& name)
{
	for (auto it = Commands.begin(); it < Commands.end(); it++)
		if ((it->Name.length() > 0 && !_stricmp(it->Name.c_str(), name.c_str())) || (it->ShortName.length() > 0 && !_stricmp(it->ShortName.c_str(), name.c_str())))
			return &(*it);

	return nullptr;
}

/// <summary>
/// Adds a command to the console commands list.
/// </summary>
/// <param name="command">The command to add.</param>
/// <returns>A pointer to the command, if added successfully.</returns>
Command* GameConsole::AddCommand(Command command)
{
	if (FindCommand(command.Name) || FindCommand(command.ShortName))
		return nullptr;

	this->Commands.push_back(command);

	return &this->Commands.back();
}

/// <summary>
/// Finalizes adding all commands: calls the update event for each command, ensuring default values are applied.
/// </summary>
void GameConsole::FinishAddCommands()
{
	for (auto command : Commands)
	{
		if (command.Type != CommandType::Command && (command.Flags & eCommandFlagsDontUpdateInitial) != eCommandFlagsDontUpdateInitial)
			if (command.UpdateEvent)
				command.UpdateEvent(std::vector<std::string>(), std::string());
	}
}

/// <summary>
/// Executes a command string (vector splits spaces?)
/// </summary>
/// <param name="command">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>The output of the executed command.</returns>
std::string GameConsole::ExecuteCommand(std::vector<std::string> command, bool isUserInput)
{
	std::string commandStr = "";
	for (auto cmd : command)
		commandStr += "\"" + cmd + "\" ";

	return ExecuteCommand(commandStr, isUserInput);
}

/// <summary>
/// Executes a command string, returning a bool indicating success.
/// </summary>
/// <param name="command">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>Whether the command executed successfully.</returns>
bool GameConsole::ExecuteCommandWithStatus(std::string command, bool isUserInput)
{
	int numArgs = 0;
	auto args = CommandLineToArgvA((char*)command.c_str(), &numArgs);

	if (numArgs <= 0)
		return false;

	auto cmd = FindCommand(args[0]);
	if (!cmd || (isUserInput && cmd->Flags & eCommandFlagsInternal))
		return false;

	std::vector<std::string> argsVect;
	if (numArgs > 1)
		for (int i = 1; i < numArgs; i++)
			argsVect.push_back(args[i]);

	if (cmd->Type == CommandType::Command)
	{
		cmd->UpdateEvent(argsVect, std::string()); // if it's a command call it and return
		return true;
	}

	std::string previousValue;
	auto updateRet = SetVariable(cmd, (numArgs > 1 ? argsVect[0] : ""), previousValue);

	if (updateRet != VariableSetReturnValue::Success)
		return false;

	if (numArgs <= 1)
		return true;

	if (!cmd->UpdateEvent)
		return true; // no update event, so we'll just return with what we set the value to

	auto ret = cmd->UpdateEvent(argsVect, std::string());

	if (ret) // error, revert the variable
		return true;

	// error, revert the variable
	this->SetVariable(cmd, previousValue, std::string());
	return false;
}

/// <summary>
/// Executes a command string
/// </summary>
/// <param name="command">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>The output of the executed command.</returns>
std::string GameConsole::ExecuteCommand(std::string command, bool isUserInput)
{
	int numArgs = 0;
	auto args = CommandLineToArgvA((char*)command.c_str(), &numArgs);

	if (numArgs <= 0)
		return "Invalid input";

	auto cmd = FindCommand(args[0]);
	if (!cmd || (isUserInput && cmd->Flags & eCommandFlagsInternal))
		return "Command/Variable not found";

	if ((cmd->Flags & eCommandFlagsRunOnMainMenu))// && !ElDorito::Instance().GameHasMenuShown)
	{
		queuedCommands.push_back(command);
		return "Command queued until mainmenu shows";
	}

	if ((cmd->Flags & eCommandFlagsHostOnly))// && !ElDorito::Instance().IsHostPlayer())
		return "Only a player hosting a game can use this command";

	std::vector<std::string> argsVect;
	if (numArgs > 1)
		for (int i = 1; i < numArgs; i++)
			argsVect.push_back(args[i]);

	if (cmd->Type == CommandType::Command)
	{
		std::string retInfo;
		cmd->UpdateEvent(argsVect, retInfo); // if it's a command call it and return
		return retInfo;
	}

	std::string previousValue;
	auto updateRet = SetVariable(cmd, (numArgs > 1 ? argsVect[0] : ""), previousValue);

	switch (updateRet)
	{
	case VariableSetReturnValue::Error:
		return "Command/Variable not found";
	case VariableSetReturnValue::InvalidArgument:
		return "Invalid value";
	case VariableSetReturnValue::OutOfRange:
		if (cmd->Type == CommandType::VariableInt)
			return "Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueIntMin) + ".." + std::to_string(cmd->ValueIntMax) + "]";
		if (cmd->Type == CommandType::VariableInt64)
			return "Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueInt64Min) + ".." + std::to_string(cmd->ValueInt64Max) + "]";
		if (cmd->Type == CommandType::VariableFloat)
			return "Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueFloatMin) + ".." + std::to_string(cmd->ValueFloatMax) + "]";

		return "Value " + argsVect[0] + " out of range [this shouldn't be happening!]";
	}

	// special case for blanking strings
	if (cmd->Type == CommandType::VariableString && numArgs > 1 && argsVect[0].empty())
		cmd->ValueString = "";

	if (numArgs <= 1)
		return previousValue;

	if (!cmd->UpdateEvent)
		return previousValue + " -> " + cmd->ValueString; // no update event, so we'll just return with what we set the value to

	std::string retVal;
	auto ret = cmd->UpdateEvent(argsVect, retVal);

	if (!ret) // error, revert the variable
		this->SetVariable(cmd, previousValue, std::string());

	if (retVal.length() <= 0)
		return previousValue + " -> " + cmd->ValueString;

	return retVal;
}

/// <summary>
/// Gets the value of an int variable.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">Returns the value of the variable.</param>
/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
bool GameConsole::GetVariableInt(const std::string& name, unsigned long& value)
{
	auto command = FindCommand(name);
	if (!command || command->Type != CommandType::VariableInt)
		return false;

	value = command->ValueInt;
	return true;
}

/// <summary>
/// Gets the value of an int64 variable.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">Returns the value of the variable.</param>
/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
bool GameConsole::GetVariableInt64(const std::string& name, unsigned long long& value)
{
	auto command = FindCommand(name);
	if (!command || command->Type != CommandType::VariableInt64)
		return false;

	value = command->ValueInt64;
	return true;
}

/// <summary>
/// Gets the value of a float variable.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">Returns the value of the variable.</param>
/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
bool GameConsole::GetVariableFloat(const std::string& name, float& value)
{
	auto command = FindCommand(name);
	if (!command || command->Type != CommandType::VariableFloat)
		return false;

	value = command->ValueFloat;
	return true;
}

/// <summary>
/// Gets the value of a string variable.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">Returns the value of the variable.</param>
/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
bool GameConsole::GetVariableString(const std::string& name, std::string& value)
{
	auto command = FindCommand(name);
	if (!command || command->Type != CommandType::VariableString)
		return false;

	value = command->ValueString;
	return true;
}

/// <summary>
/// Sets a variable from a string, string is converted to the proper variable type.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">The value to set.</param>
/// <param name="previousValue">The previous value of the variable.</param>
/// <returns>VariableSetReturnValue</returns>
VariableSetReturnValue GameConsole::SetVariable(const std::string& name, std::string& value, std::string& previousValue)
{
	auto command = FindCommand(name);
	if (!command)
		return VariableSetReturnValue::Error;

	return SetVariable(command, value, previousValue);
}

/// <summary>
/// Sets a variable from a string, string is converted to the proper variable type.
/// </summary>
/// <param name="command">The variable to update.</param>
/// <param name="value">The value to set.</param>
/// <param name="previousValue">The previous value of the variable.</param>
/// <returns>VariableSetReturnValue</returns>
VariableSetReturnValue GameConsole::SetVariable(Command* command, std::string& value, std::string& previousValue)
{
	try {
		switch (command->Type)
		{
		case CommandType::VariableString:
			previousValue = command->ValueString;
			if (value.length() > 0)
				command->ValueString = value;
			break;
		case CommandType::VariableInt:
			previousValue = std::to_string(command->ValueInt);
			if (value.length() > 0)
			{
				auto newValue = std::stoul(value, 0, 0);
				if ((command->ValueIntMin || command->ValueIntMax) && (newValue < command->ValueIntMin || newValue > command->ValueIntMax))
					return VariableSetReturnValue::OutOfRange;

				command->ValueInt = newValue;
				command->ValueString = std::to_string(command->ValueInt); // set the ValueString too so we can print the value out easier
			}
			break;
		case CommandType::VariableInt64:
			previousValue = std::to_string(command->ValueInt);
			if (value.length() > 0)
			{
				auto newValue = std::stoull(value, 0, 0);
				if ((command->ValueInt64Min || command->ValueInt64Max) && (newValue < command->ValueInt64Min || newValue > command->ValueInt64Max))
					return VariableSetReturnValue::OutOfRange;

				command->ValueInt64 = newValue;
				command->ValueString = std::to_string(command->ValueInt64); // set the ValueString too so we can print the value out easier
			}
			break;
		case CommandType::VariableFloat:
			previousValue = std::to_string(command->ValueFloat);
			if (value.length() > 0)
			{
				auto newValue = std::stof(value, 0);
				if ((command->ValueFloatMin || command->ValueFloatMax) && (newValue < command->ValueFloatMin || newValue > command->ValueFloatMax))
					return VariableSetReturnValue::OutOfRange;

				command->ValueFloat = newValue;
				command->ValueString = std::to_string(command->ValueFloat); // set the ValueString too so we can print the value out easier
			}
			break;
		}
	}
	catch (std::invalid_argument)
	{
		return VariableSetReturnValue::InvalidArgument;
	}
	catch (std::out_of_range)
	{
		return VariableSetReturnValue::InvalidArgument;
	}

	return VariableSetReturnValue::Success;
}

bool compare_commands(const Command& lhs, const Command& rhs)
{
	return lhs.Name < rhs.Name;
}

/// <summary>
/// Generates help text.
/// </summary>
/// <param name="moduleFilter">If set, only commands/variables belonging to this module will be printed.</param>
/// <returns>Help text.</returns>
std::string GameConsole::GenerateHelpText(std::string moduleFilter)
{
	std::deque<Command> tempCommands(Commands);

	std::sort(tempCommands.begin(), tempCommands.end(), compare_commands);
	std::stringstream ss;
	std::stringstream hasParent; // store commands with a parent module seperately, so they can be added to the main stringstream after the non-parent commands
	for (auto cmd : tempCommands)
	{
		if (cmd.Flags & eCommandFlagsHidden || cmd.Flags & eCommandFlagsInternal)
			continue;

		if (!moduleFilter.empty() && (cmd.ModuleName.empty() || _stricmp(cmd.ModuleName.c_str(), moduleFilter.c_str())))
			continue;

		std::string helpText = cmd.Name;
		if (cmd.Type != CommandType::Command && !(cmd.Flags & eCommandFlagsOmitValueInList))
			helpText += " " + cmd.ValueString;

		helpText += " - " + cmd.Description;

		if (cmd.ModuleName.length() > 0)
			hasParent << helpText << std::endl;
		else
			ss << helpText << std::endl;
	}

	ss << hasParent.str();

	return ss.str();
}

/// <summary>
/// Writes each variables name and value to a string.
/// </summary>
/// <returns>Each variables name and value.</returns>
std::string GameConsole::SaveVariables()
{
	std::stringstream ss;
	for (auto cmd : Commands)
	{
		if (cmd.Type == CommandType::Command || !(cmd.Flags & eCommandFlagsArchived) || (cmd.Flags & eCommandFlagsInternal))
			continue;

		ss << cmd.Name << " \"" << cmd.ValueString << "\"" << std::endl;
	}
	return ss.str();
}

/// <summary>
/// Executes a list of commands, seperated by new lines
/// </summary>
/// <param name="commands">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>Whether the command executed successfully.</returns>
std::string GameConsole::ExecuteCommands(std::string& commands, bool isUserInput)
{
	std::istringstream stream(commands);
	std::stringstream ss;
	std::string line;
	int lineIdx = 0;
	while (std::getline(stream, line))
	{
		if (!this->ExecuteCommandWithStatus(line, isUserInput))
		{
			ss << "Error at line " << lineIdx << std::endl;
		}
		lineIdx++;
	}
	return ss.str();
}

/// <summary>
/// Executes the command queue.
/// </summary>
/// <returns>Results of the executed commands.</returns>
std::string GameConsole::ExecuteQueue()
{
	std::stringstream ss;
	for (auto cmd : queuedCommands)
	{
		ss << ExecuteCommand(cmd, true) << std::endl;
	}
	queuedCommands.clear();
	return ss.str();
}

namespace
{
	char** CommandLineToArgvA(char* CmdLine, int* _argc)
	{
		char** argv;
		char*  _argv;
		unsigned long len;
		unsigned long argc;
		char   a;
		unsigned long i, j;

		bool in_QM;
		bool in_TEXT;
		bool in_SPACE;

		len = strlen(CmdLine);
		i = ((len + 2) / 2)*sizeof(void*) + sizeof(void*);

		argv = (char**)malloc(i + (len + 2)*sizeof(char));
		//argv = (char**)GlobalAlloc(GMEM_FIXED,
		//	i + (len + 2)*sizeof(char));

		if (!argv)
			return 0;

		_argv = (char*)(((unsigned char*)argv) + i);

		argc = 0;
		argv[argc] = _argv;
		in_QM = false;
		in_TEXT = false;
		in_SPACE = true;
		i = 0;
		j = 0;

		while (a = CmdLine[i]) {
			if (in_QM) {
				if (a == '\"') {
					in_QM = false;
				}
				else {
					_argv[j] = a;
					j++;
				}
			}
			else {
				switch (a) {
				case '\"':
					in_QM = true;
					in_TEXT = true;
					if (in_SPACE) {
						argv[argc] = _argv + j;
						argc++;
					}
					in_SPACE = false;
					break;
				case ' ':
				case '\t':
				case '\n':
				case '\r':
					if (in_TEXT) {
						_argv[j] = '\0';
						j++;
					}
					in_TEXT = false;
					in_SPACE = true;
					break;
				default:
					in_TEXT = true;
					if (in_SPACE) {
						argv[argc] = _argv + j;
						argc++;
					}
					_argv[j] = a;
					j++;
					in_SPACE = false;
					break;
				}
			}
			i++;
		}
		_argv[j] = '\0';
		argv[argc] = NULL;

		(*_argc) = argc;
		return argv;
	}
}