#pragma once

#include <cstdint>

namespace Blam
{
	enum class ButtonCode: uint32_t
	{
		A = 0,
		B,
		X,
		Y,
		RB,
		LB,
		LT,
		RT,
		DpadUp,
		DpadDown,
		DpadLeft,
		DpadRight,
		Start,
		Back,
		LS,
		RS,
		EmulatedInput,
		Unk1,
		Left, // analog/arrow left
		Right, // analog/arrow right
		Up, // analog/arrow up
		Down, // analog/arrow down
		LSHorizontal = 0x1A,
		LSVertical,
		RSHorizontal,
		RSVertical,
	};

	enum class KeyCode: int16_t
	{
		Escape,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		PrintScreen,
		F14,
		F15,
		Tilde, // VK_OEM_3
		Num1,
		Num2,
		Num3,
		Num4,
		Num5,
		Num6,
		Num7,
		Num8,
		Num9,
		Num0,
		Minus,
		Plus,
		Back,
		Tab,
		Q,
		W,
		E,
		R,
		T,
		Y,
		U,
		I,
		O,
		P,
		LBracket, // VK_OEM_4
		RBracket, // VK_OEM_6
		Pipe, // VK_OEM_5
		Capital,
		A,
		S,
		D,
		F,
		G,
		H,
		J,
		K,
		L,
		Colon, // VK_OEM_1
		Quote, // VK_OEM_7
		Enter,
		LShift,
		Z,
		X,
		C,
		V,
		B,
		N,
		M,
		Comma,
		Period,
		Question, // VK_OEM_2
		RShift,
		LControl,
		Unused46, // Left Windows key, but will always fail
		LAlt,
		Space,
		RAlt,
		Unused4A, // Right Windows key, but will always fail
		Apps,
		RControl,
		Up,
		Down,
		Left,
		Right,
		Insert,
		Home,
		PageUp,
		Delete,
		End,
		PageDown,
		NumLock,
		Divide,
		Multiply,
		Numpad0,
		Numpad1,
		Numpad2,
		Numpad3,
		Numpad4,
		Numpad5,
		Numpad6,
		Numpad7,
		Numpad8,
		Numpad9,
		Subtract,
		Add,
		NumpadEnter,
		Decimal,
		Unused68,
		Shift,
		Ctrl,
		Unused6B, // Windows key, but will always fail
		Alt,
	};
	const int16_t NumKeyCodes = static_cast<int>(KeyCode::Alt) + 1;

	enum class InputType: uint32_t
	{
		UI,      // ABXY, mouse clicks, etc.
		Game,    // All in-game actions (including camera)
			        //   Disabled when the pause menu is open
		Special, // Escape, tab, menu navigation
	};

	enum class KeyEventModifiers: uint8_t
	{
		Shift = 1 << 0,
		Ctrl = 1 << 1,
		Alt = 1 << 2,
	};

	enum class KeyEventType: uint32_t
	{
		Down, // A key was pressed.
		Up,   // A key was released.
		Char  // A character was typed.
	};

	struct KeyEvent
	{
		KeyEventModifiers Modifiers; // Bitfield of modifier keys that are down
		KeyEventType Type;           // Event type
		KeyCode Key;                 // The key code, or -1 if unavailable
		char16_t Char;               // For eKeyEventTypeChar events, the character that was typed, or -1 if unavailable
		bool PreviousState;          // If true, the key was down before this event happened
	};
	static_assert(sizeof(KeyEvent) == 0x10, "Invalid KeyEvent size");

	// Gets the number of ticks that a key has been held down for.
	// Will always be nonzero if the key is down.
	inline uint8_t GetKeyTicks(KeyCode key, InputType type)
	{
		typedef uint8_t(*EngineGetKeyTicksPtr)(KeyCode, InputType);
		auto EngineGetKeyTicks = reinterpret_cast<EngineGetKeyTicksPtr>(0x511B60);
		return EngineGetKeyTicks(key, type);
	}

	// Gets the number of milliseconds that a key has been held down for.
	// Will always be nonzero if the key is down.
	inline uint16_t GetKeyMs(KeyCode key, InputType type)
	{
		typedef uint8_t(*EngineGetKeyMsPtr)(KeyCode, InputType);
		auto EngineGetKeyMs = reinterpret_cast<EngineGetKeyMsPtr>(0x511CE0);
		return EngineGetKeyMs(key, type);
	}

	// Reads a raw keyboard input event. Returns false if nothing is
	// available. You should call this in a loop to ensure that you process
	// all available events. NOTE THAT THIS IS ONLY GUARANTEED TO WORK
	// AFTER WINDOWS MESSAGES HAVE BEEN PUMPED IN THE UPDATE CYCLE. ALSO,
	// THIS WILL NOT WORK IF UI INPUT IS DISABLED, REGARDLESS OF THE INPUT
	// TYPE YOU SPECIFY.
	inline bool ReadKeyEvent(KeyEvent* result, InputType type)
	{
		typedef bool(*EngineReadKeyEventPtr)(KeyEvent*, InputType);
		auto EngineReadKeyEvent = reinterpret_cast<EngineReadKeyEventPtr>(0x5118C0);
		return EngineReadKeyEvent(result, type);
	}

	// Blocks or unblocks an input type.
	inline void BlockInput(InputType type, bool block)
	{
		typedef uint8_t(*EngineBlockInputPtr)(InputType, bool);
		auto EngineBlockInput = reinterpret_cast<EngineBlockInputPtr>(0x512530);
		EngineBlockInput(type, block);
	}
}