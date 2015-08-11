#pragma once

#include "ElDorito.hpp"

namespace Modules
{
	class ModuleBase
	{
	public:
		ModuleBase(const std::string& moduleName)
		{
			this->moduleName = moduleName;
			int retCode = 0;
			commands = reinterpret_cast<ICommands*>(CreateInterface(COMMANDS_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create commands interface");

			logger = reinterpret_cast<IDebugLog*>(CreateInterface(DEBUGLOG_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create debug log interface");

			engine = reinterpret_cast<IEngine*>(CreateInterface(ENGINE_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create engine interface");

			patches = reinterpret_cast<IPatchManager*>(CreateInterface(PATCHMANAGER_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create patch manager interface");

			utils = reinterpret_cast<IUtils*>(CreateInterface(UTILS_INTERFACE_LATEST, &retCode));
			if (retCode != 0)
				throw std::runtime_error("Failed to create utils interface");
		}

		~ModuleBase()
		{
			int retCode = 0;
			patches = reinterpret_cast<IPatchManager*>(CreateInterface(PATCHMANAGER_INTERFACE_LATEST, &retCode));
			if (modulePatches)
			{
				patches->EnablePatchSet(modulePatches, false);
				modulePatches = nullptr;
			}
		}

		/// <summary>
		/// Retrieves the value of an integer variable.
		/// </summary>
		/// <param name="name">The name of the variable.</param>
		/// <param name="value">The return value.</param>
		/// <returns>true if the variable was found and is an integer.</returns>
		bool GetVariableInt(const std::string& name, unsigned long& value)
		{
			return commands->GetVariableInt(moduleName + "." + name, value);
		}

		/// <summary>
		/// Retrieves the value of a 64bit integer variable.
		/// </summary>
		/// <param name="name">The name of the variable.</param>
		/// <param name="value">The return value.</param>
		/// <returns>true if the variable was found and is a 64bit integer.</returns>
		bool GetVariableInt64(const std::string& name, unsigned long long& value)
		{
			return commands->GetVariableInt64(moduleName + "." + name, value);
		}

		/// <summary>
		/// Retrieves the value of a float variable.
		/// </summary>
		/// <param name="name">The name of the variable.</param>
		/// <param name="value">The return value.</param>
		/// <returns>true if the variable was found and is a float.</returns>
		bool GetVariableFloat(const std::string& name, float& value)
		{
			return commands->GetVariableFloat(moduleName + "." + name, value);
		}

		/// <summary>
		/// Retrieves the value of a string variable.
		/// </summary>
		/// <param name="name">The name of the variable.</param>
		/// <param name="value">The return value.</param>
		/// <returns>true if the variable was found and is a string.</returns>
		bool GetVariableString(const std::string& name, std::string& value)
		{
			return commands->GetVariableString(moduleName + "." + name, value);
		}

		/// <summary>
		/// Gets the patch set for this module.
		/// </summary>
		/// <returns>The patch set.</returns>
		PatchSet* GetModulePatchSet()
		{
			return modulePatches;
		}

	protected:
		ICommands* commands;
		IEngine* engine;
		IDebugLog* logger;
		IPatchManager* patches;
		IUtils* utils;

		/// <summary>
		/// Adds the default patches for this module (automatically enabled).
		/// </summary>
		/// <param name="patches">The patches.</param>
		/// <param name="hooks">The hooks.</param>
		/// <returns>The created & enabled patchset</returns>
		PatchSet* AddModulePatches(const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks = {})
		{
			modulePatches = this->patches->AddPatchSet(moduleName, patches, hooks);
			if (!modulePatches)
			{
				logger->Log(LogSeverity::Error, moduleName, "Failed to add module patches?!");
				return nullptr;
			}
			// toggle/enable the patch set
			this->patches->TogglePatchSet(modulePatches);
			return modulePatches;
		}

		/// <summary>
		/// Adds a command to the console.
		/// </summary>
		/// <param name="name">The name.</param>
		/// <param name="shortName">The short name.</param>
		/// <param name="description">The description.</param>
		/// <param name="flags">The flags.</param>
		/// <param name="updateEvent">The function to call when the command is run.</param>
		/// <param name="arguments">The arguments for this command.</param>
		/// <returns>The created command</returns>
		Command* AddCommand(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, CommandUpdateFunc updateEvent, std::initializer_list<const std::string> arguments = {})
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			for (auto arg : arguments)
				command.CommandArgs.push_back(arg);

			if (moduleName.empty())
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::Command;
			command.UpdateEvent = updateEvent;

			return commands->Add(command);
		}

		/// <summary>
		/// Adds an integer variable to the console.
		/// </summary>
		/// <param name="name">The name.</param>
		/// <param name="shortName">The short name.</param>
		/// <param name="description">The description.</param>
		/// <param name="flags">The flags.</param>
		/// <param name="defaultValue">The default value.</param>
		/// <param name="updateEvent">The function to call when the variable is changed.</param>
		/// <returns>The created variable</returns>
		Command* AddVariableInt(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.empty())
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableInt;
			command.DefaultValueInt = defaultValue;
			command.ValueInt = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			return commands->Add(command);
		}

		/// <summary>
		/// Adds a 64bit integer variable to the console.
		/// </summary>
		/// <param name="name">The name.</param>
		/// <param name="shortName">The short name.</param>
		/// <param name="description">The description.</param>
		/// <param name="flags">The flags.</param>
		/// <param name="defaultValue">The default value.</param>
		/// <param name="updateEvent">The function to call when the variable is changed.</param>
		/// <returns>The created variable</returns>
		Command* AddVariableInt64(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, unsigned long long defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.empty())
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableInt64;
			command.DefaultValueInt64 = defaultValue;
			command.ValueInt64 = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			return commands->Add(command);
		}

		/// <summary>
		/// Adds a float variable to the console.
		/// </summary>
		/// <param name="name">The name.</param>
		/// <param name="shortName">The short name.</param>
		/// <param name="description">The description.</param>
		/// <param name="flags">The flags.</param>
		/// <param name="defaultValue">The default value.</param>
		/// <param name="updateEvent">The function to call when the variable is changed.</param>
		/// <returns>The created variable</returns>
		Command* AddVariableFloat(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, float defaultValue = 0, CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.empty())
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableFloat;
			command.DefaultValueFloat = defaultValue;
			command.ValueFloat = defaultValue;
			command.ValueString = std::to_string(defaultValue); // set the ValueString too so we can print the value out easier
			command.UpdateEvent = updateEvent;

			return commands->Add(command);
		}

		/// <summary>
		/// Adds a string variable to the console.
		/// </summary>
		/// <param name="name">The name.</param>
		/// <param name="shortName">The short name.</param>
		/// <param name="description">The description.</param>
		/// <param name="flags">The flags.</param>
		/// <param name="defaultValue">The default value.</param>
		/// <param name="updateEvent">The function to call when the variable is changed.</param>
		/// <returns>The created variable</returns>
		Command* AddVariableString(const std::string& name, const std::string& shortName, const std::string& description, CommandFlags flags, const std::string& defaultValue = "", CommandUpdateFunc updateEvent = 0)
		{
			Command command;
			command.Name = moduleName + "." + name;
			command.ModuleName = moduleName;
			command.ShortName = shortName;
			command.Description = description;

			if (moduleName.empty())
				command.Name = name;

			command.Flags = flags;
			command.Type = CommandType::VariableString;
			command.DefaultValueString = defaultValue;
			command.ValueString = defaultValue;
			command.UpdateEvent = updateEvent;

			return commands->Add(command);
		}

	private:
#pragma warning(push)
#pragma warning(disable: 4251)
		std::string moduleName;
#pragma warning(pop)
		PatchSet* modulePatches = nullptr;
	};
}
