#include <Lite/Core/Hash/CRC32Hash.h>

#include <array>

namespace Lite {

	namespace {

		constexpr uint32_t GenerateCrc32Entry(uint32_t value)
		{
			for (uint32_t bit = 0; bit < 8; ++bit)
			{
				value = (value & 1u)
					? (value >> 1u) ^ 0xEDB88320u
					: value >> 1u;
			}

			return value;
		}

		constexpr auto GenerateCrc32Table()
		{
			std::array<uint32_t, 256> table {};

			for (std::size_t index = 0; index < table.size(); ++index)
				table[index] = GenerateCrc32Entry(static_cast<uint32_t>(index));

			return table;
		}

		inline constexpr auto Crc32Table = GenerateCrc32Table();

		static_assert(Crc32Table[1] == 0x77073096u);
		static_assert(Crc32Table[255] == 0x2D02EF8Du);

	}

	void CRC32Hash::Reset()
	{
		m_Crc = 0xFFFFFFFFu;
	}

	void CRC32Hash::Update(const void* data, size_t size)
	{
		if (!data || size == 0)
			return;

		const auto* bytes = static_cast<const uint8_t*>(data);
		uint32_t crc = m_Crc;
		for (size_t index = 0; index < size; ++index)
			crc = (crc >> 8) ^ Crc32Table[(crc ^ bytes[index]) & 0xFFu];

		m_Crc = crc;
	}

	void CRC32Hash::Update(std::string_view text)
	{
		Update(text.data(), text.size());
	}

	uint32_t CRC32Hash::Digest() const
	{
		return ~m_Crc;
	}

	uint32_t CRC32Hash::Hash(const void* data, size_t size)
	{
		CRC32Hash hasher;
		hasher.Update(data, size);
		return hasher.Digest();
	}

	uint32_t CRC32Hash::Hash(std::string_view text)
	{
		return Hash(text.data(), text.size());
	}

}
