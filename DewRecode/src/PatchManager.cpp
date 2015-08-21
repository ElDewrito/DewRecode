#include "PatchManager.hpp"
#include <ElDorito/Pointer.hpp>

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

	uint32_t patchSize = (hook->Type == HookType::JmpIfEqual || hook->Type == HookType::JmpIfNotEqual) ? 6 : 5;
	uint32_t JMPSize = ((uint32_t)hook->DestFunc - (uint32_t)hook->Address - patchSize);

	if (hook->Type == HookType::JmpIfEqual || hook->Type == HookType::JmpIfNotEqual)
	{
		if (hook->Type == HookType::JmpIfNotEqual)
			tempJE[1] = 0x85;

		memcpy(&tempJE[2], &JMPSize, 4);
		return std::vector<unsigned char>(tempJE, tempJE + 6);
	}
	else //(hook->Type == HookType::None || hook->Type == HookType::Call)
	{
		memcpy(&tempJMP[1], &JMPSize, 4);
		return std::vector<unsigned char>(tempJMP, tempJMP + 5);
	}
	return{};
}

/// <summary>
/// Adds an already created <see cref="Patch"/> to the manager.
/// </summary>
/// <param name="patch">The <see cref="Patch"/> to add.</param>
/// <returns>A pointer to the created <see cref="Patch"/>.</returns>
Patch* PatchManager::Add(Patch patch)
{
	patch.Orig.resize(patch.Data.size());
	Pointer(patch.Address).Read(patch.Orig.data(), patch.Orig.size());

	patches.push_back(patch);
	return &patches.back();
}

/// <summary>
/// Adds an already created <see cref="Hook"/> to the manager.
/// </summary>
/// <param name="hook">The <see cref="Hook"/> to add.</param>
/// <returns>A pointer to the created <see cref="Hook"/>.</returns>
Hook* PatchManager::Add(Hook hook)
{
	auto patchData = GetHookBytes(&hook);
	hook.Orig.resize(patchData.size());
	Pointer(hook.Address).Read(hook.Orig.data(), hook.Orig.size());

	hooks.push_back(hook);
	return &hooks.back();
}

/// <summary>
/// Adds an already created <see cref="PatchSet"/> to the manager.
/// </summary>
/// <param name="patchSet">The <see cref="PatchSet"/> to add.</param>
/// <returns>A pointer to the created <see cref="PatchSet"/>.</returns>
PatchSet* PatchManager::Add(PatchSet patchSet)
{
	for (auto& patch : patchSet.Patches)
	{
		patch.Orig.resize(patch.Data.size());
		Pointer(patch.Address).Read(patch.Orig.data(), patch.Orig.size());
	}

	for (auto& hook : patchSet.Hooks)
	{
		auto patchData = GetHookBytes(&hook);
		hook.Orig.resize(patchData.size());
		Pointer(hook.Address).Read(hook.Orig.data(), hook.Orig.size());
	}

	patchSets.push_back(patchSet);
	return &patchSets.back();
}

/// <summary>
/// Adds a patch to the manager.
/// </summary>
/// <param name="name">The patches name.</param>
/// <param name="address">The address to patch.</param>
/// <param name="data">The data to write.</param>
/// <returns>The created <see cref="Patch"/>.</returns>
Patch* PatchManager::AddPatch(const std::string& name, size_t address, const PatchInitializerListType& data)
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
Patch* PatchManager::AddPatch(const std::string& name, size_t address, unsigned char fillByte, size_t numBytes)
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
Hook* PatchManager::AddHook(const std::string& name, size_t address, void* destFunc, HookType type)
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
PatchSet* PatchManager::AddPatchSet(const std::string& name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks)
{
	PatchSet patchSet(name, patches, hooks);

	for (auto& patch : patchSet.Patches)
	{
		patch.Orig.resize(patch.Data.size());
		Pointer(patch.Address).Read(patch.Orig.data(), patch.Orig.size());
	}

	for (auto& hook : patchSet.Hooks)
	{
		auto patchData = GetHookBytes(&hook);
		hook.Orig.resize(patchData.size());
		Pointer(hook.Address).Read(hook.Orig.data(), hook.Orig.size());
	}

	patchSets.push_back(patchSet);
	return &patchSets.back();
}

