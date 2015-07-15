#pragma once
#include <ElDorito/ModuleBase.hpp>
#include "PlayerPropertiesExtension.hpp"
namespace Modules
{
	class PatchModuleNetwork : public ModuleBase
	{
	public:
		Patches::PlayerPropertiesExtender PlayerPropertiesExtender;
		PatchModuleNetwork();
	};
}
