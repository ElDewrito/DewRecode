#pragma once
#include <ElDorito/ElDorito.hpp>

namespace Modules
{
	class ModuleVoIP : public ModuleBase
	{
	public:
		Command* VarVoIPPushToTalk;
		Command* VarVoIPVolumeModifier;
		Command* VarVoIPAGC;
		Command* VarVoIPEchoCancellation;
		Command* VarVoIPVADLevel;
		Command* VarVoIPServerEnabled;
		Command* VarVoIPTalk;

		ModuleVoIP();
	};
}
