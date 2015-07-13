// DoritoHost.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../NewDorito/include/ElDorito/ElDorito.hpp"

int _tmain(int argc, _TCHAR* argv[])
{
	int version = GetEDVersion();

	IConsole001* console001 = reinterpret_cast<IConsole001*>(CreateInterface(CONSOLE_INTERFACE_VERSION001, &version));
	console001->ExecuteCommand("Command001");

	IPatchManager001* patches001 = reinterpret_cast<IPatchManager001*>(CreateInterface(PATCHMANAGER_INTERFACE_VERSION001, &version));

	return 0;
}

