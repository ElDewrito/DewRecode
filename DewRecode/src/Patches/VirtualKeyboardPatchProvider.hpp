#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace VirtualKeyboard
{
	class VirtualKeyboardPatchProvider : public IPatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
	};
}