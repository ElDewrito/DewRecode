#include "CommandManager.hpp"
#include <algorithm>
#include <sstream>
#include "ElDorito.hpp"
#include <ElDorito/Blam/BlamNetwork.hpp>

namespace
{
	// Maps key names to key code values
	extern std::map<std::string, Blam::Input::KeyCodes> keyCodes;

	std::shared_ptr<SyncUpdatePacketSender> updateSender;
}


SyncID GenerateID(const SynchronizationBinding &binding)
{
	// Hash a concatenation of the server var name and the client var name
	auto str = binding.ServerVariable->Name + binding.ClientVariable->Name;
	SyncID result;
	if (!ElDorito::Instance().Utils.Hash32(str, &result))
		throw std::runtime_error("Failed to generate sync ID");
	return result;
}


/// <summary>
/// Adds a command to the console commands list.
/// </summary>
/// <param name="command">The command to add.</param>
/// <returns>A pointer to the command, if added successfully.</returns>
Command* CommandManager::Add(Command command)
{
	if (Find(command.Name) || Find(command.ShortName))
		return nullptr;

	this->List.push_back(command);
	if (!(command.Flags & CommandFlags::eCommandFlagsReplicated))
		return &this->List.back();

	// create a client var for this variable and setup sync for it

	auto* retval = &this->List.back();

	auto clientCmd = Command::CreateVariableInt(command.ModuleName, command.Name + "Client", "client_" + command.ShortName, "", static_cast<CommandFlags>((command.Flags & ~eCommandFlagsReplicated) | eCommandFlagsInternal), command.DefaultValueInt, command.UpdateEvent);
	command.UpdateEvent = nullptr;
	clientCmd.Type = command.Type;

	clientCmd.ValueString = command.ValueString;
	switch (clientCmd.Type)
	{
	case CommandType::VariableInt:
		clientCmd.ValueInt = command.ValueInt;
		clientCmd.DefaultValueInt = command.DefaultValueInt;
		break;
	case CommandType::VariableInt64:
		clientCmd.ValueInt64 = command.ValueInt64;
		clientCmd.DefaultValueInt64 = command.DefaultValueInt64;
		break;
	case CommandType::VariableFloat:
		clientCmd.ValueFloat = command.ValueFloat;
		clientCmd.DefaultValueFloat = command.DefaultValueFloat;
		break;
	case CommandType::VariableString:
		clientCmd.DefaultValueString = command.DefaultValueString;
		break;
	}

	auto* clientVar = Add(clientCmd);

	SynchronizationBinding binding;
	binding.ServerVariable = retval;
	binding.ClientVariable = clientVar;
	binding.ID = GenerateID(binding);
	binding.SynchronizedPeers.reset();

	if (SyncBindings.find(binding.ID) != SyncBindings.end())
		throw std::runtime_error("Duplicate sync ID");

	SyncBindings[binding.ID] = binding;

	return retval;
}

/// <summary>
/// Finalizes adding all commands: calls the update event for each command, ensuring default values are applied.
/// </summary>
void CommandManager::FinishAdd()
{
	for (auto command : List)
	{
		if (command.Type != CommandType::Command && (command.Flags & eCommandFlagsDontUpdateInitial) != eCommandFlagsDontUpdateInitial)
			if (command.UpdateEvent)
				command.UpdateEvent(std::vector<std::string>(), LogFileContext);
	}

	auto updateHandler = std::make_shared<SyncUpdateHandler>();
	updateSender = Packets::RegisterVariadicPacket<SyncUpdatePacketData, SyncUpdatePacketVar>("eldewrito-sync-var", updateHandler);

	auto& engine = ElDorito::Instance().Engine;
	engine.OnTick(BIND_CALLBACK(this, &CommandManager::TickSyncBindings));
	engine.OnEvent("Core", "Game.Leave", BIND_CALLBACK(this, &CommandManager::OnGameLeave));
	engine.OnEvent("Core", "Engine.MainMenuShown", BIND_CALLBACK(this, &CommandManager::OnMainMenuShown));
}

/// <summary>
/// Finds a command based on its name.
/// </summary>
/// <param name="name">The name of the command.</param>
/// <returns>A pointer to the command, if found.</returns>
Command* CommandManager::Find(const std::string& name)
{
	for (auto it = List.begin(); it < List.end(); it++)
		if ((!it->Name.empty() && !_stricmp(it->Name.c_str(), name.c_str())) || (!it->ShortName.empty() && !_stricmp(it->ShortName.c_str(), name.c_str())))
			return &(*it);

	return nullptr;
}

/// <summary>
/// Executes a command string (vector splits spaces?)
/// </summary>
/// <param name="command">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>The output of the executed command.</returns>
CommandExecuteResult CommandManager::Execute(const std::vector<std::string>& command, CommandContext& context)
{
	std::string commandStr = "";
	for (auto cmd : command)
		commandStr += "\"" + cmd + "\" ";

	return Execute(commandStr, context);
}

