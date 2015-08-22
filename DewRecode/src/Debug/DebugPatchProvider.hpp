#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Debug
{
	class DebugPatchProvider : public IPatchProvider
	{
	public:
		Hook* NetworkLogHook;
		Hook* SSLLogHook;
		Hook* UILogHook;
		Hook* Game1LogHook;
		PatchSet* Game2LogHook;

		DebugPatchProvider();
		virtual PatchSet GetPatches() override;
	};
}