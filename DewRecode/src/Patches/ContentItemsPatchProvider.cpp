#include "ContentItemsPatchProvider.hpp"
#include "../ElDorito.hpp"
#include <filesystem>
#include <ShlObj.h>

namespace
{
	bool IsProfileAvailable();
	char* __cdecl AllocateGameGlobalMemory2Hook(char* Src, int a2, int a3, char a4, void* a5);
	wchar_t* __stdcall GetContentMountPathHook(wchar_t* destPtr, int size, int unk);
	char __stdcall PackageMountHook(int a1, int a2, int a3, int a4);
	char __stdcall PackageCreateHook(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
	bool __fastcall SaveFileGetNameHook(uint8_t* blfStart, void* unused, int a2, wchar_t* Src, size_t MaxCount);

	wchar_t* __fastcall FS_GetFilePathForContentItemHook(uint8_t* contentItem, void* unused, wchar_t* dest, size_t MaxCount);
	char __fastcall FS_GetFiloForContentItemHook1(uint8_t* contentItem, void* unused, void* filo);
	char __fastcall FS_GetFiloForContentItemHook(uint8_t* contentItem, void* unused, void* filo);

	char CallsXEnumerateHook();
	char __fastcall Game_SetFlagAfterCopyBLFDataHook(uint8_t* flag, void* unused, char flagIdx, char set);
}

namespace ContentItems
{
	PatchSet ContentItemsPatchProvider::GetPatches()
	{
		PatchSet patches("ContentItemsPatches",
		{
			// Fix storage device checks, so storage device funcs return 0 instead of 0xCACACACA
			Patch("StorageCheck1", 0x5A7A80, { 0x31, 0xC0, 0xC3 }),
			Patch("StorageCheck2", 0x74D570, { 0x31, 0xC0, 0xC3 }),

			// needed for the game to load our items in "content items" global
			Patch("ContentItemsGlobal1", 0xADA59A, 0x90, 6),
			Patch("ContentItemsGlobal2", 0xADC0EA, 0x90, 6),

			// patch the func that handles game variant BLFs so it'll load byteswapped blfs
			Patch("BLFByteSwap1", 0x5731CD, 0x90, 2),
			Patch("BLFByteSwap2", 0x5732CD, 0x90, 2),

			// patch proper endian -1 into BLF header creation
			Patch("EndianSwap", 0x462AE8, { 0xFF, 0xFE }),

			// patch content items global size
			Patch("ContentItemGlobal", 0x5A7E44, { 0x0, 0x4, 0x0, 0x0 }), // 1024 ?

			// Hook the func that gets the save file dest. name, 
			Patch("SaveFileGetName1", 0x52708E, { 0x8B, 0x4D, 0x14 }), // pass blf data to our func
			Patch("SaveFileGetName2", 0x527096, 0x90, 3),
		},
		{
			// Allow saving content without a profile
			Hook("ProfileFix", 0xA7DCA0, IsProfileAvailable, HookType::Jmp),

			// Hook this AllocateGameGlobalMemory to use a different one (this one is outdated maybe? crashes when object is added to "content items" global without this hook)
			Hook("ContentItemsGlobalFix", 0x55B010, AllocateGameGlobalMemory2Hook, HookType::Jmp),

			// Hook GetContentMountPath to actually return a dest folder
			Hook("ContentMountPath", 0x74CC00, GetContentMountPathHook, HookType::Jmp),

			// Hook (not patch, important otherwise stack gets fucked) content_catalogue_create_new_XContent_item stub to return true
			Hook("XContentCreate", 0x74CBE0, PackageCreateHook, HookType::Jmp),

			// Hook (not patch, like above) content package mount stub to return true
			Hook("XContentMount", 0x74D010, PackageMountHook, HookType::Jmp),

			// Hook the func that gets the save file dest. name, 
			Hook("SaveFileGetName3", 0x527091, SaveFileGetNameHook, HookType::Call),

			Hook("XEnumerate", 0x5A9050, CallsXEnumerateHook, HookType::Jmp),
			Hook("Filo1", 0x74CE00, FS_GetFiloForContentItemHook, HookType::Jmp), // game doesnt seem to use the filo for this? maybe used for overwriting
			Hook("Filo2", 0x74CE70, FS_GetFilePathForContentItemHook, HookType::Jmp),
			Hook("Filo3", 0x74CCF0, FS_GetFiloForContentItemHook1, HookType::Jmp),

			Hook("BLFFlag", 0x74D376, Game_SetFlagAfterCopyBLFDataHook, HookType::Call),
		});

		return patches;
	}
}

namespace
{
	Blam::ArrayGlobal* contentItemsGlobal = 0;
	bool enumerated = false;

