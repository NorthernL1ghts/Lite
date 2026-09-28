#pragma once

#include "Base.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Lite {

	class LITE_API CRC32Hash
	{
	public:
		void Reset();
		void Update(const void* data, size_t size);
		void Update(std::string_view text);

		[[nodiscard]] uint32_t Digest() const;

		[[nodiscard]] static uint32_t Hash(const void* data, size_t size);
		[[nodiscard]] static uint32_t Hash(std::string_view text);

	private:
		uint32_t m_Crc = 0xFFFFFFFFu;
	};

}
