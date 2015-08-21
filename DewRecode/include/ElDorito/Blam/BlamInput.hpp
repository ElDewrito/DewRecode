#pragma once

#include <cstdint>

namespace Blam
{
	namespace Input
	{
		enum ButtonCodes : uint32_t
		{
			eButtonCodesA = 0,
			eButtonCodesB,
			eButtonCodesX,
			eButtonCodesY,
			eButtonCodesRB,
			eButtonCodesLB,
			eButtonCodesLT,
			eButtonCodesRT,
			eButtonCodesDpadUp,
			eButtonCodesDpadDown,
			eButtonCodesDpadLeft,
			eButtonCodesDpadRight,
			eButtonCodesStart,
			eButtonCodesBack,
			eButtonCodesLS,
			eButtonCodesRS,
			eButtonCodesEmulatedInput,
			eButtonCodesUnk1,
			eButtonCodesLeft, // analog/arrow left
			eButtonCodesRight, // analog/arrow right
			eButtonCodesUp, // analog/arrow up
			eButtonCodesDown, // analog/arrow down
			eButtonCodesLSHorizontal = 0x1A,
			eButtonCodesLSVertical,
			eButtonCodesRSHorizontal,
			eButtonCodesRSVertical,
		};

		enum KeyCodes : int16_t
		{
			eKeyCodesEscape,
			eKeyCodesF1,
			eKeyCodesF2,
			eKeyCodesF3,
			eKeyCodesF4,
			eKeyCodesF5,
			eKeyCodesF6,
			eKeyCodesF7,
			eKeyCodesF8,
			eKeyCodesF9,
			eKeyCodesF10,
			eKeyCodesF11,
			eKeyCodesF12,
			eKeyCodesPrintScreen,
			eKeyCodesF14,
			eKeyCodesF15,
			eKeyCodesTilde, // VK_OEM_3
			eKeyCodes1,
			eKeyCodes2,
			eKeyCodes3,
			eKeyCodes4,
			eKeyCodes5,
			eKeyCodes6,
			eKeyCodes7,
			eKeyCodes8,
			eKeyCodes9,
			eKeyCodes0,
			eKeyCodesMinus,
			eKeyCodesPlus,
			eKeyCodesBack,
			eKeyCodesTab,
			eKeyCodesQ,
			eKeyCodesW,
			eKeyCodesE,
			eKeyCodesR,
			eKeyCodesT,
			eKeyCodesY,
			eKeyCodesU,
			eKeyCodesI,
			eKeyCodesO,
			eKeyCodesP,
			eKeyCodesLBracket, // VK_OEM_4
			eKeyCodesRBracket, // VK_OEM_6
			eKeyCodesPipe, // VK_OEM_5
			eKeyCodesCapital,
			eKeyCodesA,
			eKeyCodesS,
			eKeyCodesD,
			eKeyCodesF,
			eKeyCodesG,
			eKeyCodesH,
			eKeyCodesJ,
			eKeyCodesK,
			eKeyCodesL,
			eKeyCodesColon, // VK_OEM_1
			eKeyCodesQuote, // VK_OEM_7
			eKeyCodesEnter,
			eKeyCodesLShift,
			eKeyCodesZ,
			eKeyCodesX,
			eKeyCodesC,
			eKeyCodesV,
			eKeyCodesB,
			eKeyCodesN,
			eKeyCodesM,
			eKeyCodesComma,
			eKeyCodesPeriod,
			eKeyCodesQuestion, // VK_OEM_2
			eKeyCodesRShift,
			eKeyCodesLControl,
			eKeyCodesUnused46, // Left Windows key, but will always fail
			eKeyCodesLAlt,
			eKeyCodesSpace,
			eKeyCodesRAlt,
			eKeyCodesUnused4A, // Right Windows key, but will always fail
			eKeyCodesApps,
			eKeyCodesRcontrol,
			eKeyCodesUp,
			eKeyCodesDown,
			eKeyCodesLeft,
			eKeyCodesRight,
			eKeyCodesInsert,
			eKeyCodesHome,
			eKeyCodesPageUp,
			eKeyCodesDelete,
			eKeyCodesEnd,
			eKeyCodesPageDown,
			eKeyCodesNumLock,
			eKeyCodesDivide,
			eKeyCodesMultiply,
			eKeyCodesNumpad0,
			eKeyCodesNumpad1,
			eKeyCodesNumpad2,
			eKeyCodesNumpad3,
			eKeyCodesNumpad4,
			eKeyCodesNumpad5,
			eKeyCodesNumpad6,
			eKeyCodesNumpad7,
			eKeyCodesNumpad8,
			eKeyCodesNumpad9,
			eKeyCodesSubtract,
			eKeyCodesAdd,
			eKeyCodesNumpadEnter,
			eKeyCodesDecimal,
			eKeyCodesUnused68,
			eKeyCodesShift,
			eKeyCodesCtrl,
			eKeyCodesUnused6B, // Windows key, but will always fail
			eKeyCodesAlt,

			eKeyCodes_Count // Not actually a key, just represents the number
			// of keys that the game scans
		};

		enum InputType : uint32_t
		{
			eInputTypeUi,      // ABXY, mouse clicks, etc.
			eInputTypeGame,    // All in-game actions (including camera)
			//   Disabled when the pause menu is open
			eInputTypeSpecial, // Escape, tab, menu navigation
		};

		enum KeyEventModifiers : uint8_t
		{
			eKeyEventModifiersShift = 1 << 0,
			eKeyEventModifiersCtrl = 1 << 1,
			eKeyEventModifiersalt = 1 << 2,
		};

		enum KeyEventType : uint32_t
		{
			eKeyEventTypeDown, // A key was pressed.
			eKeyEventTypeUp,   // A key was released.
			eKeyEventTypeChar  // A character was typed.
		};

		struct KeyEvent
		{
			KeyEventModifiers Modifiers; // Bitfield of modifier keys that are down
			KeyEventType Type;           // Event type
			KeyCodes Code;               // The key code, or -1 if unavailable
			char16_t Char;               // For eKeyEventTypeChar events, the character that was typed, or -1 if unavailable
			bool PreviousState;          // If true, the key was down before this event happened
		};
		static_assert(sizeof(KeyEvent) == 0x10, "Invalid KeyEvent size");
	}
}