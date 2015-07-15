#pragma once
#include <string>
#include <vector>

enum class VariableSetReturnValue
{
	Success,
	Error,
	OutOfRange,
	InvalidArgument,
};

enum class CommandType
{
	Command,
	VariableInt,
	VariableInt64,
	VariableFloat,
	VariableString
};

enum CommandFlags
{
	eCommandFlagsNone,
	eCommandFlagsCheat = 1 << 0, // only allow this command on cheat-enabled servers, whenever they get implemented
	eCommandFlagsReplicated = 1 << 1, // value of this variable should be output into the server info JSON, clients should update their variable to match the one in JSON
	eCommandFlagsArchived = 1 << 2, // value of this variable should be written when using WriteConfig
	eCommandFlagsDontUpdateInitial = 1 << 3, // don't call the update event when the variable is first being initialized
	eCommandFlagsHidden = 1 << 4, // hide this command/var from the help listing
	eCommandFlagsRunOnMainMenu = 1 << 5, // if run at startup queue the command until the main menu is shown
	eCommandFlagsHostOnly = 1 << 6, // only execute the command if the user is host
	eCommandFlagsOmitValueInList = 1 << 7, // omit the variables value in help listing
	eCommandFlagsInternal = 1 << 8  // disallow the user from using this command, only internal ExecuteCommand calls can use it
};

typedef bool(*CommandUpdateFunc)(const std::vector<std::string>& Arguments, std::string& returnInfo);

struct Command
{
	std::string Name; // modulename is added to this too, makes looking it up easier
	std::string ModuleName;
	std::string ShortName; // because some people can't be bothered to type in module names
	std::string Description;

	CommandFlags Flags;
	CommandType Type;

	CommandUpdateFunc UpdateEvent;

	unsigned long ValueInt = 0;
	unsigned long long ValueInt64 = 0;
	float ValueFloat = 0;
	std::string ValueString;

	unsigned long DefaultValueInt = 0;
	unsigned long long DefaultValueInt64 = 0;
	float DefaultValueFloat = 0;
	std::string DefaultValueString;

	unsigned long ValueIntMin = 0;
	unsigned long ValueIntMax = 0;
	unsigned long long ValueInt64Min = 0;
	unsigned long long ValueInt64Max = 0;
	float ValueFloatMin = 0;
	float ValueFloatMax = 0;

	// CommandArgs is only used for help text, commands will have to parse the args themselves

	std::vector<std::string> CommandArgs; // arg names can't contain a space, since space/EOL is used to find where the command name ends and the command description begins
	// this is for the help function, so you can specify an arg here like "parameter_1 This parameter controls the first parameter"
	// in the help text this will be printed like "Usage: Game.Command <parameter_1>\r\nparameter_1: This parameter controls the first parameter."
	// also don't end descriptions with a period since it'll be added in later
	// modifiers might be added to the name later, so you can do things like "parameter_1:o" to signify the parameter is optional
};

/*
if you want to make changes to this interface create a new IConsole002 class and make them there, then edit GameConsole class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IConsole001
{
public:
	/// <summary>
	/// Adds a command to the console commands list.
	/// </summary>
	/// <param name="command">The command to add.</param>
	/// <returns>A pointer to the command, if added successfully.</returns>
	virtual Command* AddCommand(Command command) = 0;

	/// <summary>
	/// Finalizes adding all commands: calls the update event for each command, ensuring default values are applied.
	/// </summary>
	virtual void FinishAddCommands() = 0;

	/// <summary>
	/// Finds a command based on its name.
	/// </summary>
	/// <param name="name">The name of the command.</param>
	/// <returns>A pointer to the command, if found.</returns>
	virtual Command* FindCommand(const std::string& name) = 0;

	/// <summary>
	/// Executes a command string (vector splits spaces?)
	/// </summary>
	/// <param name="command">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>The output of the executed command.</returns>
	virtual std::string ExecuteCommand(std::vector<std::string> command, bool isUserInput = false) = 0;

	/// <summary>
	/// Executes a command string
	/// </summary>
	/// <param name="command">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>The output of the executed command.</returns>
	virtual std::string ExecuteCommand(std::string command, bool isUserInput = false) = 0;

	/// <summary>
	/// Executes a list of commands, seperated by new lines
	/// </summary>
	/// <param name="commands">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>Whether the command executed successfully.</returns>
	virtual std::string ExecuteCommands(std::string& commands, bool isUserInput = false) = 0;

	/// <summary>
	/// Executes a command string, returning a bool indicating success.
	/// </summary>
	/// <param name="command">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>Whether the command executed successfully.</returns>
	virtual bool ExecuteCommandWithStatus(std::string command, bool isUserInput = false) = 0;

	/// <summary>
	/// Executes the command queue.
	/// </summary>
	/// <returns>Results of the executed commands.</returns>
	virtual std::string ExecuteQueue() = 0;

	/// <summary>
	/// Gets the value of an int variable.
	/// </summary>
	/// <param name="name">The name of the variable.</param>
	/// <param name="value">Returns the value of the variable.</param>
	/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
	virtual bool GetVariableInt(const std::string& name, unsigned long& value) = 0;

	/// <summary>
	/// Gets the value of an int64 variable.
	/// </summary>
	/// <param name="name">The name of the variable.</param>
	/// <param name="value">Returns the value of the variable.</param>
	/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
	virtual bool GetVariableInt64(const std::string& name, unsigned long long& value) = 0;

	/// <summary>
	/// Gets the value of a float variable.
	/// </summary>
	/// <param name="name">The name of the variable.</param>
	/// <param name="value">Returns the value of the variable.</param>
	/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
	virtual bool GetVariableFloat(const std::string& name, float& value) = 0;

	/// <summary>
	/// Gets the value of a string variable.
	/// </summary>
	/// <param name="name">The name of the variable.</param>
	/// <param name="value">Returns the value of the variable.</param>
	/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
	virtual bool GetVariableString(const std::string& name, std::string& value) = 0;

	/// <summary>
	/// Sets a variable from a string, string is converted to the proper variable type.
	/// </summary>
	/// <param name="name">The name of the variable.</param>
	/// <param name="value">The value to set.</param>
	/// <param name="previousValue">The previous value of the variable.</param>
	/// <returns>VariableSetReturnValue</returns>
	virtual VariableSetReturnValue SetVariable(const std::string& name, std::string& value, std::string& previousValue) = 0;

	/// <summary>
	/// Sets a variable from a string, string is converted to the proper variable type.
	/// </summary>
	/// <param name="command">The variable to update.</param>
	/// <param name="value">The value to set.</param>
	/// <param name="previousValue">The previous value of the variable.</param>
	/// <returns>VariableSetReturnValue</returns>
	virtual VariableSetReturnValue SetVariable(Command* command, std::string& value, std::string& previousValue) = 0;

	/// <summary>
	/// Generates help text.
	/// </summary>
	/// <param name="moduleFilter">If set, only commands/variables belonging to this module will be printed.</param>
	/// <returns>Help text.</returns>
	virtual std::string GenerateHelpText(std::string moduleFilter = "") = 0;

	/// <summary>
	/// Writes each variables name and value to a string.
	/// </summary>
	/// <returns>Each variables name and value.</returns>
	virtual std::string SaveVariables() = 0;
};

#define CONSOLE_INTERFACE_VERSION001 "GameConsole001"
