#include "NetworkPatchProvider.hpp"
#include "../ElDorito.hpp"
#include <ElDorito/Blam/BitStream.hpp>
#include <ElDorito/CustomPackets.hpp>

#include <unordered_map>
#include <limits>

namespace
{
	char* Network_GetIPStringFromInAddr(void* inaddr);
	char Network_XnAddrToInAddrHook(void* pxna, void* pxnkid, void* in_addr);
	char Network_InAddrToXnAddrHook(void* ina, void* pxna, void* pxnkid);

	bool __fastcall PeerRequestPlayerDesiredPropertiesUpdateHook(uint8_t* thisPtr, void* unused, uint32_t arg0, uint32_t arg4, void* properties, uint32_t argC);
	void __fastcall ApplyPlayerPropertiesExtended(uint8_t* thisPtr, void* unused, int playerIndex, uint32_t arg4, uint32_t arg8, uint8_t* properties, uint32_t arg10);
	void __fastcall RegisterPlayerPropertiesPacketHook(void* thisPtr, void* unused, int packetId, const char* packetName, int arg8, int size1, int size2, void* serializeFunc, void* deserializeFunc, int arg1C, int arg20);
	void SerializePlayerPropertiesHook(Blam::BitStream* stream, uint8_t* buffer, bool flag);
	bool DeserializePlayerPropertiesHook(Blam::BitStream* stream, uint8_t* buffer, bool flag);

	const int CustomPacketId = 0x27; // Last packet ID used by the game is 0x26

	void InitializePacketsHook();
	void HandlePacketHook();

	void SerializeCustomPacket(Blam::BitStream *stream, int packetSize, const void *packet);
	bool DeserializeCustomPacket(Blam::BitStream *stream, int packetSize, void *packet);
}
namespace Network
{
	NetworkPatchProvider::NetworkPatchProvider()
	{
		auto& patches = ElDorito::Instance().PatchManager;

		PatchDedicatedServer = patches.AddPatchSet("PatchDedicatedServer", {			
			/* Patches for each call to Game_IsAppDedicatedServer
			
			Patch("ModePatch01", 0x42E903, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch02", 0x4370F2, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // Called every tick
			Patch("ModePatch03", 0x4378C0, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // Called on team change
			Patch("ModePatch04", 0x45A8B4, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // SyslinkFix3
			Patch("ModePatch05", 0x45B58F, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch06", 0x49E565, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch07", 0x51BD99, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch08", 0x52D623, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // SyslinkFix1
			Patch("ModePatch09", 0x52D673, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // SyslinkFix2
			Patch("ModePatch10", 0x52FAE9, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch11", 0x567915, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch12", 0x600639, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch13", 0x600670, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch14", 0x6006F0, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch15", 0x600722, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch16", 0x600799, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch17", 0x6007B9, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch18", 0x600886, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // Called every tick
			Patch("ModePatch19", 0x600EB0, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch20", 0x63381B, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),
			Patch("ModePatch21", 0x635123, { 0xB0, 0x01, 0x90, 0x90, 0x90 }), // Called every tick
			Patch("ModePatch22", 0x85DEEC, { 0xB0, 0x01, 0x90, 0x90, 0x90 }),*/

			Patch("ModePatch", 0x42E600, { 0xB0, 0x01 }),
			Patch("SyslinkFix1", 0x52D62E, { 0xEB }),
			Patch("SyslinkFix2", 0x52D67A, { 0xEB }),
			Patch("SyslinkFix3", 0x45A8BB, { 0xEB }),
			Patch("UICrashFix", 0x109C5E0, { 0xC2, 0x08, 0x00 }),
			Patch("TeamChangeFix", 0x4378C0, { 0xB0, 0x00, 0x90, 0x90, 0x90 })
		});

		// TODO: move this somewhere else
		PatchDisableD3D = patches.AddPatchSet("PatchDisableD3D", {
			// forces the game to use a null d3d device
			Patch("D3DUseNullDevice", 0x42EBBB, { 0x1 }),

			// fixes an exception that happens with null d3d
			Patch("D3DExceptCall", 0xA75E30, { 0xC3 }),

			// fixes the game being stuck in some d3d-related while loop
			Patch("D3DFreeze", 0xA22290, { 0xC3 }),

			// stops D3DDevice->EndScene from being called
			Patch("D3DEndScene", 0xA21796, 0x90, 6) // TODO: set eax?
		});
	}
	PatchSet NetworkPatchProvider::GetPatches()
	{
		auto versionNum = Utils::Misc::ConvertToVector<uint32_t>(Utils::Version::GetVersionInt());

		PatchSet patches("NetworkPatches",
		{
			Patch("VersionPatch1", 0x501421, versionNum),
			Patch("VersionPatch2", 0x50143A, versionNum)
		},
		{
			// Fix network debug strings having (null) instead of an IP address
			Hook("IPStringFromInAddr", 0x43F6F0, Network_GetIPStringFromInAddr, HookType::Jmp),

			// Fix for XnAddrToInAddr to try checking syslink-menu data for XnAddr->InAddr mapping before consulting XNet
			Hook("XnAddrToInAddr", 0x430B6C, Network_XnAddrToInAddrHook, HookType::Call),
			Hook("InAddrToXnAddr", 0x430F51, Network_InAddrToXnAddrHook, HookType::Call),

			// Player-properties packet hooks
			Hook("PlayerProperties1", 0x45DD20, PeerRequestPlayerDesiredPropertiesUpdateHook, HookType::Jmp),
			Hook("PlayerProperties2", 0x4DAF4F, ApplyPlayerPropertiesExtended, HookType::Call),
			Hook("PlayerProperties3", 0x4DFF7E, RegisterPlayerPropertiesPacketHook, HookType::Call),
			Hook("PlayerProperties4", 0x4DFD53, SerializePlayerPropertiesHook, HookType::Call),
			Hook("PlayerProperties5", 0x4DE178, DeserializePlayerPropertiesHook, HookType::Call),

			Hook("InitializePackets", 0x49E226, InitializePacketsHook, HookType::Call),
			Hook("HandlePacket", 0x49CAFA, HandlePacketHook, HookType::Jmp),
		});

		return patches;
	}

