#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace Graphics
{
	class GraphicsPatchProvider : public PatchProvider
	{
		PatchSet* PatchSSAO;

	public:
		GraphicsPatchProvider();
		virtual PatchSet GetPatches() override;

		bool SetSSAOEnabled(bool isEnabled);
		bool GetSSAOEnabled();
	};
}