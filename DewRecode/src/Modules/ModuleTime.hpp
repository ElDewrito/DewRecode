#pragma once
#include <ElDorito/ModuleBase.hpp>

namespace Modules
{
	class ModuleTime : public ModuleBase
	{
	public:
		Command* VarSpeed;

		// TODO: experimental fps with buggy havok physics - give hkWorld initialization a second chance in hopes of fixing it
		//Command* VarFps;

		ModuleTime();
	};
}