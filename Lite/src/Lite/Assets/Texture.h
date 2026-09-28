#pragma once

#include "AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace Lite {

	class LITE_API Texture : public Asset
	{
	public:
		static constexpr AssetType Type = AssetType::Texture;

		AssetType GetType() const override { return Type; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		const std::vector<uint8_t>& GetPixels() const { return m_Pixels; }

	private:
		friend class TextureHandler;

		bool LoadFromFile(const std::filesystem::path& path);

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		std::vector<uint8_t> m_Pixels;
	};

	class TextureHandler : public AssetHandler
	{
	public:
		AssetType GetType() const override { return AssetType::Texture; }
		std::shared_ptr<Asset> Load(const std::filesystem::path& path) override;
	};

}
