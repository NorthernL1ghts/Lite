#include "VulkanBuffer.h"

#include <cstring>

namespace Lite {

	bool VulkanBuffer::Create(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage)
	{
		m_Device = device;

		AllocatedBuffer allocated {};
		if (!CreateBuffer(
			device,
			physicalDevice,
			size,
			usage,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			allocated,
			"create buffer"))
			return false;

		m_Buffer = allocated.Buffer;
		m_Memory = allocated.Memory;
		return true;
	}

	bool VulkanBuffer::Upload(const void* data, VkDeviceSize size)
	{
		if (!data || size == 0)
			return false;

		void* mapped = nullptr;
		if (!CheckVk(vkMapMemory(m_Device, m_Memory, 0, size, 0, &mapped), "map buffer") || !mapped)
			return false;

		std::memcpy(mapped, data, static_cast<size_t>(size));
		vkUnmapMemory(m_Device, m_Memory);
		return true;
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Buffer)
		{
			vkDestroyBuffer(m_Device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Memory)
		{
			vkFreeMemory(m_Device, m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}

	void VulkanBuffer::BindVertex(VkCommandBuffer commandBuffer) const
	{
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Buffer, &offset);
	}

	void VulkanBuffer::BindIndex(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindIndexBuffer(commandBuffer, m_Buffer, 0, VK_INDEX_TYPE_UINT16);
	}

}
