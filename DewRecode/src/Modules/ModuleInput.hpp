#pragma once
#include <ElDorito/ModuleBase.hpp>
#include <map>
#include <ElDorito/Blam/BlamTypes.hpp>

namespace
{
	// Holds information about a command bound to a key
	// TODO1: refactor bindings into the Commands class?
	struct KeyBinding
	{
		std::vector<std::string> command; // If this is empty, no command is bound
		bool isHold; // True if the command binds to a boolean variable
		bool active; // True if this is a hold command and the key is down
		std::string key; // the key that corresponds to this code, easier than looking it up again
	};

	// Bindings for each key
	KeyBinding bindings[Blam::eKeyCodes_Count];
}
namespace Modules
{
	class ModuleInput : public ModuleBase
	{
	public:
		Command* VarInputRawInput;

		ModuleInput();
	};
}