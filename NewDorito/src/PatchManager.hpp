#pragma once
#include <ElDorito/ElDorito.hpp>
#include <deque>
#include <map>

class PatchManager : public IPatchManager001
{
public:
	Patch* AddPatch(std::string name, size_t address, const PatchInitializerListType& data);
	Hook* AddHook(std::string name, size_t address, void* destFunc, HookFlags flags);
	PatchSet* AddPatchSet(std::string name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks = {});

	Patch* FindPatch(std::string name);
	Hook* FindHook(std::string name);
	PatchSet* FindPatchSet(std::string name);

	PatchStatus TogglePatch(std::string name);
	PatchStatus ToggleHook(std::string name);
	PatchStatus TogglePatchSet(std::string name);

	bool TogglePatch(Patch* patch);
	bool ToggleHook(Hook* hook);
	bool TogglePatchSet(PatchSet* patchSet);

private:
	std::deque<Patch> patches;
	std::deque<Hook> hooks;
	std::deque<PatchSet> patchSets;
};