/// <summary>
/// Executes a command string
/// </summary>
/// <param name="command">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>The output of the executed command.</returns>
CommandExecuteResult CommandManager::Execute(const std::string& command, CommandContext& context, bool writeResultString)
{
	int numArgs = 0;
	auto args = CommandLineToArgvA((char*)command.c_str(), &numArgs);

	if (numArgs <= 0)
	{
		if (writeResultString)
			context.WriteOutput("Invalid input");
		return CommandExecuteResult::InvalidInput;
	}

	auto cmd = Find(args[0]);
	if (!cmd || 
		(!context.IsInternal() && cmd->Flags & eCommandFlagsInternal && !(cmd->Flags & eCommandFlagsArchived)) ||  // trying to run internal command under non-internal context, and the command also isn't archived
	    (!context.IsChat() && cmd->Flags & eCommandFlagsChatCommand) || // trying to run a chat command under non-chat context
	    (context.IsChat() && !(cmd->Flags & eCommandFlagsChatCommand))) // trying to run a non-chat command under chat context
	{
#ifdef _DEBUG
		if (!cmd || numArgs > 1 || cmd->Type == CommandType::Command)
		{
#endif
			if (writeResultString)
				context.WriteOutput("Command/variable not found");
			return CommandExecuteResult::NotFound;
#ifdef _DEBUG
		}
#endif
	}

	auto& dorito = ElDorito::Instance();

	if ((cmd->Flags & eCommandFlagsRunOnMainMenu) && !dorito.Engine.HasMainMenuShown())
	{
		queuedCommands.push_back(command);
		if (writeResultString)
			context.WriteOutput("Command/variable queued until mainmenu shows");
		return CommandExecuteResult::Queued;
	}

	if ((cmd->Flags & eCommandFlagsCheat))
	{
		if (!dorito.ServerCommands || !dorito.ServerCommands->VarCheats)
		{
			queuedCommands.push_back(command);
			if (writeResultString)
				context.WriteOutput("Command/variable queued until mainmenu shows");
			return CommandExecuteResult::Queued;
		}
		if (!dorito.ServerCommands->VarCheats->ValueInt && (numArgs > 1 || cmd->Type == CommandType::Command))
		{
			if (writeResultString)
				context.WriteOutput("Command/variable cannot be used unless Server.Cheats is set to 1");
			return CommandExecuteResult::CheatCommand;
		}
	}

	auto* session = ElDorito::Instance().Engine.GetActiveNetworkSession();

	if (cmd->Flags & eCommandFlagsMustBeHosting)
		if (!session || !session->IsEstablished() || !session->IsHost())
		{
			if (numArgs > 1 || cmd->Type == CommandType::Command)
			{
				if (writeResultString)
					context.WriteOutput("You must be hosting a game to use this command/variable");
				return CommandExecuteResult::MustBeHost;
			}
		}

	if (cmd->Flags & eCommandFlagsReplicated)
		if (session && session->IsEstablished() && !session->IsHost())
		{
			if (numArgs > 1 || cmd->Type == CommandType::Command)
			{
				if (writeResultString)
					context.WriteOutput("You must be at the main menu or hosting a game to use this command/variable");
				return CommandExecuteResult::MustBeHostOrMainMenu;
			}
		}

	std::vector<std::string> argsVect;
	if (numArgs > 1)
		for (int i = 1; i < numArgs; i++)
			argsVect.push_back(args[i]);

	if (cmd->Type == CommandType::Command)
		return cmd->UpdateEvent(argsVect, context) ? CommandExecuteResult::Success : CommandExecuteResult::CommandFailed;

	std::string previousValue;
	auto updateRet = SetVariable(cmd, (numArgs > 1 ? argsVect[0] : ""), previousValue);

	switch (updateRet)
	{
		case VariableSetReturnValue::Error:
		{
			if (writeResultString)
				context.WriteOutput("Command/variable not found");
			return CommandExecuteResult::NotFound;
		}
		case VariableSetReturnValue::InvalidArgument:
		{
			if (writeResultString)
				context.WriteOutput("Invalid value");
			return CommandExecuteResult::InvalidValue;
		}
		case VariableSetReturnValue::OutOfRange:
		{
			if (writeResultString)
			{
				if (cmd->Type == CommandType::VariableInt)
					context.WriteOutput("Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueIntMin) + ".." + std::to_string(cmd->ValueIntMax) + "]");
				else if (cmd->Type == CommandType::VariableInt64)
					context.WriteOutput("Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueInt64Min) + ".." + std::to_string(cmd->ValueInt64Max) + "]");
				else if (cmd->Type == CommandType::VariableFloat)
					context.WriteOutput("Value " + argsVect[0] + " out of range [" + std::to_string(cmd->ValueFloatMin) + ".." + std::to_string(cmd->ValueFloatMax) + "]");
				else
					context.WriteOutput("Value " + argsVect[0] + " out of range [this shouldn't be happening!]");
			}

			return CommandExecuteResult::OutOfRange;
		}
	}

	// special case for blanking strings
	if (cmd->Type == CommandType::VariableString && numArgs > 1 && argsVect[0].empty())
		cmd->ValueString = "";

	if (numArgs <= 1)
	{
		if (writeResultString)
			context.WriteOutput(previousValue);
		return CommandExecuteResult::Success;
	}

	if (!cmd->UpdateEvent)
	{
		if (writeResultString)
			context.WriteOutput(previousValue + " -> " + cmd->ValueString); // no update event, so we'll just return with what we set the value to
		return CommandExecuteResult::Success;
	}

	auto ret = cmd->UpdateEvent(argsVect, context);

	if (!ret) // error, revert the variable
		this->SetVariable(cmd, previousValue, std::string());

	if (writeResultString)
		context.WriteOutput(previousValue + " -> " + cmd->ValueString); // TODO: don't write this if the updateevent has written something

	return CommandExecuteResult::Success;
}

