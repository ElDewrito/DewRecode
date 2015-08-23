#pragma once
#include <ElDorito/ElDorito.hpp>
#include <ElDorito/Blam/BlamInput.hpp>

namespace
{
	char** CommandLineToArgvA(char* CmdLine, int* _argc);
}

// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class Commands : public ICommands
{
public:
	Command* Add(Command command);
	void FinishAdd();
	Command* Find(const std::string& name);

	const std::deque<Command>& GetList() { return List; }

	std::string Execute(const std::vector<std::string>& command, bool isUserInput = false);
	std::string Execute(const std::string& command, bool isUserInput = false);
	std::string ExecuteList(const std::string& commands, bool isUserInput = false);
	bool ExecuteWithStatus(const std::string& command, bool isUserInput = false);
	std::string ExecuteQueue();

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

	std::deque<Command> List;
private:
	std::vector<std::string> queuedCommands;

	// Bindings for each key
	KeyBinding bindings[Blam::NumKeyCodes];
};
