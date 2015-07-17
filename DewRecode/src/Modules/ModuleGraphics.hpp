#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleGraphics : public ModuleBase
	{
	public:
		Command* VarSaturation;

		// TODO: possibly refactor into single #RRGGBB command or provide such functionality as a separate all-in-one command
		Command* VarRedHue;
		Command* VarGreenHue;
		Command* VarBlueHue;

		Command* VarBloom;
		Command* VarDepthOfField;
		Command* VarLetterbox;

		ModuleGraphics();
	};
}