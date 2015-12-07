#include "GraphicsPatchProvider.hpp"
#include "../ElDorito.hpp"
#include <ElDorito/Blam/Tags/Tags.hpp>

namespace Graphics
{
	GraphicsPatchProvider::GraphicsPatchProvider()
	{
		auto& patches = ElDorito::Instance().PatchManager;

		PatchSSAO = patches.AddPatchSet("PatchSSAO",
		{
			Patch("SSAOFlagCheck", 0xA62D98, 0x90, 0x15),
			Patch("SSAOArg2Patch", 0xA62DC7, 0x90, 0xF),
			Patch("SSAOArg1Patch", 0xA62DEA, 0x90, 0xF),
			Patch("SSAOArg3Patch", 0xA62E0D, 0x90, 0xF),
		},
		{});
	}

	PatchSet GraphicsPatchProvider::GetPatches()
	{
		PatchSet patches("GraphicsPatches",
		{
		},
		{
		});

		return patches;
	}

	bool GraphicsPatchProvider::SetSSAOEnabled(bool isEnabled)
	{
		ElDorito::Instance().PatchManager.EnablePatchSet(PatchSSAO, isEnabled);
		return isEnabled;
	}

	bool GraphicsPatchProvider::GetSSAOEnabled()
	{
		return PatchSSAO->Enabled;
	}
}