/// <summary>
/// Executes a list of commands, seperated by new lines
/// </summary>
/// <param name="commands">The command string.</param>
/// <param name="isUserInput">Whether the command came from the user or internally.</param>
/// <returns>Whether the command executed successfully.</returns>
bool CommandManager::ExecuteList(const std::string& commands, CommandContext& context)
{
	std::istringstream stream(commands);
	std::stringstream ss;
	std::string line;
	int lineIdx = 0;
	while (std::getline(stream, line))
	{
		auto trimmed = ElDorito::Instance().Utils.Trim(ElDorito::Instance().Utils.Trim(line, true));
		if (trimmed.empty())
			continue;

		auto result = this->Execute(trimmed, context);
		if (result != CommandExecuteResult::Success)
		{
			context.WriteOutput("Error at line " + std::to_string(lineIdx) + " (" + CommandExecuteResultString[(int)result] + ")");
			context.WriteOutput(std::to_string(lineIdx) + ": " + trimmed);
		}
		lineIdx++;
	}
	return true;
}

/// <summary>
/// Executes the command queue.
/// </summary>
/// <returns>Results of the executed commands.</returns>
bool CommandManager::ExecuteQueue(CommandContext& context)
{
	for (auto cmd : queuedCommands)
		Execute(cmd, context);

	queuedCommands.clear();
	return true;
}

