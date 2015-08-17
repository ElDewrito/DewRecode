#pragma once
#include <Windows.h>
#include "ModuleVoIP.hpp"

DWORD WINAPI StartTeamspeakClient(Modules::ModuleVoIP* voipModule);
void StopTeamspeakClient();

UINT64 VoIPGetscHandlerID();
UINT64 VoIPGetVadHandlerID();
INT VoIPGetTalkStatus();