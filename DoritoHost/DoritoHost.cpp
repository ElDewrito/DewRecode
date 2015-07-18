// DoritoHost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ElDorito/ElDorito.hpp>

int _tmain(int argc, _TCHAR* argv[])
{
	int version = GetDoritoVersion();

	ICommands* commands = reinterpret_cast<ICommands*>(CreateInterface(COMMANDS_INTERFACE_LATEST, &version));
	commands->Execute("Command001");

	IPatchManager* patches = reinterpret_cast<IPatchManager*>(CreateInterface(PATCHMANAGER_INTERFACE_LATEST, &version));

	return 0;
}
