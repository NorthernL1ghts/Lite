#include "VulkanCommandBuffer.h"

namespace Lite {

	bool VulkanCommandBuffer::Create(VkDevice device, uint32_t queueFamily, uint32_t count)
	{
		m_Device = device;

		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamily;

		if (!CheckVk(vkCreateCommandPool(device, &poolInfo, nullptr, &m_Pool), "create command pool"))
			return false;

		m_Buffers.resize(count);
		VkCommandBufferAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_Pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = count;

		return CheckVk(vkAllocateCommandBuffers(device, &allocateInfo, m_Buffers.data()), "allocate command buffers");
	}

	void VulkanCommandBuffer::Destroy()
	{
		m_Buffers.clear();

		if (m_Pool)
		{
			vkDestroyCommandPool(m_Device, m_Pool, nullptr);
			m_Pool = VK_NULL_HANDLE;
		}
	}

	VkCommandBuffer VulkanCommandBuffer::Begin(uint32_t frame)
	{
		VkCommandBuffer commandBuffer = m_Buffers[frame];
		vkResetCommandBuffer(commandBuffer, 0);

		VkCommandBufferBeginInfo info {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		CheckVk(vkBeginCommandBuffer(commandBuffer, &info), "begin command buffer");
		return commandBuffer;
	}

	void VulkanCommandBuffer::End(uint32_t frame)
	{
		CheckVk(vkEndCommandBuffer(m_Buffers[frame]), "end command buffer");
	}

}
