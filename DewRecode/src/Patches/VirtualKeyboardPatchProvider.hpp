#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace VirtualKeyboard
{
	class VirtualKeyboardPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}