	bool NetworkPatchProvider::SetDedicatedServerMode(bool isDedicated)
	{
		ElDorito::Instance().PatchManager.EnablePatchSet(PatchDedicatedServer, isDedicated);
		return isDedicated;
	}

	bool NetworkPatchProvider::GetDedicatedServerMode()
	{
		return PatchDedicatedServer->Enabled;
	}

	bool NetworkPatchProvider::SetD3DDisabled(bool isDisabled)
	{
		ElDorito::Instance().PatchManager.EnablePatchSet(PatchDisableD3D, isDisabled);
		return isDisabled;
	}

	bool NetworkPatchProvider::GetD3DDisabled()
	{
		return PatchDisableD3D->Enabled;
	}
}

namespace
{
	char* Network_GetIPStringFromInAddr(void* inaddr)
	{
		static char ipAddrStr[64];
		memset(ipAddrStr, 0, 64);

		uint8_t ip3 = *(uint8_t*)inaddr;
		uint8_t ip2 = *((uint8_t*)inaddr + 1);
		uint8_t ip1 = *((uint8_t*)inaddr + 2);
		uint8_t ip0 = *((uint8_t*)inaddr + 3);

		uint16_t port = *(uint16_t*)((uint8_t*)inaddr + 0x10);
		uint16_t type = *(uint16_t*)((uint8_t*)inaddr + 0x12);

		sprintf_s(ipAddrStr, 64, "%hd.%hd.%hd.%hd:%hd (%hd)", ip0, ip1, ip2, ip3, port, type);

		return ipAddrStr;
	}

