#pragma once
#include <vector>

namespace Utils
{
	namespace Misc
	{
		template<class T>
		std::vector<unsigned char> ConvertToVector(T val)
		{
			std::vector<unsigned char> returnVal;
			returnVal.resize(sizeof(T));

			memcpy(returnVal.data(), &val, sizeof(T));
			return returnVal;
		}
	}
}