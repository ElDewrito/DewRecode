#pragma once

#include <cstdint>
#include <utility>
#include "../../Pointer.hpp"

// Asserts that a tag structure is the correct size.
#define TAG_STRUCT_SIZE_ASSERT(type, size)           \
	static_assert                                    \
	(                                                \
		sizeof(type) == (size),                      \
		"Tag structure " #type " has incorrect size" \
	)

namespace Blam
{
	namespace Tags
	{
		// Base struct for a tag. groupTag is the magic constant identifying
		// the corresponding group tag, e.g. 'bipd'
		template<int groupTag>
		struct Tag
		{
			static const int GroupTag = groupTag;
		};

		// A tag block element, which references an array of data.
		template<class T>
		struct TagBlock
		{
			int Count;
			T* Data;
			int UnusedC;

			// Accesses an element by index.
			T& operator[](int index) const { return Data[index]; }

			// Gets a pointer to the first element in the tag block.
			T* begin() const { return &Data[0]; }

			// Gets a pointer past the last element in the tag block.
			T* end() const { return &Data[Count]; }

			// Determines whether the tag block is not null.
			explicit operator bool() const { return Data != nullptr; }
		};
		TAG_STRUCT_SIZE_ASSERT(TagBlock<char>, 0xC);

		// A tag reference element, which references another tag.
		struct TagReference
		{
			int GroupTag;
			int Unused4;
			int Unused8;
			uint32_t Index;

			// Determines whether the tag reference is not null.
			explicit operator bool() const { return Index == 0xFFFFFFFF; }
		};
		TAG_STRUCT_SIZE_ASSERT(TagReference, 0x10);

		// References a raw data buffer.
		template<class T>
		struct DataReference
		{
			uint32_t Size;
			int Unused4;
			int Unused8;
			T* Data;
			int Unused10;

			// Determines whether the data reference is not null.
			explicit operator bool() const { return Data != nullptr; }
		};
		TAG_STRUCT_SIZE_ASSERT(DataReference<char>, 0x14);

		// Statically determines whether a type inherits from Tag<>.
		template<class T>
		struct IsTagType
		{
			typedef char yes[1];
			typedef char no[2];

			template<int U>
			static yes& test(Tag<U> const &);

			static no& test(...);

			static const bool Value = sizeof(test(std::declval<T>())) == sizeof(yes);
		};

		// Gets a tag by index.
		template<class TagType>
		inline TagType* GetTag(uint32_t index)
		{
			static_assert(IsTagType<TagType>::Value, "Cannot call GetTag() on a non-tag type");
			if (index != 0xFFFFFFFF)
			{
				typedef TagType* (*GetTagAddressPtr)(int groupTag, uint32_t index);
				auto GetTagAddress = reinterpret_cast<GetTagAddressPtr>(0x503370);
				return GetTagAddress(TagType::GroupTag, index);
			}
			return nullptr;
		}

		inline void* GetTagAddress(uint32_t index)
		{
			typedef void* (*GetTagAddressPtr)(int groupTag, uint32_t index);
			auto GetTagAddress = reinterpret_cast<GetTagAddressPtr>(0x503370);
			return GetTagAddress(0, index);
		}

		inline void* GetTagHdrAddress(uint32_t index)
		{
			auto* addr = Pointer(0x22AAFFC).Read<BYTE*>();
			int v2 = *(addr + (index * 4));
			if (v2 == -1)
				return 0;
			auto* addr2 = Pointer(0x22AAFF8).Read<BYTE*>();
			return (void*)*(addr2 + (v2 * 4));
		}

		inline uint32_t GetTagClass(uint32_t index)
		{
			typedef uint32_t(*GetTagClassPtr)(uint32_t index);
			auto GetTagClass = reinterpret_cast<GetTagClassPtr>(0x5033A0);
			return GetTagClass(index);
		}

		inline std::vector<int> GetTagsOfClass(uint32_t className)
		{
			auto numTags = Pointer(0x22AB008).Read<uint32_t>();
			std::vector<int> retVal;
			for (uint32_t i = 0; i < numTags; i++)
			{
				void* addr = GetTagAddress(i);
				if (!addr)
					continue;

				auto tagClass = GetTagClass(i);
				if (tagClass != className)
					continue;
				retVal.push_back(i);
			}
			return retVal;
		}
	}
}