	char Network_XnAddrToInAddrHook(void* pxna, void* pxnkid, void* in_addr)
	{
		uint32_t maxMachines = *(uint32_t*)(0x228E6D8);
		uint8_t* syslinkDataPtr = (uint8_t*)*(uint32_t*)(0x228E6D8 + 0x4);

		for (uint32_t i = 0; i < maxMachines; i++)
		{
			uint8_t* entryPtr = syslinkDataPtr;
			syslinkDataPtr += 0x164F8;
			uint8_t entryStatus = *entryPtr;
			if (entryStatus == 0)
				continue;

			uint8_t* xnkidPtr = entryPtr + 0x9E;
			uint8_t* xnaddrPtr = entryPtr + 0xAE;
			uint8_t* ipPtr = entryPtr + 0x170;

			if (memcmp(pxna, xnaddrPtr, 0x10) == 0 && memcmp(pxnkid, xnkidPtr, 0x10) == 0)
			{
				// in_addr struct:
				// 0x0 - 0x10: IP (first 4 bytes for IPv4, 0x10 for IPv6)
				// 0x10 - 0x12: Port number
				// 0x12 - 0x14: IP length (4 for IPv4, 0x10 for IPv6)

				memset(in_addr, 0, 0x14);
				memcpy(in_addr, ipPtr, 4);

				*(uint16_t*)((uint8_t*)in_addr + 0x10) = 11774;
				*(uint16_t*)((uint8_t*)in_addr + 0x12) = 4;

				return 1;
			}
		}

		typedef char(*Network_XnAddrToInAddrPtr)(void* pxna, void* pxnkid, void* in_addr);
		auto Network_XnAddrToInAddr = reinterpret_cast<Network_XnAddrToInAddrPtr>(0x52D840);
		return Network_XnAddrToInAddr(pxna, pxnkid, in_addr);
	}

	char Network_InAddrToXnAddrHook(void* ina, void* pxna, void* pxnkid)
	{
		uint32_t maxMachines = *(uint32_t*)(0x228E6D8);
		uint8_t* syslinkDataPtr = (uint8_t*)*(uint32_t*)(0x228E6DC);

		for (uint32_t i = 0; i < maxMachines; i++)
		{
			uint8_t* entryPtr = syslinkDataPtr;
			syslinkDataPtr += 0x164F8;
			uint8_t entryStatus = *entryPtr;
			if (entryStatus == 0)
				continue;

			uint8_t* xnkidPtr = entryPtr + 0x9E;
			uint8_t* xnaddrPtr = entryPtr + 0xAE;
			uint8_t* ipPtr = entryPtr + 0x170;

			if (memcmp(ipPtr, ina, 0x4) == 0)
			{
				memcpy(pxna, xnaddrPtr, 0x10);
				memcpy(pxnkid, xnkidPtr, 0x10);

				return 1;
			}
		}

		typedef char(*Network_InAddrToXnAddrPtr)(void* ina, void* pxna, void* pxnkid);
		auto Network_InAddrToXnAddr = reinterpret_cast<Network_InAddrToXnAddrPtr>(0x52D970);
		return Network_InAddrToXnAddr(ina, pxna, pxnkid);
	}

	// Packet size constants
	const size_t PlayerPropertiesPacketHeaderSize = 0x18;
	const size_t PlayerPropertiesSize = 0x30;
	const size_t PlayerPropertiesPacketFooterSize = 0x4;

	size_t GetPlayerPropertiesPacketSize()
	{
		static size_t size;
		if (size == 0)
		{
			size_t extensionSize = ElDorito::Instance().PlayerPropertiesExtender.GetTotalSize();
			size = PlayerPropertiesPacketHeaderSize + PlayerPropertiesSize + extensionSize + PlayerPropertiesPacketFooterSize;
		}
		return size;
	}

