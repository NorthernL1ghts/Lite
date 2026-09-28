#include "Texture.h"

#include "Lite/Core/FileSystem.h"
#include "Lite/Core/Logger.h"

namespace {

	uint16_t ReadU16(const std::vector<uint8_t>& bytes, size_t offset)
	{
		return static_cast<uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
	}

	uint32_t ReadU32(const std::vector<uint8_t>& bytes, size_t offset)
	{
		return static_cast<uint32_t>(bytes[offset]
			| (bytes[offset + 1] << 8)
			| (bytes[offset + 2] << 16)
			| (bytes[offset + 3] << 24));
	}

	int32_t ReadI32(const std::vector<uint8_t>& bytes, size_t offset)
	{
		return static_cast<int32_t>(ReadU32(bytes, offset));
	}

	std::vector<uint8_t> ReadFile(const std::filesystem::path& path)
	{
		std::vector<uint8_t> bytes = Lite::FileSystem::ReadBinary(path);
		if (bytes.empty())
			LITE_ERROR("Failed to read texture {}", path.string());

		return bytes;
	}

	bool DecodeBmp(const std::vector<uint8_t>& bytes, uint32_t& width, uint32_t& height, std::vector<uint8_t>& pixels)
	{
		if (bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M')
			return false;

		uint32_t pixelOffset = ReadU32(bytes, 10);
		int32_t signedWidth = ReadI32(bytes, 18);
		int32_t signedHeight = ReadI32(bytes, 22);
		uint16_t planes = ReadU16(bytes, 26);
		uint16_t bitCount = ReadU16(bytes, 28);
		uint32_t compression = ReadU32(bytes, 30);

		if (signedWidth <= 0 || planes != 1 || compression != 0)
			return false;
		if (bitCount != 24 && bitCount != 32)
			return false;

		bool topDown = signedHeight < 0;
		int32_t absoluteHeight = topDown ? -signedHeight : signedHeight;
		if (absoluteHeight <= 0)
			return false;

		width = static_cast<uint32_t>(signedWidth);
		height = static_cast<uint32_t>(absoluteHeight);
		uint32_t channels = bitCount / 8;
		uint32_t rowStride = ((width * channels + 3u) / 4u) * 4u;
		size_t required = static_cast<size_t>(pixelOffset) + static_cast<size_t>(rowStride) * height;
		if (bytes.size() < required)
			return false;

		pixels.assign(static_cast<size_t>(width) * height * 4u, 255u);
		for (uint32_t y = 0; y < height; ++y)
		{
			uint32_t sourceRow = topDown ? y : (height - 1u - y);
			const uint8_t* row = bytes.data() + pixelOffset + static_cast<size_t>(sourceRow) * rowStride;
			for (uint32_t x = 0; x < width; ++x)
			{
				const uint8_t* source = row + static_cast<size_t>(x) * channels;
				uint8_t* dest = pixels.data() + (static_cast<size_t>(y) * width + x) * 4u;
				dest[0] = source[2];
				dest[1] = source[1];
				dest[2] = source[0];
				dest[3] = channels == 4 ? source[3] : 255u;
			}
		}

		return true;
	}

}

namespace Lite {

	bool Texture::LoadFromFile(const std::filesystem::path& path)
	{
		std::vector<uint8_t> bytes = ReadFile(path);
		if (bytes.empty())
			return false;

		if (!DecodeBmp(bytes, m_Width, m_Height, m_Pixels))
		{
			LITE_ERROR("Texture {} is not a 24-bit or 32-bit BMP", path.string());
			return false;
		}

		SetIdentity(path.generic_string(), path.filename().string());
		m_Loaded = true;
		return true;
	}

	Ref<Asset> TextureHandler::Load(const std::filesystem::path& path)
	{
		auto texture = CreateRef<Texture>();
		if (!texture->LoadFromFile(path))
			return nullptr;

		return texture;
	}

}