	bool AddContentItem(wchar_t* itemPath)
	{
		auto& dorito = ElDorito::Instance();

		auto path = dorito.Utils.ThinString(std::wstring(itemPath));
		dorito.Logger.Log(LogSeverity::Debug, "ContentItems", "Adding content item from %s...", path.c_str());

		uint8_t fileData[0xF0];

		FILE* file;
		if (_wfopen_s(&file, itemPath, L"rb") != 0 || !file)
			return false;

		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);
		if (fileSize < 0x40)
		{
			// too small to be a BLF
			fclose(file);
			return false;
		}

		uint32_t magic;
		fread(&magic, 4, 1, file);
		if (magic != 0x5F626C66 && magic != 0x666C625F)
		{
			// not a BLF
			fclose(file);
			return false;
		}

		fseek(file, 0x40, SEEK_SET);
		fread(fileData, 1, 0xF0, file);
		fclose(file);

		dorito.Logger.Log(LogSeverity::Debug, "ContentItems", "Content item checks passed, adding entry to content items global!");

		auto dataPtr = contentItemsGlobal->AddEntry();
		if (dataPtr == nullptr)
			return false;

		dataPtr(0x4).Write<uint32_t>(0x11);
		dataPtr(0x8).Write<uint32_t>(4); // this is a blf/variant/content item type field, but setting it to 4 (slayer) works for everything
		dataPtr(0xC).Write<char*>((char*)dataPtr);

		memcpy((char*)dataPtr + 0x10, fileData, 0xF0);
		wcscpy_s((wchar_t*)((char*)dataPtr + 0x100), 0xA0, itemPath);

		return true;
	}

	bool AddContentItem(const std::string& itemPath)
	{
		// need to convert path from ASCII to unicode now
		std::wstring unicode = ElDorito::Instance().Utils.WidenString(itemPath);
		return AddContentItem((wchar_t*)unicode.c_str());
	}

	void GetFilePathForItem(wchar_t* dest, size_t MaxCount, wchar_t* variantName, int variantType)
	{
		wchar_t currentDir[256];
		memset(currentDir, 0, 256 * sizeof(wchar_t));
		GetCurrentDirectoryW(256, currentDir);

		if (variantType == 10)
			swprintf_s(dest, MaxCount, L"%ls\\mods\\maps\\%ls\\", currentDir, variantName);
		else
			swprintf_s(dest, MaxCount, L"%ls\\mods\\variants\\%ls\\", currentDir, variantName);

		int res = SHCreateDirectoryExW(NULL, dest, NULL);
		auto& dorito = ElDorito::Instance();

		auto destFriendly = dorito.Utils.ThinString(std::wstring(dest));
		dorito.Logger.Log(LogSeverity::Debug, "ContentItems", "GetFilePathForItem: SHCreateDirectoryExW(%s): %d", destFriendly.c_str(), res);

		if (variantType == 10)
			swprintf_s(dest, MaxCount, L"%ls\\mods\\maps\\%ls\\sandbox.map", currentDir, variantName);
		else
		{
			typedef wchar_t*(__cdecl *FS_GetFileNameForItemTypePtr)(uint32_t type);
			auto FS_GetFileNameForItemType = reinterpret_cast<FS_GetFileNameForItemTypePtr>(0x526550);
			wchar_t* fileName = FS_GetFileNameForItemType(variantType);

			swprintf_s(dest, MaxCount, L"%ls\\mods\\variants\\%ls\\%ls", currentDir, variantName, fileName);
		}

		auto path = dorito.Utils.ThinString(variantName);
		auto result = dorito.Utils.ThinString(dest);
		dorito.Logger.Log(LogSeverity::Debug, "ContentItems", "GetFilePathForItem(%s, %d): %s", path.c_str(), variantType, result.c_str());
	}

