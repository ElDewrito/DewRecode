#pragma once
#include <ElDorito/ElDorito.hpp>
#include <deque>
#include <map>

// if you make any changes to this class make sure to update the exported interface (create a new interface + inherit from it if the interface already shipped)
class PatchManager : public IPatchManager
{
public:
	Patch* AddPatch(std::string name, size_t address, const PatchInitializerListType& data);
	Patch* AddPatch(std::string name, size_t address, unsigned char fillByte, size_t numBytes);
	Hook* AddHook(std::string name, size_t address, void* destFunc, HookType type);
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

	PatchStatus EnablePatch(std::string name, bool enable = true);
	PatchStatus EnableHook(std::string name, bool enable = true);
	PatchStatus EnablePatchSet(std::string name, bool enable = true);

	bool EnablePatch(Patch* patch, bool enable = true);
	bool EnableHook(Hook* hook, bool enable = true);
	bool EnablePatchSet(PatchSet* patchSet, bool enable = true);

private:
	std::deque<Patch> patches;
	std::deque<Hook> hooks;
	std::deque<PatchSet> patchSets;
};
