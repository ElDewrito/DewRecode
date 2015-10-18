#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Player
{
	class PlayerPatchProvider : public IPatchProvider
	{
	public:
		PlayerPatchProvider();

		virtual PatchSet GetPatches() override;

		uint64_t GetUid();
		void EnsureValidUid();
	};
}