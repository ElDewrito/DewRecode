#include "ModuleDebug.hpp"
#include <sstream>
#include "../ElDorito.hpp"

namespace
{
	void* Debug_MemcpyHook(void* dst, void* src, uint32_t len)
	{
		auto srcFilter = ElDorito::Instance().Modules.Debug.VarMemcpySrc->ValueInt;
		auto dstFilter = ElDorito::Instance().Modules.Debug.VarMemcpyDst->ValueInt;
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

	bool MemcpySrcFilterUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto src = ElDorito::Instance().Modules.Debug.VarMemcpySrc->ValueInt;
	
		std::stringstream ss;
		ss << "Memcpy source address filter set to " << src;
		returnInfo = ss.str();

		return true;
	}

	bool MemcpyDstFilterUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto dst = ElDorito::Instance().Modules.Debug.VarMemcpyDst->ValueInt;

		std::stringstream ss;
		ss << "Memcpy destination address filter set to " << dst;
		returnInfo = ss.str();

		return true;
	}

	void* Debug_MemsetHook(void* dst, int val, uint32_t len)
	{
		auto dstFilter = ElDorito::Instance().Modules.Debug.VarMemsetDst->ValueInt;
		uint32_t dstAddress = (uint32_t)dst;

		if (dstFilter >= dstAddress && dstFilter < dstAddress + len)
		{
			int breakHere = 0;
		}

		return memset(dst, val, len);
	}

	bool MemsetDstFilterUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto dst = ElDorito::Instance().Modules.Debug.VarMemsetDst->ValueInt;

		std::stringstream ss;
		ss << "Memset destination address filter set to " << dst;
		returnInfo = ss.str();

		return true;
	}

	void ExceptionHook(char* msg)
	{
		ElDorito::Instance().Logger.Log(LogSeverity::Fatal, "GameCrash", std::string(msg));
		std::exit(0);
	}
}

namespace Modules
{
	ModuleDebug::ModuleDebug() : ModuleBase("Debug")
	{
		VarMemcpySrc = AddVariableInt("MemcpySrc", "memcpy_src", "Allows breakpointing memcpy based on specified source address filter.", eCommandFlagsHidden, 0, MemcpySrcFilterUpdate);
		VarMemcpyDst = AddVariableInt("MemcpyDst", "memcpy_dst", "Allows breakpointing memcpy based on specified destination address filter.", eCommandFlagsHidden, 0, MemcpyDstFilterUpdate);
		VarMemsetDst = AddVariableInt("MemsetDst", "memset_dst", "Allows breakpointing memset based on specified destination address filter.", eCommandFlagsHidden, 0, MemsetDstFilterUpdate);

		AddModulePatches(
		{
			Patch("CrashLog1", 0x51C158, { 0x8D, 0x85, 0x00, 0xFC, 0xFF, 0xFF, 0x50 }),
			Patch("CrashLog2", 0x51C165, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 })
		},
		{
			Hook("Memcpy", 0xBEF260, Debug_MemcpyHook, HookType::Jmp),
			Hook("Memset", 0xBEE2E0, Debug_MemsetHook, HookType::Jmp),
			Hook("CrashLogHook", 0x51C15F, ExceptionHook, HookType::Call)
		});
	}
}
