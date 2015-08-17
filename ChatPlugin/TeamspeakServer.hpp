#pragma once
#include <Windows.h>
#include "ModuleVoIP.hpp"


DWORD WINAPI StartTeamspeakServer(Modules::ModuleVoIP* voipModule);
bool IsTeamspeakServerRunning();
void StopTeamspeakServer();
