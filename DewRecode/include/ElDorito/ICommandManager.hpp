#pragma once
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <bitset>

// Holds information about a command bound to a key
struct KeyBinding
{
	//std::vector<std::string> command; // If this is empty, no command is bound
	//bool isHold; // True if the command binds to a boolean variable
	std::string command;
	bool active; // True if this is a hold command and the key is down
	std::string key; // the key that corresponds to this code, easier than looking it up again
};

enum class BindingReturnValue
{
	Success,
	ClearedBinding,
	UnknownKey,
	InvalidArgument,
};

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
	VariableString,
	Count
};

enum CommandFlags
{
	eCommandFlagsNone,
	eCommandFlagsCheat = 1 << 0,				// only allow the command/variable to be run/changed if Server.Cheats is set to 1
	eCommandFlagsReplicated = 1 << 1,			// value of this variable should be output into the server info JSON, clients should update their variable to match the one in JSON
	eCommandFlagsArchived = 1 << 2,				// value of this variable should be written when using WriteConfig
	eCommandFlagsDontUpdateInitial = 1 << 3,	// don't call the update event when the variable is first being initialized
	eCommandFlagsHidden = 1 << 4,				// hide this command/var from the help listing
	eCommandFlagsRunOnMainMenu = 1 << 5,		// if run at startup queue the command until the main menu is shown
	eCommandFlagsMustBeHosting = 1 << 6,		// only execute the command if the user is host
	eCommandFlagsOmitValueInList = 1 << 7,		// omit the variables value in help listing
	eCommandFlagsInternal = 1 << 8,				// disallow the user from using this command, only internal ExecuteCommand calls can use it
};

enum class CommandExecuteResult
{
	Success,
	InvalidInput,
	NotFound,
	Queued,
	CheatCommand,
	MustBeHost,
	MustBeHostOrMainMenu,
	CommandFailed,
	InvalidValue,
	OutOfRange,
	Count
};

const std::string CommandExecuteResultString[(uint32_t)CommandExecuteResult::Count] =
{
	"Success",
	"Invalid input",
	"Not found",
	"Queued",
	"Cheat command",
	"Must be host",
	"Must be host or mainmenu",
	"Command failed",
	"Invalid value",
	"Out of range"
};

typedef uint32_t SyncID;
const size_t MaxStringLength = 512;

struct SyncUpdatePacketVar
{
	SyncID ID;
	CommandType Type;
	union
	{
		uint64_t Int;
		float Float;
		char String[MaxStringLength + 1];
	} Value;
};

//typedef bool(*CommandUpdateFunc)(const std::vector<std::string>& Arguments, ICommandContext& context);
class ICommandProvider;
typedef std::function<bool(const std::vector<std::string>& Arguments, ICommandContext& context)> CommandUpdateFunc;
#define BIND_COMMAND(classPtr, funcPtr) std::bind(funcPtr, classPtr, std::placeholders::_1, std::placeholders::_2)

struct Command;

