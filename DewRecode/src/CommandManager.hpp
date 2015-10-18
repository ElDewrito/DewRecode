#pragma once
#include <ElDorito/ElDorito.hpp>
#include <unordered_map>

#include "CommandContexts/ConsoleContext.hpp"
#include "CommandContexts/LogFileContext.hpp"

namespace
{
	char** CommandLineToArgvA(char* CmdLine, int* _argc);
}

struct SyncUpdatePacketData
{
};

typedef Packets::VariadicPacket<SyncUpdatePacketData, SyncUpdatePacketVar> SyncUpdatePacket;
typedef Packets::VariadicPacketSender<SyncUpdatePacketData, SyncUpdatePacketVar> SyncUpdatePacketSender;

class SyncUpdateHandler : public Packets::VariadicPacketHandler<SyncUpdatePacketData, SyncUpdatePacketVar>
{
public:
	SyncUpdateHandler() : Packets::VariadicPacketHandler<SyncUpdatePacketData, SyncUpdatePacketVar>(1) { }

	void Serialize(Blam::BitStream* stream, const SyncUpdatePacketData* data, int extraDataCount, const SyncUpdatePacketVar* extraData) override;
	bool Deserialize(Blam::BitStream* stream, SyncUpdatePacketData* data, int extraDataCount, SyncUpdatePacketVar* extraData) override;
	void HandlePacket(Blam::Network::ObserverChannel* sender, const Packets::VariadicPacket<SyncUpdatePacketData, SyncUpdatePacketVar>* packet) override;
};

// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class CommandManager : public ICommandManager
{
public:
	Command* Add(Command command);
	void FinishAdd();
	Command* Find(const std::string& name);

	const std::deque<Command>& GetList() { return List; }

	bool Execute(const std::vector<std::string>& command, ICommandContext& context);
	bool Execute(const std::string& command, ICommandContext& context);
	bool ExecuteList(const std::string& commands, ICommandContext& context);
	bool ExecuteQueue(ICommandContext& context);

	bool GetVariableInt(const std::string& name, unsigned long& value);
	bool GetVariableInt64(const std::string& name, unsigned long long& value);
	bool GetVariableFloat(const std::string& name, float& value);
	bool GetVariableString(const std::string& name, std::string& value);

	VariableSetReturnValue SetVariable(const std::string& name, const std::string& value, std::string& previousValue);
	VariableSetReturnValue SetVariable(Command* command, const std::string& value, std::string& previousValue);

	std::string GenerateHelpText(const std::string& moduleFilter = "");
	std::string GenerateHelpText(const Command& command);

	std::string SaveVariables();

	BindingReturnValue AddBinding(const std::string& key, const std::string& command);
	KeyBinding* GetBinding(const std::string& key);
	KeyBinding* GetBinding(int keyCode);

	ICommandContext& GetLogFileContext();

	void TickSyncBindings(const std::chrono::duration<double>& deltaTime);
	void TickBinding(SynchronizationBinding* binding);
	
	void SynchronizePeer(int peerIndex);
	std::vector<SynchronizationBinding*> FindOutOfDateBindings(int peerIndex);

	void OnGameLeave(void* param);

	CommandManager();
	std::deque<Command> List;
	std::unordered_map<SyncID, SynchronizationBinding> SyncBindings;

	ConsoleContext ConsoleContext;
	LogFileContext LogFileContext;

private:
	std::vector<std::string> queuedCommands;

	// Bindings for each key
	KeyBinding bindings[Blam::Input::eKeyCodes_Count];
};
