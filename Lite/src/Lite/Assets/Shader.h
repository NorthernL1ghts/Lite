#pragma once

#include "AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>

namespace Lite {

	enum class ShaderStage
	{
		Unknown = 0,
		Vertex,
		Fragment
	};

	class LITE_API Shader : public Asset
	{
	public:
		static constexpr AssetType Type = AssetType::Shader;

		AssetType GetType() const override { return Type; }

		ShaderStage GetStage() const { return m_Stage; }
		const std::vector<uint32_t>& GetCode() const { return m_Code; }

		VkShaderModule CreateModule(VkDevice device) const;

	private:
		friend class ShaderHandler;

		bool LoadFromFile(const std::filesystem::path& path);

		ShaderStage m_Stage = ShaderStage::Unknown;
		std::vector<uint32_t> m_Code;
	};

	class ShaderHandler : public AssetHandler
	{
	public:
		AssetType GetType() const override { return AssetType::Shader; }
		Ref<Asset> Load(const std::filesystem::path& path) override;
	};

}