	// Changes the size of the player-properties packet to include extension data
	void __fastcall RegisterPlayerPropertiesPacketHook(void* thisPtr, void* unused, int packetId, const char* packetName, int arg8, int size1, int size2, void* serializeFunc, void* deserializeFunc, int arg1C, int arg20)
	{
		size_t newSize = GetPlayerPropertiesPacketSize();
		typedef void(__thiscall *RegisterPacketPtr)(void* thisPtr, int packetId, const char* packetName, int arg8, int size1, int size2, void* serializeFunc, void* deserializeFunc, int arg1C, int arg20);
		auto RegisterPacket = reinterpret_cast<RegisterPacketPtr>(0x4801B0);
		RegisterPacket(thisPtr, packetId, packetName, arg8, newSize, newSize, serializeFunc, deserializeFunc, arg1C, arg20);
	}

	void SanitizePlayerName(wchar_t *name)
	{
		// Clamp the name length to 15 chars
		name[15] = '\0';

		int i, firstNonSpace = -1, lastNonSpace = -1;
		for (i = 0; name[i]; i++)
		{
			// Replace non-ASCII characters with a letter corresponding to the string position
			if (name[i] < 32 || name[i] > 126)
				name[i] = 'A' + i;

			// Replace double quotes with single quotes
			if (name[i] == '"')
				name[i] = '\'';

			// Track the first and last non-space chars
			if (name[i] != ' ')
			{
				if (firstNonSpace < 0)
					firstNonSpace = i;
				lastNonSpace = i;
			}
		}
		if (firstNonSpace < 0)
		{
			// String is all spaces
			wcscpy_s(name, 16, L"Forgot");
			return;
		}

		// Strip the spaces from the beginning and end
		auto newLength = lastNonSpace - firstNonSpace + 1;
		memmove(&name[0], &name[firstNonSpace], newLength * sizeof(wchar_t));
	}

	// Applies player properties data including extended properties
	void __fastcall ApplyPlayerPropertiesExtended(uint8_t* thisPtr, void* unused, int playerIndex, uint32_t arg4, uint32_t arg8, uint8_t* properties, uint32_t arg10)
	{
		// The player name is at the beginning of the block - sanitize it
		SanitizePlayerName(reinterpret_cast<wchar_t*>(properties));

		// Apply the base properties
		typedef void(__thiscall *ApplyPlayerPropertiesPtr)(void* thisPtr, int playerIndex, uint32_t arg4, uint32_t arg8, void* properties, uint32_t arg10);
		auto ApplyPlayerProperties = reinterpret_cast<ApplyPlayerPropertiesPtr>(0x450890);
		ApplyPlayerProperties(thisPtr, playerIndex, arg4, arg8, properties, arg10);

		// Apply the extended properties
		uint8_t* sessionData = thisPtr + 0x10A8 + playerIndex * 0x1648;
		ElDorito::Instance().PlayerPropertiesExtender.ApplyData(playerIndex, sessionData, properties + PlayerPropertiesSize);
	}

