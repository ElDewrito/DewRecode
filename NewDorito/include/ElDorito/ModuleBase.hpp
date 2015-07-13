#pragma once

#include "ElDorito.hpp"

namespace Modules
{
	class ModuleBase
	{
	public:
		ModuleBase(std::string moduleName)
		{
			this->moduleName = moduleName;
		}

		bool GetVariableInt(const std::string& name, unsigned long& value)
		{
			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode == 0 && console->GetVariableInt(moduleName + "." + name, value);
		}

		bool GetVariableFloat(const std::string& name, float& value)
		{
			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode == 0 && console->GetVariableFloat(moduleName + "." + name, value);
		}

		bool GetVariableString(const std::string& name, std::string& value)
		{
			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode == 0 && console->GetVariableString(moduleName + "." + name, value);
		}

	protected:
		Command* AddCommand(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, CommandUpdateFunc updateEvent, std::initializer_list<std::string> arguments = {})
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			for (auto arg : arguments)
				command.CommandArgs.push_back(arg);

			if (moduleName.length() <= 0)
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::Command;
			command.UpdateEvent = updateEvent;

			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode != 0 ? 0 : console->AddCommand(command);
		}

		Command* AddVariableInt(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.length() <= 0)
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableInt;
			command.DefaultValueInt = defaultValue;
			command.ValueInt = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode != 0 ? 0 : console->AddCommand(command);
		}

		Command* AddVariableInt64(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long long defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.length() <= 0)
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableInt64;
			command.DefaultValueInt64 = defaultValue;
			command.ValueInt64 = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode != 0 ? 0 : console->AddCommand(command);
		}

		Command* AddVariableFloat(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, float defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.length() <= 0)
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableFloat;
			command.DefaultValueFloat = defaultValue;
			command.ValueFloat = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode != 0 ? 0 : console->AddCommand(command);
		}

		Command* AddVariableString(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, std::string defaultValue = "", CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.length() <= 0)
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableString;
			command.DefaultValueString = defaultValue;
			command.ValueString = defaultValue;
			command.UpdateEvent = updateEvent;

			int retCode = 0;
			IConsole001* console = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &retCode));
			return retCode != 0 ? 0 : console->AddCommand(command);
		}

	private:
#pragma warning(push)
#pragma warning(disable: 4251)
		std::string moduleName;
#pragma warning(pop)
	};
}
