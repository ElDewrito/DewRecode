#include "DebugPatchProvider.hpp"
#include "../ElDorito.hpp"
#include <ElDorito/Blam/BlamNetwork.hpp>

namespace
{
	int networkLogHook(char* format, ...);
	void __cdecl sslLogHook(char a1, int a2, void* a3, void* a4, char a5);
	void __cdecl uiLogHook(char a1, int a2, void* a3, void* a4, char a5);
	void dbglog(const char* module, char* format, ...);
	void debuglog_float(char* name, float value);
	void debuglog_int(char* name, int value);
	void debuglog_string(char* name, char* value);

	bool __fastcall packetRecvHook(void* thisPtr, int unused, Blam::BitStream* stream, int* packetIdOut, int* packetSizeOut);
	void __fastcall packetSendHook(void* thisPtr, int unused, Blam::BitStream* stream, int packetId, int packetSize);

	void* Debug_MemcpyHook(void* dst, void* src, uint32_t len);
	void* Debug_MemsetHook(void* dst, int val, uint32_t len);
	void ExceptionHook(char* msg);
}

namespace Debug
{
	DebugPatchProvider::DebugPatchProvider()
	{
		auto& patches = ElDorito::Instance().PatchManager;

		NetworkLogHook = patches.AddHook("NetworkLog", 0xD858D0, networkLogHook, HookType::Jmp);
		SSLLogHook = patches.AddHook("SSLLog", 0xE7FE10, sslLogHook, HookType::Jmp);
		UILogHook = patches.AddHook("UILog", 0xEED600, uiLogHook, HookType::Jmp);
		Game1LogHook = patches.AddHook("Game1Log", 0x506FB0, dbglog, HookType::Jmp);
		Game2LogHook = patches.AddPatchSet("Game2Log", {}, {
			Hook("DebugLogFloat", 0x6189F0, debuglog_float, HookType::Jmp),
			Hook("DebugLogIntHook", 0x618A10, debuglog_int, HookType::Jmp),
			Hook("DebugLogStringHook", 0x618A30, debuglog_string, HookType::Jmp)
		});

		PacketsHook = patches.AddPatchSet("PacketsLog", {}, {
			Hook("PacketReceiveHook", 0x47FF88, packetRecvHook, HookType::Call),
			Hook("PacketSendHook", 0x4800A4, packetSendHook, HookType::Call),
		});
	}

	PatchSet DebugPatchProvider::GetPatches()
	{
		PatchSet patches("DebugPatches",
		{
			Patch("CrashLog1", 0x51C158, { 0x8D, 0x85, 0x00, 0xFC, 0xFF, 0xFF, 0x50 }),
			Patch("CrashLog2", 0x51C165, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 })
		},
		{
			Hook("Memcpy", 0xBEF260, Debug_MemcpyHook, HookType::Jmp),
			Hook("Memset", 0xBEE2E0, Debug_MemsetHook, HookType::Jmp),
			Hook("CrashLogHook", 0x51C15F, ExceptionHook, HookType::Call)
		});

		return patches;
	}
}

namespace
{
	void dbglog(const char* module, char* format, ...)
	{
		char* backupFormat = "";
		if (!format)
			format = backupFormat;

		if (module != 0 && strcmp(module, "game_tick") == 0)
			return; // filter game_tick spam

		va_list ap;
		va_start(ap, format);

		char buff[4096];
		vsprintf_s(buff, 4096, format, ap);
		va_end(ap);

		ElDorito::Instance().Logger.Log(LogSeverity::Info, module, "%s", buff);
	}

