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

class IConsole001
{
public:
	virtual Command* AddCommand(Command command) = 0;
	virtual void FinishAddCommands() = 0;
	virtual Command* FindCommand(const std::string& name) = 0;

	virtual std::string ExecuteCommand(std::vector<std::string> command, bool isUserInput = false) = 0;
	virtual std::string ExecuteCommand(std::string command, bool isUserInput = false) = 0;
	virtual std::string ExecuteCommands(std::string& commands, bool isUserInput = false) = 0;
	virtual bool ExecuteCommandWithStatus(std::string command, bool isUserInput = false) = 0;
	virtual std::string ExecuteQueue() = 0;

	virtual bool GetVariableInt(const std::string& name, unsigned long& value) = 0;
	virtual bool GetVariableInt64(const std::string& name, unsigned long long& value) = 0;
	virtual bool GetVariableFloat(const std::string& name, float& value) = 0;
	virtual bool GetVariableString(const std::string& name, std::string& value) = 0;

	virtual VariableSetReturnValue SetVariable(const std::string& name, std::string& value, std::string& previousValue) = 0;
	virtual VariableSetReturnValue SetVariable(Command* command, std::string& value, std::string& previousValue) = 0;

	virtual std::string GenerateHelpText(std::string moduleFilter = "") = 0;

	virtual std::string SaveVariables() = 0;
};

#define CONSOLE_INTERFACE_VERSION001 "GameConsole001"