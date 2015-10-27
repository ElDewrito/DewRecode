#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Player
{
	class PlayerPatchProvider : public PatchProvider
	{
	public:
		PlayerPatchProvider();

		virtual PatchSet GetPatches() override;

		uint64_t GetUid();
		void EnsureValidUid();
	};
}