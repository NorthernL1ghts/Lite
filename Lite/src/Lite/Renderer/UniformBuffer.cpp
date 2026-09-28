#include "UniformBuffer.h"

#include "Renderer.h"
#include "Vulkan/VulkanUtils.h"

#include <cstring>

namespace Lite {

	bool UniformBuffer::Create(uint32_t size, uint32_t set, VkShaderStageFlags stages)
	{
		m_Device = Renderer::GetDevice();
		m_Size = size;
		m_Set = set;
		m_Data.assign(size, 0);

		for (uint32_t frame = 0; frame < VulkanSync::FramesInFlight; ++frame)
		{
			if (!m_Buffers[frame].Create(m_Device, Renderer::GetPhysicalDevice(), size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
			{
				Destroy();
				return false;
			}
		}

		VkDescriptorSetLayoutBinding binding {};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = stages;

		VkDescriptorSetLayoutCreateInfo layoutInfo {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &binding;
		if (!CheckVk(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_Layout), "create uniform layout"))
		{
			Destroy();
			return false;
		}

		VkDescriptorPoolSize poolSize {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = VulkanSync::FramesInFlight;

		VkDescriptorPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = VulkanSync::FramesInFlight;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		if (!CheckVk(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_Pool), "create uniform pool"))
		{
			Destroy();
			return false;
		}

		std::array<VkDescriptorSetLayout, VulkanSync::FramesInFlight> layouts {};
		layouts.fill(m_Layout);

		VkDescriptorSetAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_Pool;
		allocateInfo.descriptorSetCount = VulkanSync::FramesInFlight;
		allocateInfo.pSetLayouts = layouts.data();
		if (!CheckVk(vkAllocateDescriptorSets(m_Device, &allocateInfo, m_Sets.data()), "allocate uniform sets"))
		{
			Destroy();
			return false;
		}

		for (uint32_t frame = 0; frame < VulkanSync::FramesInFlight; ++frame)
		{
			VkDescriptorBufferInfo bufferInfo {};
			bufferInfo.buffer = m_Buffers[frame].Get();
			bufferInfo.range = size;

			VkWriteDescriptorSet write {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = m_Sets[frame];
			write.dstBinding = 0;
			write.descriptorCount = 1;
			write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			write.pBufferInfo = &bufferInfo;
			vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
		}

		LITE_INFO("Uniform buffer ready (set {}, {} bytes, {} frames)", set, size, VulkanSync::FramesInFlight);
		return true;
	}

	void UniformBuffer::Destroy()
	{
		if (m_Pool)
		{
			vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
			m_Pool = VK_NULL_HANDLE;
		}

		m_Sets.fill(VK_NULL_HANDLE);
		m_Set = 0;

		if (m_Layout)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}

		for (VulkanBuffer& buffer : m_Buffers)
			buffer.Destroy();

		m_Data.clear();
		m_Size = 0;
	}

	void UniformBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		if (!data || size == 0 || size > m_Size || offset > m_Size - size)
			return;

		std::memcpy(m_Data.data() + offset, data, size);
		if (!m_Buffers[Renderer::GetFrameIndex()].Upload(m_Data.data(), m_Size))
			return;
	}

	void UniformBuffer::Bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const
	{
		uint32_t frame = Renderer::GetFrameIndex();
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, m_Set, 1, &m_Sets[frame], 0, nullptr);
	}

}