/// <summary>
/// Looks up a <see cref="Patch"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="Patch"/>.</param>
/// <returns>A pointer to the <see cref="Patch"/>, if found.</returns>
Patch* PatchManager::FindPatch(const std::string& name)
{
	for (auto it = patches.begin(); it != patches.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Looks up a <see cref="Hook"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="Hook"/>.</param>
/// <returns>A pointer to the <see cref="Hook"/>, if found.</returns>
Hook* PatchManager::FindHook(const std::string& name)
{
	for (auto it = hooks.begin(); it != hooks.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Looks up a <see cref="PatchSet"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="PatchSet"/>.</param>
/// <returns>A pointer to the <see cref="PatchSet"/>, if found.</returns>
PatchSet* PatchManager::FindPatchSet(const std::string& name)
{
	for (auto it = patchSets.begin(); it != patchSets.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

/// <summary>
/// Toggles a <see cref="Patch"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="Patch"/>.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::TogglePatch(const std::string& name)
{
	Patch* patch = FindPatch(name);
	if (!patch)
		return PatchStatus::NotFound;

	return TogglePatch(patch) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a <see cref="Hook"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="Hook"/>.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::ToggleHook(const std::string& name)
{
	Hook* hook = FindHook(name);
	if (!hook)
		return PatchStatus::NotFound;

	return ToggleHook(hook) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a <see cref="PatchSet"/> (and all children patches/hooks) based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="PatchSet"/>.</param>
/// <returns>PatchStatus enum</returns>
PatchStatus PatchManager::TogglePatchSet(const std::string& name)
{
	PatchSet* patchSet = FindPatchSet(name);
	if (!patchSet)
		return PatchStatus::NotFound;

	return TogglePatchSet(patchSet) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Toggles a <see cref="Patch"/>.
/// </summary>
/// <param name="patch">The <see cref="Patch"/> to toggle.</param>
/// <returns>true if the <see cref="Patch"/> is active, false if not.</returns>
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
/// Toggles a <see cref="Hook"/>.
/// </summary>
/// <param name="hook">The <see cref="Hook"/> to toggle.</param>
/// <returns>true if the <see cref="Hook"/> is active, false if not.</returns>
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
/// Toggles a <see cref="PatchSet"/> (and all children patches/hooks).
/// </summary>
/// <param name="patchSet">The <see cref="PatchSet"/> to toggle.</param>
/// <returns>true if the <see cref="PatchSet"/> is active, false if not.</returns>
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
/// Enables/disables a <see cref="Patch"/> based on its name.
/// </summary>
/// <param name="name">The name of the <see cref="Patch"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="Patch"/>.</returns>
PatchStatus PatchManager::EnablePatch(const std::string& name, bool enable)
{
	Patch* patch = FindPatch(name);
	if (!patch)
		return PatchStatus::NotFound;

	return EnablePatch(patch, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a <see cref="Hook"/> based on its name.
/// </summary>
/// <param name="hook">The name of the <see cref="Hook"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="Hook"/>.</returns>
PatchStatus PatchManager::EnableHook(const std::string& name, bool enable)
{
	Hook* hook = FindHook(name);
	if (!hook)
		return PatchStatus::NotFound;

	return EnableHook(hook, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a <see cref="PatchSet"/> based on its name.
/// </summary>
/// <param name="patchSet">The name of the <see cref="PatchSet"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="PatchSet"/>.</returns>
PatchStatus PatchManager::EnablePatchSet(const std::string& name, bool enable)
{
	PatchSet* patchSet = FindPatchSet(name);
	if (!patchSet)
		return PatchStatus::NotFound;

	return EnablePatchSet(patchSet, enable) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

/// <summary>
/// Enables/disables a <see cref="Patch"/>.
/// </summary>
/// <param name="patch">A pointer to the <see cref="Patch"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="Patch"/>.</returns>
bool PatchManager::EnablePatch(Patch* patch, bool enable)
{
	if (patch->Enabled == enable)
		return true; // patch is already set to this
	return TogglePatch(patch);
}

/// <summary>
/// Enables/disables a <see cref="Hook"/>.
/// </summary>
/// <param name="hook">A pointer to the <see cref="Hook"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="Hook"/>.</returns>
bool PatchManager::EnableHook(Hook* hook, bool enable)
{
	if (hook->Enabled == enable)
		return true; // hook is already set to this
	return ToggleHook(hook);
}

/// <summary>
/// Enables/disables a <see cref="PatchSet"/>.
/// </summary>
/// <param name="patchSet">A pointer to the <see cref="PatchSet"/>.</param>
/// <param name="enable">Whether to enable it or not (default true)</param>
/// <returns>The status of the <see cref="PatchSet"/>.</returns>
bool PatchManager::EnablePatchSet(PatchSet* patchSet, bool enable)
{
	if (patchSet->Enabled == enable)
		return true; // patchset is already set to this
	return TogglePatchSet(patchSet);
}