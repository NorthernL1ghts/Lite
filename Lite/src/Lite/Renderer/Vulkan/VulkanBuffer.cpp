#include <Lite/Renderer/Vulkan/VulkanBuffer.h>

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
		m_Capacity = size;
		m_Mapped = nullptr;
		if (!CheckVk(vkMapMemory(m_Device, m_Memory, 0, size, 0, &m_Mapped), "map buffer") || m_Mapped == nullptr)
		{
			m_Mapped = nullptr;
			Destroy();
			return false;
		}

		return true;
	}

	bool VulkanBuffer::Upload(const void* data, VkDeviceSize size)
	{
		if (m_Mapped == nullptr || data == nullptr || size == 0 || size > m_Capacity)
			return false;

		std::memcpy(m_Mapped, data, static_cast<size_t>(size));
		return true;
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Mapped != nullptr && m_Memory != VK_NULL_HANDLE)
		{
			vkUnmapMemory(m_Device, m_Memory);
			m_Mapped = nullptr;
		}

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

		m_Capacity = 0;
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
