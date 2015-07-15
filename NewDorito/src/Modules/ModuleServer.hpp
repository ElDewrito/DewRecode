#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleServer : public ModuleBase
	{
	public:
		Command* VarServerName;
		Command* VarServerPassword;
		Command* VarServerCountdown;
		Command* VarServerMaxPlayers;
		Command* VarServerPort;
		Command* VarServerShouldAnnounce;
		BYTE SyslinkData[0x176];

		ModuleServer();
	};
}