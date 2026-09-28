#pragma once

#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanSync.h"

#include <array>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace Lite {

	struct CameraUniform
	{
		Mat4 ViewProjection = Mat4::Identity();
	};

	struct MaterialUniform
	{
		Vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	class LITE_API UniformBuffer
	{
	public:
		bool Create(uint32_t size, uint32_t set, VkShaderStageFlags stages);
		void Destroy();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);
		void Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const;

		VkDescriptorSetLayout GetLayout() const { return m_Layout; }

	private:
		std::array<VulkanBuffer, VulkanSync::FramesInFlight> m_Buffers;
		std::array<VkDescriptorSet, VulkanSync::FramesInFlight> m_Sets {};
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		uint32_t m_Set = 0;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;
		std::vector<uint8_t> m_Data;
		uint32_t m_Size = 0;
	};

}
