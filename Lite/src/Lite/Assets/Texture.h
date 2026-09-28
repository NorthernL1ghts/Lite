#pragma once

#include "AssetHandler.h"

#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace Lite {

	class LITE_API Texture : public Asset
	{
	public:
		static constexpr AssetType Type = AssetType::Texture;

		~Texture();

		[[nodiscard]] static Ref<Texture> Create(uint32_t width, uint32_t height, const uint8_t* rgba);

		AssetType GetType() const override { return Type; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		VkDescriptorSetLayout GetSetLayout() const { return m_SetLayout; }
		VkSampler GetSampler() const { return m_Sampler; }
		VkImageView GetView() const { return m_View; }

		void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const;

	private:
		friend class TextureHandler;

		bool LoadFromFile(const std::filesystem::path& path);
		bool CreateGpu(const std::vector<uint8_t>& pixels, std::string_view name);
		void DestroyGpu();

		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		VkImageView m_View = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
		VkDescriptorSet m_Set = VK_NULL_HANDLE;
	};

	class TextureHandler : public AssetHandler
	{
	public:
		AssetType GetType() const override { return AssetType::Texture; }
		Ref<Asset> Load(const std::filesystem::path& path) override;
	};

}
