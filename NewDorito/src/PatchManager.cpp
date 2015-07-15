#include "PatchManager.hpp"
#include "../include/ElDorito/Pointer.hpp"

/// <summary>
/// Generates a byte array for the specified hook.
/// </summary>
/// <param name="hook">The hook.</param>
/// <returns>A vector of each byte for the hook.</returns>
std::vector<unsigned char> GetHookBytes(Hook* hook)
{
	uint8_t tempJMP[5] = { 0xE9, 0x90, 0x90, 0x90, 0x90 };
	uint8_t tempJE[6] = { 0x0F, 0x84, 0x90, 0x90, 0x90, 0x90 };

	if (hook->Type == HookType::Call)
		tempJMP[0] = 0xE8; // change it to call instruction

	uint32_t patchSize = (hook->Type == HookType::JmpIfEqual) ? 6 : 5;
	uint32_t JMPSize = ((uint32_t)hook->DestFunc - (uint32_t)hook->Address - patchSize);

	if (hook->Type == HookType::JmpIfEqual)
	{
		memcpy(&tempJE[2], &JMPSize, 4);
		return std::vector<unsigned char>(tempJE, tempJE + 6);
	}
	else if (hook->Type == HookType::JmpIfNotEqual)
	{
		// TODO
	}
	else //(hook->Type == HookType::None || hook->Type == HookType::Call)
	{
		memcpy(&tempJMP[1], &JMPSize, 4);
		return std::vector<unsigned char>(tempJMP, tempJMP + 5);
	}
	return{};
}

/// <summary>
/// Adds a patch to the manager.
/// </summary>
/// <param name="name">The patches name.</param>
/// <param name="address">The address to patch.</param>
/// <param name="data">The data to write.</param>
/// <returns>The created <see cref="Patch"/>.</returns>
Patch* PatchManager::AddPatch(std::string name, size_t address, const PatchInitializerListType& data)
{
	Patch patch(name, address, data);

	patch.Orig.resize(data.size());
	Pointer(address).Read(patch.Orig.data(), patch.Orig.size());

	patches.push_back(patch);
	return &patches.back();
}

/// <summary>
/// Adds a patch to the manager.
/// </summary>
/// <param name="name">The patches name.</param>
/// <param name="address">The address to patch.</param>
/// <param name="fillByte">What byte to fill with.</param>
/// <param name="numBytes">Number of bytes to fill.</param>
/// <returns>The created <see cref="Patch"/>.</returns>
Patch* PatchManager::AddPatch(std::string name, size_t address, unsigned char fillByte, size_t numBytes)
{
	Patch patch(name, address, fillByte, numBytes);

	patch.Orig.resize(numBytes);
	Pointer(address).Read(patch.Orig.data(), patch.Orig.size());

	patches.push_back(patch);
	return &patches.back();
}

/// <summary>
/// Adds a hook to the manager.
/// </summary>
/// <param name="name">The hooks name.</param>
/// <param name="address">The address to hook.</param>
/// <param name="destFunc">The dest function.</param>
/// <param name="type">The type of hook.</param>
/// <returns>The created <see cref="Hook"/>.</returns>
Hook* PatchManager::AddHook(std::string name, size_t address, void* destFunc, HookType type)
{
	Hook hook(name, address, destFunc, type);

	auto patchData = GetHookBytes(&hook);
	hook.Orig.resize(patchData.size());
	Pointer(address).Read(hook.Orig.data(), hook.Orig.size());

	hooks.push_back(hook);
	return &hooks.back();
}

/// <summary>
/// Adds a set of patches/hooks to the manager.
/// </summary>
/// <param name="name">The name of the patchset.</param>
/// <param name="patches">The patches to include in the patchset.</param>
/// <param name="hooks">The hooks to include in the patchset.</param>
/// <returns>The created <see cref="PatchSet"/>.</returns>
PatchSet* PatchManager::AddPatchSet(std::string name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks)
{
	PatchSet patchSet(name, patches, hooks);

	for (auto patch : patchSet.Patches)
	{
		patch.Orig.resize(patch.Data.size());
		Pointer(patch.Address).Read(patch.Orig.data(), patch.Orig.size());
	}

	for (auto hook : patchSet.Hooks)
	{
		auto patchData = GetHookBytes(&hook);
		hook.Orig.resize(patchData.size());
		Pointer(hook.Address).Read(hook.Orig.data(), hook.Orig.size());
	}

	patchSets.push_back(patchSet);
	return &patchSets.back();
}

