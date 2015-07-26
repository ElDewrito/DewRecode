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

		// TODO: might be better to just inline hook the existing memcpy
		// depending on how performance suffers for debug builds using this
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
}

namespace Modules
{
	ModuleDebug::ModuleDebug() : ModuleBase("Debug")
	{
		VarMemcpySrc = AddVariableInt("MemcpySrc", "memcpy_src", "Allows breakpointing memcpy based on specified source address filter.", eCommandFlagsHidden, 0, MemcpySrcFilterUpdate);
		VarMemcpyDst = AddVariableInt("MemcpyDst", "memcpy_dst", "Allows breakpointing memcpy based on specified destination address filter.", eCommandFlagsHidden, 0, MemcpyDstFilterUpdate);

		Hook hook = Hook("Memcpy", 0xBEF260, Debug_MemcpyHook, HookType::Jmp);
		patches->EnableHook(&hook, true);
	}
}
