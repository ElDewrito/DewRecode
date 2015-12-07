#pragma once
#include "ElDorito.hpp"

class PatchProvider
{
public:
	virtual PatchSet GetPatches() { return PatchSet("null", {}, {}); }
	virtual void RegisterCallbacks(IEngine* engine) { return; }
	virtual void PatchTags(int numTags) { return; }
};