/*
if you want to make changes to this interface create a new ICommandManager002 class and make them there, then edit CommandManager class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class ICommandManager001
{
public:
	/// <summary>
	/// Adds a command to the console commands list.
	/// </summary>
	/// <param name="command">The command to add.</param>
	/// <returns>A pointer to the command, if added successfully.</returns>
	virtual Command* Add(Command command) = 0;

	/// <summary>
	/// Finalizes adding all commands: calls the update event for each command, ensuring default values are applied.
	/// </summary>
	virtual void FinishAdd() = 0;

	/// <summary>
	/// Finds a command based on its name.
	/// </summary>
	/// <param name="name">The name of the command.</param>
	/// <returns>A pointer to the command, if found.</returns>
	virtual Command* Find(const std::string& name) = 0;

	virtual const std::deque<Command>& GetList() = 0;

	/// <summary>
	/// Executes a command string (vector splits spaces?)
	/// </summary>
	/// <param name="command">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>Whether the command executed successfully.</returns>
	virtual CommandExecuteResult Execute(const std::vector<std::string>& command, ICommandContext& context) = 0;

	/// <summary>
	/// Executes a command string
	/// </summary>
	/// <param name="command">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>Whether the command executed successfully.</returns>
	virtual CommandExecuteResult Execute(const std::string& command, ICommandContext& context) = 0;

	/// <summary>
	/// Executes a list of commands, seperated by new lines
	/// </summary>
	/// <param name="commands">The command string.</param>
	/// <param name="isUserInput">Whether the command came from the user or internally.</param>
	/// <returns>Whether the commands executed successfully.</returns>
	virtual bool ExecuteList(const std::string& commands, ICommandContext& context) = 0;

	/// <summary>
	/// Executes the command queue.
	/// </summary>
	/// <returns>Whether the commands executed successfully.</returns>
	virtual bool ExecuteQueue(ICommandContext& context) = 0;

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
	virtual VariableSetReturnValue SetVariable(const std::string& name, const std::string& value, std::string& previousValue) = 0;

	/// <summary>
	/// Sets a variable from a string, string is converted to the proper variable type.
	/// </summary>
	/// <param name="command">The variable to update.</param>
	/// <param name="value">The value to set.</param>
	/// <param name="previousValue">The previous value of the variable.</param>
	/// <returns>VariableSetReturnValue</returns>
	virtual VariableSetReturnValue SetVariable(Command* command, const std::string& value, std::string& previousValue) = 0;

	/// <summary>
	/// Generates help text.
	/// </summary>
	/// <param name="moduleFilter">If set, only commands/variables belonging to this module will be printed.</param>
	/// <returns>Help text.</returns>
	virtual std::string GenerateHelpText(const std::string& moduleFilter = "") = 0;

	/// <summary>
	/// Generates help text for a command.
	/// </summary>
	/// <param name="command">The command to generate help text for.</param>
	/// <returns>Help text.</returns>
	virtual std::string GenerateHelpText(const Command& command) = 0;

	/// <summary>
	/// Writes each variables name and value to a string.
	/// </summary>
	/// <returns>Each variables name and value.</returns>
	virtual std::string SaveVariables() = 0;

	/// <summary>
	/// Adds or clears a keyboard binding.
	/// </summary>
	/// <param name="key">The key to bind.</param>
	/// <param name="command">The command to run (empty if clearing).</param>
	/// <returns>BindingReturnValue</returns>
	virtual BindingReturnValue AddBinding(const std::string& key, const std::string& command) = 0;

	/// <summary>
	/// Gets the binding for a key.
	/// </summary>
	/// <param name="key">The key.</param>
	/// <returns>A pointer to the KeyBinding struct for this key.</returns>
	virtual KeyBinding* GetBinding(const std::string& key) = 0;

	/// <summary>
	/// Gets the binding for a keycode.
	/// </summary>
	/// <param name="keyCode">The key code.</param>
	/// <returns>A pointer to the KeyBinding struct for this key code.</returns>
	virtual KeyBinding* GetBinding(int keyCode) = 0;

	virtual ICommandContext& GetLogFileContext() = 0;
};

#define COMMANDMANAGER_INTERFACE_VERSION001 "CommandManager001"

/* use this class if you're updating ICommandManager after we've released a build
also update the ICommandManager typedef and COMMANDMANAGER_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class ICommandManager002 : public ICommandManager001
{

};

#define COMMANDMANAGER_INTERFACE_VERSION002 "CommandManager002"*/

