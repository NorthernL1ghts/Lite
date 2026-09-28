#pragma once

#include <Lite/Renderer/Vulkan/VulkanUtils.h>

#include <cstdint>
#include <vector>

namespace Lite {

	class VulkanCommandBuffer
	{
	public:
		bool Create(VkDevice device, uint32_t queueFamily, uint32_t count);
		void Destroy();

		VkCommandBuffer Begin(uint32_t frame);
		bool End(uint32_t frame);
		VkCommandBuffer Get(uint32_t frame) const { return m_Buffers[frame]; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkCommandPool m_Pool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_Buffers;
	};

}
