#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace Input
{
	typedef std::function<void()> DefaultInputHandler;

	// Base class for objects which override game input.
	// When an input context is active, game and special input will be
	// blocked.
	class InputContext
	{
	public:
		virtual ~InputContext() { }

		// Called when the input context is made active.
		virtual void InputActivated() = 0;

		// Called when the input context is deactivated.
		virtual void InputDeactivated() = 0;

		// Called on the active context each time the game processes input,
		// overriding the default input processing routine. If this returns
		// false, the input context will be deactivated.
		//
		// The difference between this and UiInputTick() is that this
		// cannot call Blam::Input::ReadKeyEvent() because Windows messages
		// won't have been pumped yet. However, this happens earlier on in
		// the engine update cycle.
		virtual bool GameInputTick() = 0;

		// Called on the active context each time Windows messages are
		// pumped. If this returns false, the input context will be
		// deactivated.
		virtual bool UiInputTick() = 0;
	};

	class InputPatchProvider : public IPatchProvider
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