#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Scoreboard
{
	class ScoreboardPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}