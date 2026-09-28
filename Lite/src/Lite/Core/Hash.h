#pragma once

#include "CRC32Hash.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Lite {

	[[nodiscard]] inline uint32_t Hash(const void* data, size_t size)
	{
		return CRC32Hash::Hash(data, size);
	}

	[[nodiscard]] inline uint32_t Hash(std::string_view text)
	{
		return CRC32Hash::Hash(text);
	}

	[[nodiscard]] constexpr uint64_t HashCombine(uint64_t seed, uint64_t value)
	{
		return seed ^ (value + 0x9E3779B97F4A7C15ull + (seed << 6) + (seed >> 2));
	}

}
