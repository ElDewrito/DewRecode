#pragma once

namespace Blam
{
	class ArrayGlobal
	{
		char Name[0x20];
		int MaxEntries;
		int EntrySize;
		uint16_t Unk28;
		uint16_t Unk2A;
		int AllocTag;
		int AllocFunc;
		int AddedEntryCount; // number of entries that have been added (can be different from EntryCount since some globals have an entry added by default?)
		int EntryCount;
		int Unk3C; // dupe of EntryCount?
		uint16_t NextEntry;
		uint16_t Unk42;
		void* FirstEntry;
		void* LastEntry;
		int FirstEntryOffset;
		int LastEntryOffset;

	public:
		Pointer GetEntry(int entryNum)
		{
			if (entryNum >= EntryCount)
				return nullptr;
			return Pointer((char*)FirstEntry + (entryNum * EntrySize));
		}

		int GetCount()
		{
			return EntryCount;
		}

		Pointer AddEntry()
		{
			typedef int(__cdecl *Globals_ArrayPushPtr)(void* arrayGlobal);
			auto Globals_ArrayPush = reinterpret_cast<Globals_ArrayPushPtr>(0x55B410);
			int dataIdx = Globals_ArrayPush(this);
			if (dataIdx == -1)
				return nullptr;
			return GetEntry((uint16_t)dataIdx);
		}
	};
}