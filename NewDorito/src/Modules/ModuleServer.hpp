#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleServer : public ModuleBase
	{
	public:
		Command* VarServerCountdown;
		Command* VarServerMaxPlayers;
		Command* VarServerPort;
		Command* VarServerCheats;

		BYTE SyslinkData[0x176];

		ModuleServer();
	};
}