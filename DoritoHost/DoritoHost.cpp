// DoritoHost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ElDorito/ElDorito.hpp>

int _tmain(int argc, _TCHAR* argv[])
{
	int version = GetDoritoVersion();

	ICommands001* commands = reinterpret_cast<ICommands001*>(CreateInterface(COMMANDS_INTERFACE_VERSION001, &version));
	commands->Execute("Command001");

	IPatchManager001* patches001 = reinterpret_cast<IPatchManager001*>(CreateInterface(PATCHMANAGER_INTERFACE_VERSION001, &version));

	return 0;
}