	// This completely replaces c_network_session::peer_request_player_desired_properties_update
	// Editing the existing function doesn't allow for a lot of flexibility
	bool __fastcall PeerRequestPlayerDesiredPropertiesUpdateHook(uint8_t* thisPtr, void* unused, uint32_t arg0, uint32_t arg4, void* properties, uint32_t argC)
	{
		int unk0 = *reinterpret_cast<int*>(thisPtr + 0x25B870);
		if (unk0 == 3)
			return false;

		// Get the player index
		int unk1 = *reinterpret_cast<int*>(thisPtr + 0x1A3D40);
		uint8_t* unk2 = thisPtr + 0x20;
		uint8_t* unk3 = unk2 + unk1 * 0xF8 + 0x118;
		typedef int(*GetPlayerIndexPtr)(void* arg0, int arg4);
		auto GetPlayerIndex = reinterpret_cast<GetPlayerIndexPtr>(0x52E280);
		int playerIndex = GetPlayerIndex(unk3, 0x10);
		if (playerIndex == -1)
			return false;

		// Copy the player properties to a new array and add the extension data
		size_t packetSize = GetPlayerPropertiesPacketSize();
		size_t extendedSize = packetSize - PlayerPropertiesPacketHeaderSize - PlayerPropertiesPacketFooterSize;
		auto extendedProperties = std::make_unique<uint8_t[]>(extendedSize);
		memcpy(&extendedProperties[0], properties, PlayerPropertiesSize);
		ElDorito::Instance().PlayerPropertiesExtender.BuildData(playerIndex, &extendedProperties[PlayerPropertiesSize]);

		if (unk0 == 6 || unk0 == 7)
		{
			// Apply player properties locally
			ApplyPlayerPropertiesExtended(unk2, NULL, playerIndex, arg0, arg4, &extendedProperties[0], argC);
		}
		else
		{
			// Send player properties across the network
			int unk5 = *reinterpret_cast<int*>(thisPtr + 0x30);
			int unk6 = *reinterpret_cast<int*>(thisPtr + 0x1A3D4C + unk5 * 0xC);
			if (unk6 == -1)
				return true;

			// Allocate the packet
			auto packet = std::make_unique<uint8_t[]>(packetSize);
			memset(&packet[0], 0, packetSize);

			// Initialize it
			int id = *reinterpret_cast<int*>(thisPtr + 0x25BBF0);
			typedef void(*InitPacketPtr)(int id, void* packet);
			auto InitPacket = reinterpret_cast<InitPacketPtr>(0x482040);
			InitPacket(id, &packet[0]);

			// Set up the header and footer
			*reinterpret_cast<int*>(&packet[0x10]) = arg0;
			*reinterpret_cast<uint32_t*>(&packet[0x14]) = arg4;
			*reinterpret_cast<uint32_t*>(&packet[packetSize - PlayerPropertiesPacketFooterSize]) = argC;

			// Copy the player properties structure in
			memcpy(&packet[PlayerPropertiesPacketHeaderSize], &extendedProperties[0], extendedSize);

			// Send!
			int unk7 = *reinterpret_cast<int*>(thisPtr + 0x10);
			void* networkObserver = *reinterpret_cast<void**>(thisPtr + 0x8);

			typedef void(__thiscall *ObserverChannelSendMessagePtr)(void* thisPtr, int arg0, int arg4, int arg8, int messageType, int messageSize, void* data);
			auto ObserverChannelSendMessage = reinterpret_cast<ObserverChannelSendMessagePtr>(0x4474F0);
			ObserverChannelSendMessage(networkObserver, unk7, unk6, 0, 0x1A, packetSize, &packet[0]);
		}
		return true;
	}

	// Serializes extended player-properties data
	void SerializePlayerPropertiesHook(Blam::BitStream* stream, uint8_t* buffer, bool flag)
	{
		// Serialize base data
		typedef void(*SerializePlayerPropertiesPtr)(Blam::BitStream* stream, uint8_t* buffer, bool flag);
		auto SerializePlayerProperties = reinterpret_cast<SerializePlayerPropertiesPtr>(0x4433C0);
		SerializePlayerProperties(stream, buffer, flag);

		// Serialize extended data
		ElDorito::Instance().PlayerPropertiesExtender.SerializeData(stream, buffer + PlayerPropertiesSize);
	}

	// Deserializes extended player-properties data
	bool DeserializePlayerPropertiesHook(Blam::BitStream* stream, uint8_t* buffer, bool flag)
	{
		// Deserialize base data
		typedef bool(*DeserializePlayerPropertiesPtr)(Blam::BitStream* stream, uint8_t* buffer, bool flag);
		auto DeserializePlayerProperties = reinterpret_cast<DeserializePlayerPropertiesPtr>(0x4432E0);
		bool succeeded = DeserializePlayerProperties(stream, buffer, flag);

		// Deserialize extended data
		if (succeeded)
			ElDorito::Instance().PlayerPropertiesExtender.DeserializeData(stream, buffer + PlayerPropertiesSize);
		return succeeded;
	}