typedef ICommandManager001 ICommandManager;
#define COMMANDMANAGER_INTERFACE_LATEST COMMANDMANAGER_INTERFACE_VERSION001

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

	static Command CreateCommand(const std::string& nameSpace, const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, CommandUpdateFunc updateEvent, std::initializer_list<const std::string> arguments = {})
	{
		Command command;
		command.Name = nameSpace + "." + name;
		command.ModuleName = nameSpace;
		command.ShortName = shortName;
		command.Description = description;

		for (auto arg : arguments)
			command.CommandArgs.push_back(arg);

		if (nameSpace.empty())
			command.Name = name;

		command.Flags = flags;
		command.Type = CommandType::Command;
		command.UpdateEvent = updateEvent;

		return command;
	}

	static Command CreateVariableInt(const std::string& nameSpace, const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long defaultValue = 0, CommandUpdateFunc updateEvent = nullptr)
	{
		Command command;
		command.Name = nameSpace + "." + name;
		command.ModuleName = nameSpace;
		command.ShortName = shortName;
		command.Description = description;

		if (nameSpace.empty())
			command.Name = name;

		command.Flags = flags;
		command.Type = CommandType::VariableInt;
		command.DefaultValueInt = defaultValue;
		command.ValueInt = defaultValue;
		command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
		command.DefaultValueString = command.ValueString;
		command.UpdateEvent = updateEvent;

		return command;
	}

	static Command CreateVariableInt64(const std::string& nameSpace, const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long long defaultValue = 0, CommandUpdateFunc updateEvent = nullptr)
	{
		Command command;
		command.Name = nameSpace + "." + name;
		command.ModuleName = nameSpace;
		command.ShortName = shortName;
		command.Description = description;

		if (nameSpace.empty())
			command.Name = name;

		command.Flags = flags;
		command.Type = CommandType::VariableInt64;
		command.DefaultValueInt64 = defaultValue;
		command.ValueInt64 = defaultValue;
		command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
		command.DefaultValueString = command.ValueString;
		command.UpdateEvent = updateEvent;

		return command;
	}

	static Command CreateVariableFloat(const std::string& nameSpace, const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, float defaultValue = 0, CommandUpdateFunc updateEvent = nullptr)
	{
		Command command;
		command.Name = nameSpace + "." + name;
		command.ModuleName = nameSpace;
		command.ShortName = shortName;
		command.Description = description;

		if (nameSpace.empty())
			command.Name = name;

		command.Flags = flags;
		command.Type = CommandType::VariableFloat;
		command.DefaultValueFloat = defaultValue;
		command.ValueFloat = defaultValue;
		command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
		command.DefaultValueString = command.ValueString;
		command.UpdateEvent = updateEvent;

		return command;
	}

	static Command CreateVariableString(const std::string& nameSpace, const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, const std::string& defaultValue = "", CommandUpdateFunc updateEvent = nullptr)
	{
		Command command;
		command.Name = nameSpace + "." + name;
		command.ModuleName = nameSpace;
		command.ShortName = shortName;
		command.Description = description;

		if (nameSpace.empty())
			command.Name = name;

		command.Flags = flags;
		command.Type = CommandType::VariableString;
		command.DefaultValueString = defaultValue;
		command.ValueString = defaultValue;
		command.DefaultValueString = command.ValueString;
		command.UpdateEvent = updateEvent;

		return command;
	}

	bool Compare(Command* command)
	{
		if (Type != command->Type)
			throw std::runtime_error("Cannot compare variables of different types");
		switch (Type)
		{
		case CommandType::VariableInt:
			return ValueInt == command->ValueInt;
		case CommandType::VariableInt64:
			return ValueInt64 == command->ValueInt64;
		case CommandType::VariableFloat:
			return ValueFloat == command->ValueFloat;
		case CommandType::VariableString:
			return ValueString == command->ValueString;
		default:
			throw std::runtime_error("Unsupported variable type");
		}
	}

	void Reset()
	{
		ValueString = DefaultValueString;
		switch (Type)
		{
		case CommandType::VariableInt:
			ValueInt = DefaultValueInt;
			break;
		case CommandType::VariableInt64:
			ValueInt64 = DefaultValueInt64;
			break;
		case CommandType::VariableFloat:
			ValueFloat = DefaultValueFloat;
			break;
		case CommandType::VariableString:
			break;
		default:
			throw std::runtime_error("Unsupported variable type");
		}

		if (UpdateEvent)
		{
			int retCode = 0;
			ICommandManager* engine = reinterpret_cast<ICommandManager*>(CreateInterface(COMMANDMANAGER_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create command interface");

			UpdateEvent({ ValueString }, engine->GetLogFileContext());
		}
	}
	void CopyFrom(Command* command)
	{
		if (Type != command->Type)
			throw std::runtime_error("Cannot compare variables of different types");

		ValueString = command->ValueString;
		switch (Type)
		{
		case CommandType::VariableInt:
			ValueInt = command->ValueInt;
			break;
		case CommandType::VariableInt64:
			ValueInt64 = command->ValueInt64;
			break;
		case CommandType::VariableFloat:
			ValueFloat = command->ValueFloat;
			break;
		case CommandType::VariableString:
			break;
		default:
			throw std::runtime_error("Unsupported variable type");
		}

		if (UpdateEvent)
		{
			int retCode = 0;
			ICommandManager* engine = reinterpret_cast<ICommandManager*>(CreateInterface(COMMANDMANAGER_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create command interface");

			UpdateEvent({ ValueString }, engine->GetLogFileContext());
		}
	}

	void CopyFrom(const SyncUpdatePacketVar* var)
	{
		// Note: unlike the other overload, this can't throw or else it could
		// be abused to crash someone's game. Validation is done during
		// deserialization.
		if (Type != var->Type)
			return;

		switch (Type)
		{
		case CommandType::VariableInt:
			ValueInt = static_cast<uint32_t>(var->Value.Int);
			ValueString = std::to_string(ValueInt);
			break;
		case CommandType::VariableInt64:
			ValueInt64 = var->Value.Int;
			ValueString = std::to_string(ValueInt64);
			break;
		case CommandType::VariableFloat:
			ValueFloat = var->Value.Float;
			ValueString = std::to_string(ValueFloat);
			break;
		case CommandType::VariableString:
			ValueString = var->Value.String;
			break;
		default:
			return;
		}
		if (UpdateEvent)
		{
			int retCode = 0;
			ICommandManager* engine = reinterpret_cast<ICommandManager*>(CreateInterface(COMMANDMANAGER_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create command interface");

			UpdateEvent({ ValueString }, engine->GetLogFileContext());
		}
	}
};

// Binds a server variable to a client variable.
struct SynchronizationBinding
{
	SyncID ID;
	Command *ServerVariable;
	Command *ClientVariable;

	// This is used to determine which peers have updated their binding.
	// If a peer's bit is set to 1, then it's up-to-date.
	std::bitset<Blam::Network::MaxPeers> SynchronizedPeers;
};