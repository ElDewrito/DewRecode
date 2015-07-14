#include "PatchManager.hpp"
#include "../include/ElDorito/Pointer.hpp"

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

Patch* PatchManager::AddPatch(std::string name, size_t address, const PatchInitializerListType& data)
{
	Patch patch = { name, address, data, {}, false };

	patch.Orig.resize(data.size());
	Pointer(address).Read(patch.Orig.data(), patch.Orig.size());

	patches.push_back(patch);
	return &patches.back();
}

Hook* PatchManager::AddHook(std::string name, size_t address, void* destFunc, HookType type)
{
	Hook hook = { name, address, destFunc, type, {}, false };

	auto patchData = GetHookBytes(&hook);
	hook.Orig.resize(patchData.size());
	Pointer(address).Read(hook.Orig.data(), hook.Orig.size());

	hooks.push_back(hook);
	return &hooks.back();
}

PatchSet* PatchManager::AddPatchSet(std::string name, const PatchSetInitializerListType& patches, const PatchSetHookInitializerListType& hooks)
{
	PatchSet patchSet = { name, patches, hooks, false };

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

Patch* PatchManager::FindPatch(std::string name)
{
	for (auto it = patches.begin(); it != patches.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

Hook* PatchManager::FindHook(std::string name)
{
	for (auto it = hooks.begin(); it != hooks.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

PatchSet* PatchManager::FindPatchSet(std::string name)
{
	for (auto it = patchSets.begin(); it != patchSets.end(); ++it)
	{
		if (!(*it).Name.compare(name))
			return &(*it);
	}
	return nullptr;
}

PatchStatus PatchManager::TogglePatch(std::string name)
{
	Patch* patch = FindPatch(name);
	if (!patch)
		return PatchStatus::NotFound;

	return TogglePatch(patch) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

PatchStatus PatchManager::ToggleHook(std::string name)
{
	Hook* hook = FindHook(name);
	if (!hook)
		return PatchStatus::NotFound;

	return ToggleHook(hook) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

PatchStatus PatchManager::TogglePatchSet(std::string name)
{
	PatchSet* patchSet = FindPatchSet(name);
	if (!patchSet)
		return PatchStatus::NotFound;

	return TogglePatchSet(patchSet) ? PatchStatus::Enabled : PatchStatus::Disabled;
}

bool PatchManager::TogglePatch(Patch* patch)
{
	if (patch->Enabled)
		Pointer(patch->Address).Write(patch->Orig.data(), patch->Orig.size());
	else
		Pointer(patch->Address).Write(patch->Data.data(), patch->Data.size());

	patch->Enabled = !patch->Enabled;
	return patch->Enabled;
}

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

bool PatchManager::TogglePatchSet(PatchSet* patchSet)
{
	for (auto it = patchSet->Patches.begin(); it != patchSet->Patches.end(); ++it)
		TogglePatch(&(*it));

	for (auto it = patchSet->Hooks.begin(); it != patchSet->Hooks.end(); ++it)
		ToggleHook(&(*it));

	patchSet->Enabled = !patchSet->Enabled;
	return patchSet->Enabled;
}
