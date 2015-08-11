#pragma once
#include <vector>
#include <string>
#include <deque>


typedef std::initializer_list<unsigned char> PatchInitializerListType;

struct Patch
{
	std::string Name;
	size_t Address;
	std::vector<unsigned char> Data, Orig;
	bool Enabled;
	Patch(std::string name, size_t address, std::vector<unsigned char> data)
	{
		Name = name;
		Address = address;
		Data = data;
		Orig = {};
		Enabled = false;
	}

	Patch(std::string name, size_t address, PatchInitializerListType data)
	{
		Name = name;
		Address = address;
		Data = data;
		Orig = {};
		Enabled = false;
	}

	Patch(std::string name, size_t address, unsigned char fillByte, size_t numBytes)
	{
		Name = name;
		Address = address;
		Data.resize(numBytes);
		memset(Data.data(), fillByte, numBytes);
		Orig = {};
		Enabled = false;
	}
};

enum class HookType : int
{
	Call,
	Jmp,
	JmpIfEqual,
	JmpIfNotEqual, // unimplemented
};

struct Hook
{
	std::string Name;
	size_t Address;
	void* DestFunc;
	HookType Type;
	std::vector<unsigned char> Orig;
	bool Enabled;

	Hook(std::string name, size_t address, void* destFunc, HookType type)
	{
		Name = name;
		Address = address;
		DestFunc = destFunc;
		Type = type;
		Orig = {};
		Enabled = false;
	}
};

typedef std::initializer_list<Patch> PatchSetInitializerListType;
typedef std::initializer_list<Hook> PatchSetHookInitializerListType;
struct PatchSet
{
	std::string Name;
	std::deque<Patch> Patches;
	std::deque<Hook> Hooks;
	bool Enabled;

	PatchSet(std::string name, PatchSetInitializerListType patches, PatchSetHookInitializerListType hooks)
	{
		Name = name;
		Patches = patches;
		Hooks = hooks;
		Enabled = false;
	}
};

enum class PatchStatus
{
	NotFound,
	Enabled,
	Disabled
};

