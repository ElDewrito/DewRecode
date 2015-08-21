#pragma once
#include "ElDorito.hpp"

class IPatchProvider
{
public:
	virtual PatchSet GetPatches() { return PatchSet("null", {}, {}); }
	virtual void RegisterCallbacks(IEngine* engine) { return; }
};