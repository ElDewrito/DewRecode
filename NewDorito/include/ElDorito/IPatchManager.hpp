#pragma once
#include <vector>
#include <string>
#include <deque>

struct Patch
{
	std::string Name;
	size_t Address;
	std::vector<unsigned char> Data, Orig;
	bool Enabled;
};

enum class HookType : int
{
	None = 0,
	Call = 1,
	JmpIfEqual = 2,
	JmpIfNotEqual = 4 // unimplemented
};

struct Hook
{
	std::string Name;
	size_t Address;
	void* DestFunc;
	HookType Type;
	std::vector<unsigned char> Orig;
	bool Enabled;
};

struct PatchSet
{
	std::string Name;
	std::deque<Patch> Patches;
	std::deque<Hook> Hooks;
	bool Enabled;
};

enum class PatchStatus
{
	NotFound,
	Enabled,
	Disabled
};

typedef std::initializer_list<unsigned char> PatchInitializerListType;
typedef std::initializer_list<Patch> PatchSetInitializerListType;
typedef std::initializer_list<Hook> PatchSetHookInitializerListType;

/*
if you want to make changes to this interface create a new IPatchManager002 class and make them there, then edit PatchManager class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IPatchManager001
{
public:
	virtual Patch* AddPatch(std::string name, size_t address, const PatchInitializerListType& data) = 0;
	virtual Hook* AddHook(std::string name, size_t address, void* destFunc, HookType type) = 0;
	virtual PatchSet* AddPatchSet(std::string name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks = {}) = 0;

	virtual Patch* FindPatch(std::string name) = 0;
	virtual Hook* FindHook(std::string name) = 0;
	virtual PatchSet* FindPatchSet(std::string name) = 0;

	virtual PatchStatus TogglePatch(std::string name) = 0;
	virtual PatchStatus ToggleHook(std::string name) = 0;
	virtual PatchStatus TogglePatchSet(std::string name) = 0;

	virtual bool TogglePatch(Patch* patch) = 0;
	virtual bool ToggleHook(Hook* hook) = 0;
	virtual bool TogglePatchSet(PatchSet* patchSet) = 0;
};

#define PATCHMANAGER_INTERFACE_VERSION001 "PatchManager001"
