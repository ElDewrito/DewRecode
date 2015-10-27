#pragma once
#include <ElDorito/PatchProvider.hpp>
#include <ElDorito/InputContext.hpp>

namespace Input
{
	typedef std::function<void()> DefaultInputHandler;

	class InputPatchProvider : public PatchProvider
	{
	public:
		virtual PatchSet GetPatches() override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		void PushContext(std::shared_ptr<InputContext> context);
		void RegisterDefaultInputHandler(DefaultInputHandler func);

		void KeyboardUpdated();

		void SetControllerIndex(int idx);
	};
}