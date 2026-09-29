#pragma once

#include <Lite/Math/Math.h>

#include <istream>
#include <string>
#include <string_view>

namespace Lite {

	inline std::string Trim(std::string_view value)
	{
		size_t begin = 0;
		while (begin < value.size() && (value[begin] == ' ' || value[begin] == '\t' || value[begin] == '\r'))
			++begin;

		size_t end = value.size();
		while (end > begin && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r'))
			--end;

		return std::string(value.substr(begin, end - begin));
	}

	inline void ReadVec3(std::istream& stream, Vec3& value)
	{
		stream >> value.x >> value.y >> value.z;
	}

	inline void ReadVec4(std::istream& stream, Vec4& value)
	{
		stream >> value.x >> value.y >> value.z >> value.w;
	}

}