/// <summary>
/// Gets the value of an int variable.
/// </summary>
/// <param name="name">The name of the variable.</param>
/// <param name="value">Returns the value of the variable.</param>
/// <returns>true if the value argument was updated, false if the variable isn't found or is the wrong type.</returns>
bool CommandManager::GetVariableInt(const std::string& name, unsigned long& value)
{
	auto command = Find(name);
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
bool CommandManager::GetVariableInt64(const std::string& name, unsigned long long& value)
{
	auto command = Find(name);
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
bool CommandManager::GetVariableFloat(const std::string& name, float& value)
{
	auto command = Find(name);
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
bool CommandManager::GetVariableString(const std::string& name, std::string& value)
{
	auto command = Find(name);
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
VariableSetReturnValue CommandManager::SetVariable(const std::string& name, const std::string& value, std::string& previousValue, bool callUpdateEvent)
{
	auto command = Find(name);
	if (!command)
		return VariableSetReturnValue::Error;

	return SetVariable(command, value, previousValue, callUpdateEvent);
}

/// <summary>
/// Sets a variable from a string, string is converted to the proper variable type.
/// </summary>
/// <param name="command">The variable to update.</param>
/// <param name="value">The value to set.</param>
/// <param name="previousValue">The previous value of the variable.</param>
/// <returns>VariableSetReturnValue</returns>
VariableSetReturnValue CommandManager::SetVariable(Command* command, const std::string& value, std::string& previousValue, bool callUpdateEvent)
{
	try {
		switch (command->Type)
		{
		case CommandType::VariableString:
			previousValue = command->ValueString;
			if (!value.empty())
				command->ValueString = value;
			break;
		case CommandType::VariableInt:
			previousValue = std::to_string(command->ValueInt);
			if (!value.empty())
			{
				auto newValue = std::stoul(value, 0, 0);
				if ((command->ValueIntMin || command->ValueIntMax) && (newValue < command->ValueIntMin || newValue > command->ValueIntMax))
					return VariableSetReturnValue::OutOfRange;

				command->ValueInt = newValue;
				command->ValueString = std::to_string(command->ValueInt); // set the ValueString too so we can print the value out easier
			}
			break;
		case CommandType::VariableInt64:
			previousValue = std::to_string(command->ValueInt64);
			if (!value.empty())
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
			if (!value.empty())
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

	if (callUpdateEvent)
		command->UpdateEvent({ command->ValueString }, LogFileContext);

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
std::string CommandManager::GenerateHelpText(const std::string& moduleFilter)
{
	std::deque<Command> tempCommands(List);

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

		if (!cmd.ModuleName.empty())
			hasParent << helpText << std::endl;
		else
			ss << helpText << std::endl;
	}

	ss << hasParent.str();

	return ss.str();
}

std::string CommandManager::GenerateHelpText(const Command& command)
{
	std::stringstream ss;
	ss << command.Name << " - " << command.Description << "." << std::endl;
	ss << "Usage: " << command.Name << " ";
	if (command.Type != CommandType::Command)
	{
		ss << "<value(";
		switch (command.Type)
		{
		case CommandType::VariableInt:
			ss << "int";
			break;
		case CommandType::VariableInt64:
			ss << "int64";
			break;
		case CommandType::VariableFloat:
			ss << "float";
			break;
		case CommandType::VariableString:
			ss << "string";
			break;
		}
		ss << ")>" << std::endl << "Current value: " << command.ValueString << std::endl << std::endl;
	}
	else
	{
		std::stringstream paramStream;
		for (auto arg : command.CommandArgs)
		{
			auto argname = arg;
			std::string argdesc = "";
			auto nameEnd = arg.find(" ");
			if (nameEnd != std::string::npos)
			{
				argname = arg.substr(0, nameEnd);
				if (arg.length() > (nameEnd + 1))
					argdesc = arg.substr(nameEnd + 1);
			}
			ss << "<" << argname << "> ";
			if (argdesc.length() > 0)
				paramStream << "  " << argname << ": " << argdesc << std::endl;
		}
		ss << std::endl;
		auto paramDescs = paramStream.str();
		if (paramDescs.length() > 0)
			ss << paramDescs << std::endl;
		else
			ss << std::endl;
	}

	return ss.str();
}

/// <summary>
/// Writes each variables name and value to a string.
/// </summary>
/// <returns>Each variables name and value.</returns>
std::string CommandManager::SaveVariables()
{
	std::stringstream ss;
	for (auto cmd : List)
	{
		if (cmd.Type == CommandType::Command || !(cmd.Flags & eCommandFlagsArchived))
			continue;

		ss << cmd.Name << " \"" << cmd.ValueString << "\"" << std::endl;
	}
	ss << std::endl;

	for (int i = 0; i < Blam::Input::eKeyCodes_Count; i++)
	{
		auto& bind = bindings[i];
		if (bind.command.empty() || bind.key.empty())
			continue;

		ss << "Input.Bind " << bind.key << " " << bind.command;
		ss << std::endl;
	}
	ss << std::endl;
	for (auto id : ElDorito::Instance().DisabledModIds)
		ss << "Mods disableID " << id << std::endl;

	return ss.str();
}

/// <summary>
/// Adds or clears a keyboard binding.
/// </summary>
/// <param name="key">The key to bind.</param>
/// <param name="command">The command to run (empty if clearing).</param>
/// <returns>BindingReturnValue</returns>
BindingReturnValue CommandManager::AddBinding(const std::string& key, const std::string& command)
{
	// Get the key, convert it to lowercase, and translate it to a key code
	auto actualKey = ElDorito::Instance().Utils.ToLower(key);
	auto it = keyCodes.find(key);
	if (it == keyCodes.end())
		return BindingReturnValue::UnknownKey;

	auto keyCode = it->second;
	auto binding = &bindings[keyCode];

	// If no command was specified, unset the binding
	if (command.empty())
	{
		binding->command.clear();
		return BindingReturnValue::ClearedBinding;
	}

	// Set the binding
	binding->key = actualKey;
	binding->command = command;
	return BindingReturnValue::Success;
}

/// <summary>
/// Gets the binding for a key.
/// </summary>
/// <param name="key">The key.</param>
/// <returns>A pointer to the KeyBinding struct for this key.</returns>
KeyBinding* CommandManager::GetBinding(const std::string& key)
{
	auto actualKey = ElDorito::Instance().Utils.ToLower(key);
	auto it = keyCodes.find(key);
	if (it == keyCodes.end())
		return nullptr;

	auto keyCode = it->second;
	return &bindings[keyCode];
}

/// <summary>
/// Gets the binding for a keycode.
/// </summary>
/// <param name="keyCode">The key code.</param>
/// <returns>A pointer to the KeyBinding struct for this key code.</returns>
KeyBinding* CommandManager::GetBinding(int keyCode)
{
	if (keyCode < 0 || keyCode >= Blam::Input::eKeyCodes_Count)
		return nullptr;

	return &bindings[keyCode];
}

CommandContext& CommandManager::GetLogFileContext()
{
	return this->LogFileContext;
}

bool IsPeerReady(Blam::Network::Session *session, int peerIndex)
{
	auto membership = &session->MembershipInfo;
	if (peerIndex == membership->LocalPeerIndex || !membership->Peers[peerIndex].IsEstablished())
		return false;
	auto channelInfo = &membership->PeerChannels[peerIndex];
	return !channelInfo->Unavailable && channelInfo->ChannelIndex >= 0;
}

std::vector<SynchronizationBinding*> CommandManager::FindOutOfDateBindings(int peerIndex)
{
	// Find bindings which don't have the peer in their set
	std::vector<SynchronizationBinding*> result;
	for (auto &&binding : SyncBindings)
	{
		if (!binding.second.SynchronizedPeers[peerIndex])
			result.push_back(&binding.second);
	}
	return result;
}

void BuildVariableUpdate(const SynchronizationBinding *binding, SyncUpdatePacketVar *result)
{
	auto var = binding->ServerVariable;
	result->ID = binding->ID;
	result->Type = var->Type;
	switch (var->Type)
	{
	case CommandType::VariableInt:
		result->Value.Int = var->ValueInt;
		break;
	case CommandType::VariableInt64:
		result->Value.Int = var->ValueInt64;
		break;
	case CommandType::VariableFloat:
		result->Value.Float = var->ValueFloat;
		break;
	case CommandType::VariableString:
		strncpy_s(result->Value.String, var->ValueString.c_str(), MaxStringLength);
		result->Value.String[MaxStringLength] = 0;
		break;
	default:
		throw std::runtime_error("Unsupported variable type");
	}
}

std::shared_ptr<SyncUpdatePacket> BuildUpdatePacket(const std::vector<SynchronizationBinding*> &bindings)
{
	auto result = updateSender->New(bindings.size());
	for (size_t i = 0; i < bindings.size(); i++)
	{
		auto binding = bindings[i];
		auto update = &result->ExtraData[i];
		BuildVariableUpdate(binding, update);
	}
	return result;
}

void CommandManager::SynchronizePeer(int peerIndex)
{
	// Get the bindings which need to be sent to the peer
	auto outOfDateBindings = FindOutOfDateBindings(peerIndex);
	if (!outOfDateBindings.size())
		return;

	// Build and send an update packet
	auto packet = BuildUpdatePacket(outOfDateBindings);
	updateSender->Send(peerIndex, packet);

	// Mark the peer as synchronized
	for (auto binding : outOfDateBindings)
		binding->SynchronizedPeers[peerIndex] = true;
}

void CommandManager::TickSyncBindings(const std::chrono::duration<double>& deltaTime)
{
	auto session = ElDorito::Instance().Engine.GetActiveNetworkSession();
	if (!session || !session->IsHost())
		return;

	for (auto &&binding : SyncBindings)
		TickBinding(&binding.second);

	std::bitset<Blam::Network::MaxPeers> visitedPeers;

	auto membership = &session->MembershipInfo;
	for (auto peer = membership->FindFirstPeer(); peer != -1; peer = membership->FindNextPeer(peer))
	{
		// Only synchronize remote peers which are completely established
		if (peer != membership->LocalPeerIndex && IsPeerReady(session, peer))
		{
			SynchronizePeer(peer);
			visitedPeers[peer] = true;
		}
	}

	// Clear the synchronization statuses for peers which weren't visited
	// (this takes care of disconnecting peers)
	for (auto &&binding : SyncBindings)
		binding.second.SynchronizedPeers &= visitedPeers;
}

void CommandManager::TickBinding(SynchronizationBinding* binding)
{
	// Only update if the server variable changed
	if (binding->ServerVariable->Compare(binding->ClientVariable))
		return;

	// Synchronize the client variable locally
	binding->ClientVariable->CopyFrom(binding->ServerVariable);

	// Clear the binding's peer synchronization status so that it will be
	// batched in with the next update
	binding->SynchronizedPeers.reset();
}

void CommandManager::OnGameLeave(void* param)
{
	auto* session = ElDorito::Instance().Engine.GetActiveNetworkSession();
	if (session && session->IsEstablished() && session->IsHost())
		return; // don't reset vars if we're host

	// when leaving a game reset replicated vars back to their defaults
	for (auto &&binding : SyncBindings)
	{
		binding.second.ServerVariable->Reset();
		binding.second.ClientVariable->Reset();
	}
}

void CommandManager::OnMainMenuShown(void* param)
{
	ExecuteQueue(ConsoleContext);
}

CommandManager::CommandManager()
{
}

void SyncUpdateHandler::Serialize(Blam::BitStream* stream, const SyncUpdatePacketData* data, int extraDataCount, const SyncUpdatePacketVar* extraData)
{
	for (auto i = 0; i < extraDataCount; i++)
	{
		// Send ID and type followed by value
		auto var = &extraData[i];
		stream->WriteUnsigned(var->ID, 32);
		stream->WriteUnsigned<uint32_t>((uint32_t)var->Type, 0, (uint32_t)CommandType::Count - 1);

		switch (var->Type)
		{
		case CommandType::VariableInt:
			stream->WriteUnsigned(var->Value.Int, 32);
			break;
		case CommandType::VariableInt64:
			stream->WriteUnsigned(var->Value.Int, 64);
			break;
		case CommandType::VariableFloat:
			stream->WriteBlock(32, reinterpret_cast<const uint8_t*>(&var->Value.Float));
			break;
		case CommandType::VariableString:
			stream->WriteString(var->Value.String);
			break;
		default:
			throw std::runtime_error("Unsupported variable type");
		}
	}
}

bool SyncUpdateHandler::Deserialize(Blam::BitStream* stream, SyncUpdatePacketData* data, int extraDataCount, SyncUpdatePacketVar* extraData)
{
	for (auto i = 0; i < extraDataCount; i++)
	{
		// Read ID and type followed by value
		auto var = &extraData[i];
		var->ID = stream->ReadUnsigned<uint32_t>(32);
		var->Type = static_cast<CommandType>(stream->ReadUnsigned<uint32_t>(0, (int)CommandType::Count - 1));
		switch (var->Type)
		{
		case CommandType::VariableInt:
			var->Value.Int = stream->ReadUnsigned<uint64_t>(32);
			break;
		case CommandType::VariableInt64:
			var->Value.Int = stream->ReadUnsigned<uint64_t>(64);
			break;
		case CommandType::VariableFloat:
			stream->ReadBlock(32, reinterpret_cast<uint8_t*>(&var->Value.Float));
			break;
		case CommandType::VariableString:
			stream->ReadString(var->Value.String);
			break;
		default:
			return false;
		}
	}
	return true;
}

void SyncUpdateHandler::HandlePacket(Blam::Network::ObserverChannel* sender, const Packets::VariadicPacket<SyncUpdatePacketData, SyncUpdatePacketVar>* packet)
{
	auto& dorito = ElDorito::Instance();
	auto session = dorito.Engine.GetActiveNetworkSession();
	if (session->GetChannelPeer(sender) != session->MembershipInfo.HostPeerIndex)
		return; // Packets must come from the host

	// Update each variable based on the binding ID
	for (auto i = 0; i < packet->GetExtraDataCount(); i++)
	{
		auto value = &packet->ExtraData[i];
		auto it = dorito.CommandManager.SyncBindings.find(value->ID);
		if (it == dorito.CommandManager.SyncBindings.end())
			continue;
		it->second.ClientVariable->CopyFrom(value);
		it->second.ServerVariable->CopyFrom(value);
	}
}

namespace
{
	/*std::map<Blam::Input::KeyCodes, WPARAM> keyToVKEY =
	{
		{ Blam::Input::eKeyCodesEscape, VK_ESCAPE },
		{ Blam::Input::eKeyCodesF1, VK_F1 },
		{ Blam::Input::eKeyCodesF2, VK_F2 },
		{ Blam::Input::eKeyCodesF3, VK_F3 },
		{ Blam::Input::eKeyCodesF4, VK_F4 },
		{ Blam::Input::eKeyCodesF5, VK_F5 },
		{ Blam::Input::eKeyCodesF6, VK_F6 },
		{ Blam::Input::eKeyCodesF7, VK_F7 },
		{ Blam::Input::eKeyCodesF8, VK_F8 },
		{ Blam::Input::eKeyCodesF9, VK_F9 },
		{ Blam::Input::eKeyCodesF10, VK_F10 },
		{ Blam::Input::eKeyCodesF11, VK_F11 },
		{ Blam::Input::eKeyCodesF12, VK_F12 },
		{ Blam::Input::eKeyCodesPrintScreen, VK_PRINT },
		{ Blam::Input::eKeyCodesF14, VK_F14 },
		{ Blam::Input::eKeyCodesF15, VK_F15 },
		{ Blam::Input::eKeyCodesTilde, 0 },
		{ Blam::Input::eKeyCodes1, 0x31 },
		{ Blam::Input::eKeyCodes2, 0x32 },
		{ Blam::Input::eKeyCodes3, 0x33 },
		{ Blam::Input::eKeyCodes4, 0x34 },
		{ Blam::Input::eKeyCodes5, 0x35 },
		{ Blam::Input::eKeyCodes6, 0x36 },
		{ Blam::Input::eKeyCodes7, 0x37 },
		{ Blam::Input::eKeyCodes8, 0x38 },
		{ Blam::Input::eKeyCodes9, 0x39 },
		{ Blam::Input::eKeyCodes0, 0x30 },
		{ Blam::Input::eKeyCodesMinus, VK_OEM_MINUS },
		{ Blam::Input::eKeyCodesPlus, VK_OEM_PLUS },
		{ Blam::Input::eKeyCodesBack, VK_BACK },
		{ Blam::Input::eKeyCodesTab, VK_TAB },
		{ Blam::Input::eKeyCodesA, 0x41 },
		{ Blam::Input::eKeyCodesB, 0x42 },
		{ Blam::Input::eKeyCodesC, 0x43 },
		{ Blam::Input::eKeyCodesD, 0x44 },
		{ Blam::Input::eKeyCodesE, 0x45 },
		{ Blam::Input::eKeyCodesF, 0x46 },
		{ Blam::Input::eKeyCodesG, 0x47 },
		{ Blam::Input::eKeyCodesH, 0x48 },
		{ Blam::Input::eKeyCodesI, 0x49 },
		{ Blam::Input::eKeyCodesJ, 0x4A },
		{ Blam::Input::eKeyCodesK, 0x4B },
		{ Blam::Input::eKeyCodesL, 0x4C },
		{ Blam::Input::eKeyCodesM, 0x4D },
		{ Blam::Input::eKeyCodesN, 0x4E },
		{ Blam::Input::eKeyCodesO, 0x4F },
		{ Blam::Input::eKeyCodesP, 0x50 },
		{ Blam::Input::eKeyCodesQ, 0x51 },
		{ Blam::Input::eKeyCodesR, 0x52 },
		{ Blam::Input::eKeyCodesS, 0x53 },
		{ Blam::Input::eKeyCodesT, 0x54 },
		{ Blam::Input::eKeyCodesU, 0x55 },
		{ Blam::Input::eKeyCodesV, 0x56 },
		{ Blam::Input::eKeyCodesW, 0x57 },
		{ Blam::Input::eKeyCodesX, 0x58 },
		{ Blam::Input::eKeyCodesY, 0x59 },
		{ Blam::Input::eKeyCodesZ, 0x5A },
		{ Blam::Input::eKeyCodesLBracket },
		{ Blam::Input::eKeyCodesRBracket },
		{ Blam::Input::eKeyCodesPipe },
		{ Blam::Input::eKeyCodesCapital },
		{ Blam::Input::eKeyCodesColon },
		{ Blam::Input::eKeyCodesQuote },
		{ Blam::Input::eKeyCodesEnter },
		{ Blam::Input::eKeyCodesLShift },
		{ Blam::Input::eKeyCodesComma },
		{ Blam::Input::eKeyCodesPeriod },
		{ Blam::Input::eKeyCodesQuestion },
		{ Blam::Input::eKeyCodesRShift },
		{ Blam::Input::eKeyCodesLControl },
		{ Blam::Input::eKeyCodesLAlt },
		{ Blam::Input::eKeyCodesSpace },
		{ Blam::Input::eKeyCodesRAlt },
		{ Blam::Input::eKeyCodesApps },
		{ Blam::Input::eKeyCodesRcontrol },
		{ Blam::Input::eKeyCodesUp },
		{ Blam::Input::eKeyCodesDown },
		{ Blam::Input::eKeyCodesLeft },
		{ Blam::Input::eKeyCodesRight },
		{ Blam::Input::eKeyCodesInsert },
		{ Blam::Input::eKeyCodesHome },
		{ Blam::Input::eKeyCodesPageUp },
		{ Blam::Input::eKeyCodesDelete },
		{ Blam::Input::eKeyCodesEnd },
		{ Blam::Input::eKeyCodesPageDown },
		{ Blam::Input::eKeyCodesNumLock },
		{ Blam::Input::eKeyCodesDivide },
		{ Blam::Input::eKeyCodesMultiply },
		{ Blam::Input::eKeyCodesNumpad0 },
		{ Blam::Input::eKeyCodesNumpad1 },
		{ Blam::Input::eKeyCodesNumpad2 },
		{ Blam::Input::eKeyCodesNumpad3 },
		{ Blam::Input::eKeyCodesNumpad4 },
		{ Blam::Input::eKeyCodesNumpad5 },
		{ Blam::Input::eKeyCodesNumpad6 },
		{ Blam::Input::eKeyCodesNumpad7 },
		{ Blam::Input::eKeyCodesNumpad8 },
		{ Blam::Input::eKeyCodesNumpad9 },
		{ Blam::Input::eKeyCodesSubtract },
		{ Blam::Input::eKeyCodesAdd },
		{ Blam::Input::eKeyCodesNumpadEnter },
		{ Blam::Input::eKeyCodesDecimal },
		{ Blam::Input::eKeyCodesShift },
		{ Blam::Input::eKeyCodesCtrl },
		{ Blam::Input::eKeyCodesAlt }
	};*/

	// Key codes table
	std::map<std::string, Blam::Input::KeyCodes> keyCodes =
	{
		{ "escape", Blam::Input::eKeyCodesEscape },
		{ "f1", Blam::Input::eKeyCodesF1 },
		{ "f2", Blam::Input::eKeyCodesF2 },
		{ "f3", Blam::Input::eKeyCodesF3 },
		{ "f4", Blam::Input::eKeyCodesF4 },
		{ "f5", Blam::Input::eKeyCodesF5 },
		{ "f6", Blam::Input::eKeyCodesF6 },
		{ "f7", Blam::Input::eKeyCodesF7 },
		{ "f8", Blam::Input::eKeyCodesF8 },
		{ "f9", Blam::Input::eKeyCodesF9 },
		{ "f10", Blam::Input::eKeyCodesF10 },
		{ "f11", Blam::Input::eKeyCodesF11 },
		{ "f12", Blam::Input::eKeyCodesF12 },
		{ "printscreen", Blam::Input::eKeyCodesPrintScreen },
		{ "f14", Blam::Input::eKeyCodesF14 },
		{ "f15", Blam::Input::eKeyCodesF15 },
		{ "tilde", Blam::Input::eKeyCodesTilde },
		{ "1", Blam::Input::eKeyCodes1 },
		{ "2", Blam::Input::eKeyCodes2 },
		{ "3", Blam::Input::eKeyCodes3 },
		{ "4", Blam::Input::eKeyCodes4 },
		{ "5", Blam::Input::eKeyCodes5 },
		{ "6", Blam::Input::eKeyCodes6 },
		{ "7", Blam::Input::eKeyCodes7 },
		{ "8", Blam::Input::eKeyCodes8 },
		{ "9", Blam::Input::eKeyCodes9 },
		{ "0", Blam::Input::eKeyCodes0 },
		{ "minus", Blam::Input::eKeyCodesMinus },
		{ "plus", Blam::Input::eKeyCodesPlus },
		{ "back", Blam::Input::eKeyCodesBack },
		{ "tab", Blam::Input::eKeyCodesTab },
		{ "q", Blam::Input::eKeyCodesQ },
		{ "w", Blam::Input::eKeyCodesW },
		{ "e", Blam::Input::eKeyCodesE },
		{ "r", Blam::Input::eKeyCodesR },
		{ "t", Blam::Input::eKeyCodesT },
		{ "y", Blam::Input::eKeyCodesY },
		{ "u", Blam::Input::eKeyCodesU },
		{ "i", Blam::Input::eKeyCodesI },
		{ "o", Blam::Input::eKeyCodesO },
		{ "p", Blam::Input::eKeyCodesP },
		{ "lbracket", Blam::Input::eKeyCodesLBracket },
		{ "rbracket", Blam::Input::eKeyCodesRBracket },
		{ "pipe", Blam::Input::eKeyCodesPipe },
		{ "capital", Blam::Input::eKeyCodesCapital },
		{ "a", Blam::Input::eKeyCodesA },
		{ "s", Blam::Input::eKeyCodesS },
		{ "d", Blam::Input::eKeyCodesD },
		{ "f", Blam::Input::eKeyCodesF },
		{ "g", Blam::Input::eKeyCodesG },
		{ "h", Blam::Input::eKeyCodesH },
		{ "j", Blam::Input::eKeyCodesJ },
		{ "k", Blam::Input::eKeyCodesK },
		{ "l", Blam::Input::eKeyCodesL },
		{ "colon", Blam::Input::eKeyCodesColon },
		{ "quote", Blam::Input::eKeyCodesQuote },
		{ "enter", Blam::Input::eKeyCodesEnter },
		{ "lshift", Blam::Input::eKeyCodesLShift },
		{ "z", Blam::Input::eKeyCodesZ },
		{ "x", Blam::Input::eKeyCodesX },
		{ "c", Blam::Input::eKeyCodesC },
		{ "v", Blam::Input::eKeyCodesV },
		{ "b", Blam::Input::eKeyCodesB },
		{ "n", Blam::Input::eKeyCodesN },
		{ "m", Blam::Input::eKeyCodesM },
		{ "comma", Blam::Input::eKeyCodesComma },
		{ "period", Blam::Input::eKeyCodesPeriod },
		{ "question", Blam::Input::eKeyCodesQuestion },
		{ "rshift", Blam::Input::eKeyCodesRShift },
		{ "lcontrol", Blam::Input::eKeyCodesLControl },
		{ "lalt", Blam::Input::eKeyCodesLAlt },
		{ "space", Blam::Input::eKeyCodesSpace },
		{ "ralt", Blam::Input::eKeyCodesRAlt },
		{ "apps", Blam::Input::eKeyCodesApps },
		{ "rcontrol", Blam::Input::eKeyCodesRcontrol },
		{ "up", Blam::Input::eKeyCodesUp },
		{ "down", Blam::Input::eKeyCodesDown },
		{ "left", Blam::Input::eKeyCodesLeft },
		{ "right", Blam::Input::eKeyCodesRight },
		{ "insert", Blam::Input::eKeyCodesInsert },
		{ "home", Blam::Input::eKeyCodesHome },
		{ "pageup", Blam::Input::eKeyCodesPageUp },
		{ "delete", Blam::Input::eKeyCodesDelete },
		{ "end", Blam::Input::eKeyCodesEnd },
		{ "pagedown", Blam::Input::eKeyCodesPageDown },
		{ "numlock", Blam::Input::eKeyCodesNumLock },
		{ "divide", Blam::Input::eKeyCodesDivide },
		{ "multiply", Blam::Input::eKeyCodesMultiply },
		{ "numpad0", Blam::Input::eKeyCodesNumpad0 },
		{ "numpad1", Blam::Input::eKeyCodesNumpad1 },
		{ "numpad2", Blam::Input::eKeyCodesNumpad2 },
		{ "numpad3", Blam::Input::eKeyCodesNumpad3 },
		{ "numpad4", Blam::Input::eKeyCodesNumpad4 },
		{ "numpad5", Blam::Input::eKeyCodesNumpad5 },
		{ "numpad6", Blam::Input::eKeyCodesNumpad6 },
		{ "numpad7", Blam::Input::eKeyCodesNumpad7 },
		{ "numpad8", Blam::Input::eKeyCodesNumpad8 },
		{ "numpad9", Blam::Input::eKeyCodesNumpad9 },
		{ "subtract", Blam::Input::eKeyCodesSubtract },
		{ "add", Blam::Input::eKeyCodesAdd },
		{ "numpadenter", Blam::Input::eKeyCodesNumpadEnter },
		{ "decimal", Blam::Input::eKeyCodesDecimal },
		{ "shift", Blam::Input::eKeyCodesShift },
		{ "ctrl", Blam::Input::eKeyCodesCtrl },
		{ "alt", Blam::Input::eKeyCodesAlt },
	};

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