#include "VulkanBuffer.h"

#include <cstring>

namespace Lite {

	bool VulkanBuffer::Create(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage)
	{
		m_Device = device;

		VkBufferCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.size = size;
		info.usage = usage;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (!CheckVk(vkCreateBuffer(device, &info, nullptr, &m_Buffer), "create buffer"))
			return false;

		VkMemoryRequirements requirements {};
		vkGetBufferMemoryRequirements(device, m_Buffer, &requirements);

		uint32_t memoryType = FindMemoryType(
			physicalDevice,
			requirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (memoryType == UINT32_MAX)
			return false;

		VkMemoryAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = requirements.size;
		allocateInfo.memoryTypeIndex = memoryType;

		if (!CheckVk(vkAllocateMemory(device, &allocateInfo, nullptr, &m_Memory), "allocate buffer memory"))
			return false;

		return CheckVk(vkBindBufferMemory(device, m_Buffer, m_Memory, 0), "bind buffer memory");
	}

	void VulkanBuffer::Upload(const void* data, VkDeviceSize size)
	{
		void* mapped = nullptr;
		vkMapMemory(m_Device, m_Memory, 0, size, 0, &mapped);
		std::memcpy(mapped, data, static_cast<size_t>(size));
		vkUnmapMemory(m_Device, m_Memory);
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