	void AddAllBLFContentItems(const std::string& path)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "Loading BLF files from %s...", path.c_str());

		for (std::tr2::sys::directory_iterator itr(path); itr != std::tr2::sys::directory_iterator(); ++itr)
		{
			if (std::tr2::sys::is_directory(itr->status()))
				AddAllBLFContentItems(itr->path());
			else
				AddContentItem(itr->path());
		}
	}

	char CallsXEnumerateHook()
	{
		if (!contentItemsGlobal)
			return 0;
		if (enumerated)
			return 1;

		// TODO: change this to use unicode instead of ASCII
		auto variantPath = std::tr2::sys::current_path<std::tr2::sys::path>();
		variantPath /= "mods";

		auto mapsPath = variantPath;
		variantPath /= "variants";
		mapsPath /= "maps";

		AddAllBLFContentItems(variantPath);
		AddAllBLFContentItems(mapsPath);

		enumerated = true;
		return 1;
	}

	void GetFilePathForContentItem(wchar_t* dest, size_t MaxCount, uint8_t* contentItem)
	{
		wchar_t* variantName = (wchar_t*)(contentItem + 0x18);
		uint32_t variantType = *(uint32_t*)(contentItem + 0xC8);
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "GetFilePathForContentItem called:");

		GetFilePathForItem(dest, MaxCount, variantName, variantType);
	}


	char __fastcall Game_SetFlagAfterCopyBLFDataHook(uint8_t* flag, void* unused, char flagIdx, char set)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "Game_SetFlagAfterCopyBLFDataHook(%d, %d)", (int)flagIdx, (int)set);

		typedef char(__fastcall *Game_SetFlagPtr)(uint8_t* flag, void* unused, char flagIdx, char set);
		auto Game_SetFlag = reinterpret_cast<Game_SetFlagPtr>(0x52BD40);
		char ret = Game_SetFlag(flag, unused, flagIdx, set);

		uint8_t* contentItem = flag - 4;

		*(uint32_t*)(contentItem + 4) = 0x11;
		*(uint32_t*)(contentItem + 8) = 4; // this is a blf/variant/content item type field, but setting it to 4 (slayer) works for everything
		*(uint32_t*)(contentItem + 0xC) = (uint32_t)contentItem;

		wchar_t* variantPath = (wchar_t*)(contentItem + 0x100);

		GetFilePathForContentItem(variantPath, 0x100, contentItem);

		return ret;
	}

	char __fastcall FS_GetFiloForContentItemHook(uint8_t* contentItem, void* unused, void* filo)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "FS_GetFiloForContentItemHook");

		typedef void*(__cdecl *FSCallsSetupFiloStruct2Ptr)(void* destFilo, wchar_t* Src, char unk);
		auto fsCallsSetupFiloStruct2 = reinterpret_cast<FSCallsSetupFiloStruct2Ptr>(0x5285B0);
		fsCallsSetupFiloStruct2(filo, (wchar_t*)(contentItem + 0x100), 0);

		return 1;
	}

	char __fastcall FS_GetFiloForContentItemHook1(uint8_t* contentItem, void* unused, void* filo)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "FS_GetFiloForContentItemHook1");

		typedef void*(__cdecl *FSCallsSetupFiloStruct2Ptr)(void* destFilo, wchar_t* Src, char unk);
		auto fsCallsSetupFiloStruct2 = reinterpret_cast<FSCallsSetupFiloStruct2Ptr>(0x5285B0);
		fsCallsSetupFiloStruct2(filo, (wchar_t*)(contentItem + 0x100), 1);

		return 1;
	}

	wchar_t* __fastcall FS_GetFilePathForContentItemHook(uint8_t* contentItem, void* unused, wchar_t* dest, size_t MaxCount)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "FS_GetFilePathForContentItemHook");

		// copy the path we put in the unused XCONTENT_DATA field in the content item global
		wcscpy_s(dest, MaxCount, (wchar_t*)(contentItem + 0x100));
		return dest;
	}

	bool __fastcall SaveFileGetNameHook(uint8_t* blfStart, void* unused, int a2, wchar_t* Src, size_t MaxCount)
	{
		// TODO: moving this to Documents\\My Games\\Halo 3\\Saves\\Maps / Games / etc might be better, no need to worry about admin perms

		wchar_t* variantName = (wchar_t*)(blfStart + 0x48); // when saving forge maps we only get the variant name, not the blf data :s
		int variantType = *(uint32_t*)(blfStart + 0xF8);
		auto nameStr = ElDorito::Instance().Utils.ThinString(std::wstring(variantName));
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "SaveFileGetNameHook(%s, %d)", nameStr.c_str(), variantType);

		GetFilePathForItem(Src, MaxCount, variantName, variantType);

		return 1;
	}

	char __stdcall PackageCreateHook(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "PackageCreateHook returning 1");
		return 1;
	}

	char __stdcall PackageMountHook(int a1, int a2, int a3, int a4)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "PackageMountHook returning 1");
		return 1;
	}

	wchar_t* __stdcall GetContentMountPathHook(wchar_t* destPtr, int size, int unk)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "GetContentMountPathHook");

		// TODO: move this to temp folder, or find a way to disable it (game uses path returned by this func to create a 0 byte sandbox.map)
		wchar_t currentDir[256];
		memset(currentDir, 0, 256 * sizeof(wchar_t));
		GetCurrentDirectoryW(256, currentDir);

		swprintf_s(destPtr, size, L"%ls\\mods\\temp\\", currentDir);

		int res = SHCreateDirectoryExW(NULL, destPtr, NULL);
		auto& dorito = ElDorito::Instance();

		auto destFriendly = dorito.Utils.ThinString(std::wstring(destPtr));
		dorito.Logger.Log(LogSeverity::Debug, "ContentItems", "GetContentMountPathHook: SHCreateDirectoryExW(%s): %d", destFriendly.c_str(), res);

		return destPtr;
	}

	char* __cdecl AllocateGameGlobalMemory2Hook(char* Src, int a2, int a3, char a4, void* a5)
	{
		typedef char* (__cdecl *AllocateGameGlobalMemoryPtr)(char* Src, int a2, int a3, char a4, void* a5);
		auto AllocateGameGlobalMemory = reinterpret_cast<AllocateGameGlobalMemoryPtr>(0x55AFA0);
		char* retData = AllocateGameGlobalMemory(Src, a2, a3, a4, a5);

		*(uint16_t*)(retData + 0x2A) = 0x3;

		if (Src == (char*)0x165B170) // "content items"
		{
			ElDorito::Instance().Logger.Log(LogSeverity::Debug, "ContentItems", "AllocateGameGlobalMemory2Hook: allocating content items global");

			// set some bytes thats needed in the contentfile related funcs
			*(uint8_t*)(retData + 0x29) = 1;
			*(uint32_t*)(retData + 0x40) = 1;
			contentItemsGlobal = reinterpret_cast<Blam::ArrayGlobal*>(retData);
		}

		return retData;
	}

	bool IsProfileAvailable()
	{
		return true;
	}
}