/*
if you want to make changes to this interface create a new IPatchManager002 class and make them there, then edit PatchManager class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IPatchManager001
{
public:
	/// <summary>
	/// Adds a patch to the manager.
	/// </summary>
	/// <param name="name">The patches name.</param>
	/// <param name="address">The address to patch.</param>
	/// <param name="data">The data to write.</param>
	/// <returns>The created <see cref="Patch"/>.</returns>
	virtual Patch* AddPatch(const std::string& name, size_t address, const PatchInitializerListType& data) = 0;

	/// <summary>
	/// Adds a patch to the manager.
	/// </summary>
	/// <param name="name">The patches name.</param>
	/// <param name="address">The address to patch.</param>
	/// <param name="fillByte">What byte to fill with.</param>
	/// <param name="numBytes">Number of bytes to fill.</param>
	/// <returns>The created <see cref="Patch"/>.</returns>
	virtual Patch* AddPatch(const std::string& name, size_t address, unsigned char fillByte, size_t numBytes) = 0;

	/// <summary>
	/// Adds a hook to the manager.
	/// </summary>
	/// <param name="name">The hooks name.</param>
	/// <param name="address">The address to hook.</param>
	/// <param name="destFunc">The dest function.</param>
	/// <param name="type">The type of hook.</param>
	/// <returns>The created <see cref="Hook"/>.</returns>
	virtual Hook* AddHook(const std::string& name, size_t address, void* destFunc, HookType type) = 0;

	/// <summary>
	/// Adds a set of patches/hooks to the manager.
	/// </summary>
	/// <param name="name">The name of the patchset.</param>
	/// <param name="patches">The patches to include in the patchset.</param>
	/// <param name="hooks">The hooks to include in the patchset.</param>
	/// <returns>The created <see cref="PatchSet"/>.</returns>
	virtual PatchSet* AddPatchSet(const std::string& name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks = {}) = 0;

	/// <summary>
	/// Looks up a patch based on its name.
	/// </summary>
	/// <param name="name">The name of the patch.</param>
	/// <returns>A pointer to the patch, if found.</returns>
	virtual Patch* FindPatch(const std::string& name) = 0;

	/// <summary>
	/// Looks up a hook based on its name.
	/// </summary>
	/// <param name="name">The name of the hook.</param>
	/// <returns>A pointer to the hook, if found.</returns>
	virtual Hook* FindHook(const std::string& name) = 0;

	/// <summary>
	/// Looks up a patch set based on its name.
	/// </summary>
	/// <param name="name">The name of the patch set.</param>
	/// <returns>A pointer to the patch set, if found.</returns>
	virtual PatchSet* FindPatchSet(const std::string& name) = 0;

	/// <summary>
	/// Toggles a patch based on its name.
	/// </summary>
	/// <param name="name">The name of the patch.</param>
	/// <returns>PatchStatus enum</returns>
	virtual PatchStatus TogglePatch(const std::string& name) = 0;

	/// <summary>
	/// Toggles a hook based on its name.
	/// </summary>
	/// <param name="name">The name of the hook.</param>
	/// <returns>PatchStatus enum</returns>
	virtual PatchStatus ToggleHook(const std::string& name) = 0;

	/// <summary>
	/// Toggles a patch set (and all children patches/hooks) based on its name.
	/// </summary>
	/// <param name="name">The name of the patch set.</param>
	/// <returns>PatchStatus enum</returns>
	virtual PatchStatus TogglePatchSet(const std::string& name) = 0;

	/// <summary>
	/// Toggles a patch.
	/// </summary>
	/// <param name="patch">The patch to toggle.</param>
	/// <returns>true if the patch is active, false if not.</returns>
	virtual bool TogglePatch(Patch* patch) = 0;

	/// <summary>
	/// Toggles a hook.
	/// </summary>
	/// <param name="hook">The hook to toggle.</param>
	/// <returns>true if the hook is active, false if not.</returns>
	virtual bool ToggleHook(Hook* hook) = 0;

	/// <summary>
	/// Toggles a patch set (and all children patches/hooks).
	/// </summary>
	/// <param name="patchSet">The patch set to toggle.</param>
	/// <returns>true if the hook is active, false if not.</returns>
	virtual bool TogglePatchSet(PatchSet* patchSet) = 0;

	/// <summary>
	/// Enables/disables a patch based on its name.
	/// </summary>
	/// <param name="name">The name of the patch.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the patch.</returns>
	virtual PatchStatus EnablePatch(const std::string& name, bool enable = true) = 0;

	/// <summary>
	/// Enables/disables a hook based on its name.
	/// </summary>
	/// <param name="hook">The name of the hook.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the hook.</returns>
	virtual PatchStatus EnableHook(const std::string& name, bool enable = true) = 0;

	/// <summary>
	/// Enables/disables a patch set based on its name.
	/// </summary>
	/// <param name="patchSet">The name of the patch set.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the patch set.</returns>
	virtual PatchStatus EnablePatchSet(const std::string& name, bool enable = true) = 0;

	/// <summary>
	/// Enables/disables a patch.
	/// </summary>
	/// <param name="patch">The patch.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the patch.</returns>
	virtual bool EnablePatch(Patch* patch, bool enable = true) = 0;

	/// <summary>
	/// Enables/disables a hook.
	/// </summary>
	/// <param name="hook">The hook.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the hook.</returns>
	virtual bool EnableHook(Hook* hook, bool enable = true) = 0;

	/// <summary>
	/// Enables/disables a patch set.
	/// </summary>
	/// <param name="patchSet">The patch set.</param>
	/// <param name="enable">Whether to enable it or not (default true)</param>
	/// <returns>The status of the patch set.</returns>
	virtual bool EnablePatchSet(PatchSet* patchSet, bool enable = true) = 0;
};

#define PATCHMANAGER_INTERFACE_VERSION001 "PatchManager001"

/* use this class if you're updating IPatchManager after we've released a build
also update the IPatchManager typedef and PATCHMANAGER_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class IPatchManager002 : public IPatchManager001
{

};

#define PATCHMANAGER_INTERFACE_VERSION002 "PatchManager002"*/

typedef IPatchManager001 IPatchManager;
#define PATCHMANAGER_INTERFACE_LATEST PATCHMANAGER_INTERFACE_VERSION001