/// <summary>
/// Looks up a patch based on its name.
/// </summary>
/// <param name="name">The name of the patch.</param>
/// <returns>A pointer to the patch, if found.</returns>
Patch* PatchManager::FindPatch(std::string name)
{
	for (auto it = patches.begin(); it != patches.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Looks up a hook based on its name.
/// </summary>
/// <param name="name">The name of the hook.</param>
/// <returns>A pointer to the hook, if found.</returns>
Hook* PatchManager::FindHook(std::string name)
{
	for (auto it = hooks.begin(); it != hooks.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Looks up a patch set based on its name.
/// </summary>
/// <param name="name">The name of the patch set.</param>
/// <returns>A pointer to the patch set, if found.</returns>
PatchSet* PatchManager::FindPatchSet(std::string name)
{
	for (auto it = patchSets.begin(); it != patchSets.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Toggles a patch based on its name.
/// </summary>
/// <param name="name">The name of the patch.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::TogglePatch(std::string name)
{
	Patch* patch = FindPatch(name);
	if (!patch)
		return PatchStatus::NotFound;

	return TogglePatch(patch) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a hook based on its name.
/// </summary>
/// <param name="name">The name of the hook.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::ToggleHook(std::string name)
{
	Hook* hook = FindHook(name);
	if (!hook)
		return PatchStatus::NotFound;

	return ToggleHook(hook) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a patch set (and all children patches/hooks) based on its name.
/// </summary>
/// <param name="name">The name of the patch set.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::TogglePatchSet(std::string name)
{
	PatchSet* patchSet = FindPatchSet(name);
	if (!patchSet)
		return PatchStatus::NotFound;

	return TogglePatchSet(patchSet) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a patch.
/// </summary>
/// <param name="patch">The patch to toggle.</param>
/// <returns>true if the patch is active, false if not.</returns>
bool PatchManager::TogglePatch(Patch* patch)
{
	if (patch->Enabled)
		Pointer(patch->Address).Write(patch->Orig.data(), patch->Orig.size());
	else
		Pointer(patch->Address).Write(patch->Data.data(), patch->Data.size());

	patch->Enabled = !patch->Enabled;
	return patch->Enabled;
}

/// <summary>
/// Toggles a hook.
/// </summary>
/// <param name="hook">The hook to toggle.</param>
/// <returns>true if the hook is active, false if not.</returns>
bool PatchManager::ToggleHook(Hook* hook)
{
	if (hook->Enabled)
		Pointer(hook->Address).Write(hook->Orig.data(), hook->Orig.size());
	else
	{
		auto hookData = GetHookBytes(hook);
		Pointer(hook->Address).Write(hookData.data(), hookData.size());
	}

	hook->Enabled = !hook->Enabled;
	return hook->Enabled;
}

/// <summary>
/// Toggles a patch set (and all children patches/hooks).
/// </summary>
/// <param name="patchSet">The patch set to toggle.</param>
/// <returns>true if the hook is active, false if not.</returns>
bool PatchManager::TogglePatchSet(PatchSet* patchSet)
{
	for (auto it = patchSet->Patches.begin(); it != patchSet->Patches.end(); ++it)
		TogglePatch(&(*it));

	for (auto it = patchSet->Hooks.begin(); it != patchSet->Hooks.end(); ++it)
		ToggleHook(&(*it));

	patchSet->Enabled = !patchSet->Enabled;
	return patchSet->Enabled;
}

/// <summary>
/// Enables/disables a patch based on its name.
/// </summary>
/// <param name="name">The name of the patch.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the patch.</returns>
PatchStatus PatchManager::EnablePatch(std::string name, bool enable)
{
	Patch* patch = FindPatch(name);
	if (!patch)
		return PatchStatus::NotFound;

	return EnablePatch(patch, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a hook based on its name.
/// </summary>
/// <param name="hook">The name of the hook.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the hook.</returns>
PatchStatus PatchManager::EnableHook(std::string name, bool enable)
{
	Hook* hook = FindHook(name);
	if (!hook)
		return PatchStatus::NotFound;

	return EnableHook(hook, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a patch set based on its name.
/// </summary>
/// <param name="patchSet">The name of the patch set.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the patch set.</returns>
PatchStatus PatchManager::EnablePatchSet(std::string name, bool enable)
{
	PatchSet* patchSet = FindPatchSet(name);
	if (!patchSet)
		return PatchStatus::NotFound;

	return EnablePatchSet(patchSet, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a patch.
/// </summary>
/// <param name="patch">The patch.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the patch.</returns>
bool PatchManager::EnablePatch(Patch* patch, bool enable)
{
	if (patch->Enabled == enable)
		return true; // patch is already set to this
	return TogglePatch(patch);
}

/// <summary>
/// Enables/disables a hook.
/// </summary>
/// <param name="hook">The hook.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the hook.</returns>
bool PatchManager::EnableHook(Hook* hook, bool enable)
{
	if (hook->Enabled == enable)
		return true; // hook is already set to this
	return ToggleHook(hook);
}

/// <summary>
/// Enables/disables a patch set.
/// </summary>
/// <param name="patchSet">The patch set.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the patch set.</returns>
bool PatchManager::EnablePatchSet(PatchSet* patchSet, bool enable)
{
	if (patchSet->Enabled == enable)
		return true; // patchset is already set to this
	return TogglePatchSet(patchSet);
}