#pragma once

#include "VulkanUtils.h"

namespace Lite {

	class VulkanBuffer
	{
	public:
		bool Create(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage);
		bool Upload(const void* data, VkDeviceSize size);
		void Destroy();

		void BindVertex(VkCommandBuffer commandBuffer) const;
		void BindIndex(VkCommandBuffer commandBuffer) const;

		VkBuffer Get() const { return m_Buffer; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
	};

}
