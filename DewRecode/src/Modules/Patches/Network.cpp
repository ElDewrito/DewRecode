#include "Network.hpp"
#include "../../ElDorito.hpp"

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
			size_t extensionSize = ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.GetTotalSize();
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

	// Applies player properties data including extended properties
	void __fastcall ApplyPlayerPropertiesExtended(uint8_t* thisPtr, void* unused, int playerIndex, uint32_t arg4, uint32_t arg8, uint8_t* properties, uint32_t arg10)
	{
		// Apply the base properties
		typedef void(__thiscall *ApplyPlayerPropertiesPtr)(void* thisPtr, int playerIndex, uint32_t arg4, uint32_t arg8, void* properties, uint32_t arg10);
		auto ApplyPlayerProperties = reinterpret_cast<ApplyPlayerPropertiesPtr>(0x450890);
		ApplyPlayerProperties(thisPtr, playerIndex, arg4, arg8, properties, arg10);

		// Apply the extended properties
		uint8_t* sessionData = thisPtr + 0x10A8 + playerIndex * 0x1648;
		ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.ApplyData(playerIndex, sessionData, properties + PlayerPropertiesSize);
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
		ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.BuildData(playerIndex, &extendedProperties[PlayerPropertiesSize]);

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
		ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.SerializeData(stream, buffer + PlayerPropertiesSize);
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
			ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.DeserializeData(stream, buffer + PlayerPropertiesSize);
		return succeeded;
	}
}

namespace Modules
{
	PatchModuleNetwork::PatchModuleNetwork() : ModuleBase("Patches.Network")
	{
		AddModulePatches({},
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
		});

		// Patch version subs to return version of this DLL, to make people with older DLLs incompatible
		// (we don't do this in a patchset so that other plugins can patch the version if they want, with no conflict errors generated by patchmanager)
		uint32_t verNum = Utils::Version::GetVersionInt();
		Pointer(0x501421).Write<uint32_t>(verNum);
		Pointer(0x50143A).Write<uint32_t>(verNum);
	}
}
