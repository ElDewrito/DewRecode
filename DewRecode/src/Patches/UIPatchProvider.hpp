#pragma once
#include <ElDorito/PatchProvider.hpp>

namespace UI
{
	class UIPatchProvider : public PatchProvider
	{
	public:
		UIPatchProvider();

		virtual PatchSet GetPatches() override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		void Tick(const std::chrono::duration<double>& deltaTime);
		void ApplyMapNameFixes(void* param);

		bool DialogShow = false;
		unsigned int DialogStringId;
		int DialogArg1;
		int DialogFlags;
		unsigned int DialogParentStringId;
	};
}