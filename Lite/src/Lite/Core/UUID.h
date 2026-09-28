#pragma once

#include <Lite/Core/Base.h>

#include <array>
#include <cstdint>
#include <functional>
#include <string>

namespace Lite {

	class LITE_API UUID
	{
	public:
		UUID();
		static UUID Null();

		bool IsNull() const;
		std::string ToString() const;

		const std::array<uint8_t, 16>& Bytes() const { return m_Bytes; }

		bool operator==(const UUID& other) const { return m_Bytes == other.m_Bytes; }
		bool operator!=(const UUID& other) const { return !(*this == other); }
		bool operator<(const UUID& other) const { return m_Bytes < other.m_Bytes; }

	private:
		explicit UUID(std::array<uint8_t, 16> bytes);

		std::array<uint8_t, 16> m_Bytes {};
	};

}

template<>
struct std::hash<Lite::UUID>
{
	size_t operator()(const Lite::UUID& id) const noexcept
	{
		const std::array<uint8_t, 16>& bytes = id.Bytes();
		uint64_t high = 0;
		uint64_t low = 0;
		for (int index = 0; index < 8; ++index)
		{
			high = (high << 8) | bytes[static_cast<size_t>(index)];
			low = (low << 8) | bytes[static_cast<size_t>(index + 8)];
		}

		return std::hash<uint64_t>{}(high) ^ (std::hash<uint64_t>{}(low) << 1);
	}
};
