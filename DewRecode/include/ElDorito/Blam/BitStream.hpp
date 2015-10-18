#pragma once

#include "../Utils/Bits.hpp"

#include <cstdint>

namespace Blam
{
	// Interface for the bitstream objects the engine uses
	class BitStream
	{
	public:
		// TODO: Add a constructor

		bool ReadBool()
		{
			return ReadBit();
		}

		template<class T>
		T ReadUnsigned(int bits)
		{
			return static_cast<T>(ReadBits(bits));
		}

		template<class T>
		T ReadUnsigned(T minValue, T maxValue)
		{
			return static_cast<T>(ReadUnsigned<T>(Utils::Bits::CountBits(maxValue - minValue)));
		}

		void WriteBool(bool b)
		{
			WriteUnsigned(b, 1);
		}

		template<class T>
		void WriteUnsigned(T val, int bits)
		{
			WriteBits(static_cast<uint64_t>(val), bits);
		}

		template<class T>
		void WriteUnsigned(T val, T minValue, T maxValue)
		{
			WriteUnsigned(val - minValue, Utils::Bits::CountBits(maxValue - minValue));
		}

		void ReadBlock(size_t bits, uint8_t *out)
		{
			typedef void(__thiscall* ReadBlockPtr)(BitStream *thisPtr, uint8_t *out, size_t bits);
			ReadBlockPtr ReadBlock = reinterpret_cast<ReadBlockPtr>(0x558740);
			ReadBlock(this, out, bits);
		}

		void WriteBlock(size_t bits, const uint8_t *data)
		{
			typedef void(__thiscall* WriteBlockPtr)(BitStream *thisPtr, const uint8_t *data, size_t bits, int unkn);
			WriteBlockPtr WriteBlock = reinterpret_cast<WriteBlockPtr>(0x55A000);
			WriteBlock(this, data, bits, 0);
		}

		// Serializes a string to the BitStream.
		template<class CharType, size_t MaxSize>
		void WriteString(const CharType(&str)[MaxSize])
		{
			// Compute length
			size_t length = 0;
			while (length < MaxSize - 1 && str[length])
				length++;

			// Write length
			WriteUnsigned<uint64_t>(length, 0U, MaxSize - 1);

			// Write string
			WriteBlock(length * sizeof(CharType) * 8, reinterpret_cast<const uint8_t*>(&str));
		}

		// Deserializes a string from the BitStream.
		// Returns false if the string is invalid.
		template<class CharType, size_t MaxSize>
		bool ReadString(CharType(&str)[MaxSize])
		{
			// Length
			auto length = ReadUnsigned<size_t>(0U, MaxSize - 1);
			if (length >= MaxSize)
				return false;

			// String
			memset(&str, 0, MaxSize * sizeof(CharType));
			ReadBlock(length * sizeof(CharType) * 8, reinterpret_cast<uint8_t*>(&str));
			return true;
		}

	private:
		uint8_t *start;      // 0x00
		uint8_t *end;        // 0x04
		uint32_t size;       // 0x08
		uint32_t unkC;       // 0x0C
		uint32_t unk10;      // 0x10 - mode?
		void *unk14;         // 0x14
		uint32_t unk18;      // 0x18
		uint32_t position;   // 0x1C
		uint64_t window;     // 0x20
		int windowBitsUsed;  // 0x28
		uint8_t *currentPtr; // 0x2C
		uint32_t unk30;      // 0x30
		uint8_t unk34[100];  // 0x34 - debug info? the constructor doesn't initialize this
		uint32_t unk98;      // 0x98
		uint32_t unk9C;      // 0x9C

		int GetAvailableBits() const { return sizeof(window) * 8 - windowBitsUsed; }
		bool BitsAvailable(int bits) const { return bits <= GetAvailableBits(); }

		bool ReadBit()
		{
			typedef bool(__thiscall* ReadBitPtr)(BitStream *thisPtr);
			ReadBitPtr ReadBit = reinterpret_cast<ReadBitPtr>(0x558570);
			return ReadBit(this);
		}

		uint64_t ReadBits(int bits)
		{
			if (bits <= 32)
			{
				typedef uint32_t(__thiscall* ReadBits32Ptr)(BitStream *thisPtr, int bits);
				ReadBits32Ptr ReadBits32 = reinterpret_cast<ReadBits32Ptr>(0x5589A0);
				return ReadBits32(this, bits);
			}

			// To read more than 32 bits at a time, this function has to be called instead
			typedef uint64_t(__thiscall* ReadBits64Ptr)(BitStream *thisPtr, int bits);
			ReadBits64Ptr ReadBits64 = reinterpret_cast<ReadBits64Ptr>(0x559160);
			return ReadBits64(this, bits);
		}
		
		void WriteBits(uint64_t val, int bits)
		{
			if (BitsAvailable(bits))
			{
				position += bits;
				windowBitsUsed += bits;
				window <<= bits;
				window |= val & (0xFFFFFFFFFFFFFFFF >> (64 - bits));
			}
			else
			{
				typedef void(__thiscall* WriteBitsPtr)(BitStream *thisPtr, uint64_t val, int bits);
				WriteBitsPtr WriteBits = reinterpret_cast<WriteBitsPtr>(0x559EB0);
				return WriteBits(this, val, bits);
			}
		}
	};
}