	void debuglog_string(char* name, char* value)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Info, "Debug", "%s: %s", name, value);
	}

	void debuglog_int(char* name, int value)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Info, "Debug", "%s: %d", name, value);
	}

	void debuglog_float(char* name, float value)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Info, "Debug", "%s: %f", name, value);
	}

	int networkLogHook(char* format, ...)
	{
		auto& dorito = ElDorito::Instance();

		// fix strings using broken printf statements
		std::string formatStr(format);
		dorito.Utils.ReplaceString(formatStr, "%LX", "%llX");

		char dstBuf[4096];
		memset(dstBuf, 0, 4096);

		va_list args;
		va_start(args, format);
		vsnprintf_s(dstBuf, 4096, 4096, formatStr.c_str(), args);
		va_end(args);

		dorito.Logger.Log(LogSeverity::Info, "Network", "%s", dstBuf);

		return 1;
	}

	void __cdecl sslLogHook(char a1, int a2, void* a3, void* a4, char a5)
	{
		char* logData1 = (*(char**)(a3));
		char* logData2 = (*(char**)((DWORD_PTR)a3 + 0x8));
		if (logData1 == 0)
			logData1 = "";
		else
			logData1 += 0xC;

		if (logData2 == 0)
			logData2 = "";
		else
			logData2 += 0xC;

		ElDorito::Instance().Logger.Log(LogSeverity::Info, (const char*)logData1, (char*)logData2);
		return;
	}

	void __cdecl uiLogHook(char a1, int a2, void* a3, void* a4, char a5)
	{
		char* logData1 = (*(char**)(a3));
		if (logData1 == 0)
			logData1 = "";
		else
			logData1 += 0xC;

		ElDorito::Instance().Logger.Log(LogSeverity::Info, "UiLog", (char*)logData1);
		return;
	}

	void* Debug_MemcpyHook(void* dst, void* src, uint32_t len)
	{
		auto srcFilter = ElDorito::Instance().DebugCommands->VarMemcpySrc->ValueInt;
		auto dstFilter = ElDorito::Instance().DebugCommands->VarMemcpyDst->ValueInt;
		uint32_t srcAddress = (uint32_t)src;
		uint32_t dstAddress = (uint32_t)dst;

		if (srcFilter >= srcAddress && srcFilter < srcAddress + len)
		{
			int breakHere = 0;
		}

		if (dstFilter >= dstAddress && dstFilter < dstAddress + len)
		{
			int breakHere = 0;
		}

		return memcpy(dst, src, len);
	}

	void* Debug_MemsetHook(void* dst, int val, uint32_t len)
	{
		auto dstFilter = ElDorito::Instance().DebugCommands->VarMemsetDst->ValueInt;
		uint32_t dstAddress = (uint32_t)dst;

		if (dstFilter >= dstAddress && dstFilter < dstAddress + len)
		{
			int breakHere = 0;
		}

		return memset(dst, val, len);
	}

	void ExceptionHook(char* msg)
	{
		auto& dorito = ElDorito::Instance();

		auto* except = *Pointer(0x238E880).Read<EXCEPTION_RECORD**>();

		dorito.Logger.Log(LogSeverity::Fatal, "GameCrash", std::string(msg));
		dorito.Logger.Log(LogSeverity::Fatal, "GameCrash", "Code: 0x%x, flags: 0x%x, record: 0x%x, addr: 0x%x, numparams: 0x%x",
			except->ExceptionCode, except->ExceptionFlags, except->ExceptionRecord, except->ExceptionAddress, except->NumberParameters);

		std::exit(0);
	}

	bool __fastcall packetRecvHook(void *thisPtr, int unused, Blam::BitStream *stream, int *packetIdOut, int *packetSizeOut)
	{
		typedef bool(__thiscall *DeserializePacketInfoPtr)(void *thisPtr, Blam::BitStream *stream, int *packetIdOut, int *packetSizeOut);
		auto DeserializePacketInfo = reinterpret_cast<DeserializePacketInfoPtr>(0x47FFE0);
		if (!DeserializePacketInfo(thisPtr, stream, packetIdOut, packetSizeOut))
			return false;

		auto& dorito = ElDorito::Instance();

		auto packetTable = dorito.Engine.GetPacketTable();
		if (!packetTable)
			return true;
		auto packet = &packetTable->Packets[*packetIdOut];
		dorito.Logger.Log(LogSeverity::Info, "Packets", "RECV %s (size=0x%X)", packet->Name, *packetSizeOut);
		return true;
	}

	void __fastcall packetSendHook(void *thisPtr, int unused, Blam::BitStream *stream, int packetId, int packetSize)
	{
		typedef bool(__thiscall *SerializePacketInfoPtr)(void *thisPtr, Blam::BitStream *stream, int packetId, int packetSize);
		auto SerializePacketInfo = reinterpret_cast<SerializePacketInfoPtr>(0x4800D0);
		SerializePacketInfo(thisPtr, stream, packetId, packetSize);

		auto& dorito = ElDorito::Instance();

		auto packetTable = dorito.Engine.GetPacketTable();
		if (!packetTable)
			return;
		auto packet = &packetTable->Packets[packetId];
		dorito.Logger.Log(LogSeverity::Info, "Packets", "SEND %s (size=0x%X)", packet->Name, packetSize);
	}
}