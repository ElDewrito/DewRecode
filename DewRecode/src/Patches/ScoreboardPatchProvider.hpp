#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Scoreboard
{
	class ScoreboardPatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}