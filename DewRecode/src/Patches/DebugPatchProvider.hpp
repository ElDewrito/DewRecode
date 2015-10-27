#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Debug
{
	class DebugPatchProvider : public PatchProvider
	{
	public:
		Hook* NetworkLogHook;
		Hook* SSLLogHook;
		Hook* UILogHook;
		Hook* Game1LogHook;
		PatchSet* Game2LogHook;
		PatchSet* PacketsHook;

		DebugPatchProvider();
		virtual PatchSet GetPatches() override;
	};
}