	void InitializePacketsHook()
	{
		// Replace the packet table with one we control
		// Only one extra packet type is ever allocated
		auto packetCount = CustomPacketId + 1;
		auto customPacketTable = reinterpret_cast<Blam::Network::PacketTable*>(new Blam::Network::RegisteredPacket[packetCount]);
		ElDorito::Instance().Engine.SetPacketTable(customPacketTable);
		memset(customPacketTable, 0, packetCount * sizeof(Blam::Network::RegisteredPacket));

		// Register the "master" custom packet
		auto name = "eldewrito-custom-packet";
		auto minSize = 0;
		auto maxSize = LONG_MAX;
		customPacketTable->Register(CustomPacketId, name, 0, minSize, maxSize, SerializeCustomPacket, DeserializeCustomPacket, 0, 0);

		// HACK: Patch the packet verification function and change the max packet ID it accepts
		Pointer(0x480022).Write<uint8_t>(CustomPacketId + 1);
	}

	void SerializeCustomPacket(Blam::BitStream *stream, int packetSize, const void *packet)
	{
		auto packetBase = static_cast<const Packets::PacketBase*>(packet);

		// Serialize the packet header
		stream->WriteBlock(sizeof(packetBase->Header) * 8, reinterpret_cast<const uint8_t*>(&packetBase->Header));

		// Serialize the type GUID
		stream->WriteUnsigned(packetBase->TypeGuid, sizeof(packetBase->TypeGuid) * 8);

		// Look up the handler to use and serialize the rest of the packet
		auto type = ElDorito::Instance().Engine.LookUpPacketType(packetBase->TypeGuid);
		if (!type)
			return;

		// Verify the packet size with the handler
		auto minSize = type->Handler->GetMinRawPacketSize();
		auto maxSize = type->Handler->GetMaxRawPacketSize();
		if (packetSize < minSize || packetSize > maxSize)
			return;

		// Serialize the rest of the packet
		type->Handler->SerializeRawPacket(stream, packetSize, packet);
	}

	bool DeserializeCustomPacket(Blam::BitStream *stream, int packetSize, void *packet)
	{
		if (packetSize < static_cast<int>(sizeof(Packets::PacketBase)))
			return false;
		auto packetBase = static_cast<Packets::PacketBase*>(packet);

		// Deserialize the packet header
		stream->ReadBlock(sizeof(packetBase->Header) * 8, reinterpret_cast<uint8_t*>(&packetBase->Header));

		// Deserialize the type GUID and look up the handler to use
		packetBase->TypeGuid = stream->ReadUnsigned<Packets::PacketGuid>(sizeof(Packets::PacketGuid) * 8);
		auto type = ElDorito::Instance().Engine.LookUpPacketType(packetBase->TypeGuid);
		if (!type)
			return false;

		// Verify the packet size with the handler
		auto minSize = type->Handler->GetMinRawPacketSize();
		auto maxSize = type->Handler->GetMaxRawPacketSize();
		if (packetSize < minSize || packetSize > maxSize)
			return false;

		// Deserialize the rest of the packet
		return type->Handler->DeserializeRawPacket(stream, packetSize, packet);
	}

	bool HandleCustomPacket(int id, Blam::Network::ObserverChannel *sender, const void *packet)
	{
		// Only handle the master custom packet
		if (id != CustomPacketId)
			return false;

		// Use the type GUID to look up the handler to use
		auto packetBase = static_cast<const Packets::PacketBase*>(packet);
		auto type = ElDorito::Instance().Engine.LookUpPacketType(packetBase->TypeGuid);
		if (!type)
			return false;
		type->Handler->HandleRawPacket(sender, packet);
		return true;
	}

	__declspec(naked) void HandlePacketHook()
	{
		__asm
		{
			// Check if the channel is closed
			mov edx, dword ptr[ebp + 8]
				cmp dword ptr[edx + 0xA10], 5
				jnz failed

				// Pass the packet to the custom handler
				push dword ptr[ebp + 0x14]
				push edx
				push edi
				call HandleCustomPacket
				test al, al
				jz failed

				// Return gracefully
				push 0x49CB14
				ret

			failed :
			// Execute replaced code
			mov ecx, [esi + 8]
				push 0
				push 0x49CAFF
				ret
		}
	}
}