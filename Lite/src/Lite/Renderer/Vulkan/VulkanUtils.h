#pragma once

#include <Lite/Core/Log/Logger.h>

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Lite {

	struct AllocatedBuffer
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
	};

	[[nodiscard]] inline bool CheckVk(VkResult result, const char* action)
	{
		if (result == VK_SUCCESS)
			return true;

		LITE_ERROR("Vulkan {} failed ({})", action, static_cast<int>(result));
		return false;
	}

	[[nodiscard]] inline uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memory {};
		vkGetPhysicalDeviceMemoryProperties(device, &memory);

		for (uint32_t index = 0; index < memory.memoryTypeCount; ++index)
		{
			if ((typeBits & (1u << index)) && (memory.memoryTypes[index].propertyFlags & properties) == properties)
				return index;
		}

		LITE_ERROR("No matching Vulkan memory type");
		return UINT32_MAX;
	}

	inline void DestroyBuffer(VkDevice device, AllocatedBuffer& buffer)
	{
		if (buffer.Buffer)
			vkDestroyBuffer(device, buffer.Buffer, nullptr);
		if (buffer.Memory)
			vkFreeMemory(device, buffer.Memory, nullptr);

		buffer.Buffer = VK_NULL_HANDLE;
		buffer.Memory = VK_NULL_HANDLE;
	}

	[[nodiscard]] inline bool AllocateMemory(VkDevice device, VkPhysicalDevice physicalDevice, const VkMemoryRequirements& requirements, VkMemoryPropertyFlags properties, VkDeviceMemory& memory, const char* action)
	{
		uint32_t memoryType = FindMemoryType(physicalDevice, requirements.memoryTypeBits, properties);
		if (memoryType == UINT32_MAX)
			return false;

		VkMemoryAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = requirements.size;
		allocateInfo.memoryTypeIndex = memoryType;
		return CheckVk(vkAllocateMemory(device, &allocateInfo, nullptr, &memory), action);
	}

	[[nodiscard]] inline bool CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, AllocatedBuffer& out, const char* action)
	{
		out = {};

		VkBufferCreateInfo info {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.size = size;
		info.usage = usage;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (!CheckVk(vkCreateBuffer(device, &info, nullptr, &out.Buffer), action))
			return false;

		VkMemoryRequirements requirements {};
		vkGetBufferMemoryRequirements(device, out.Buffer, &requirements);
		if (!AllocateMemory(device, physicalDevice, requirements, properties, out.Memory, action)
			|| !CheckVk(vkBindBufferMemory(device, out.Buffer, out.Memory, 0), action))
		{
			DestroyBuffer(device, out);
			return false;
		}

		return true;
	}

	template<typename Record>
	[[nodiscard]] inline bool SubmitOnce(VkDevice device, uint32_t queueFamily, VkQueue queue, Record&& record, const char* action)
	{
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		poolInfo.queueFamilyIndex = queueFamily;

		VkCommandPool pool = VK_NULL_HANDLE;
		if (!CheckVk(vkCreateCommandPool(device, &poolInfo, nullptr, &pool), action))
			return false;

		VkCommandBufferAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;

		VkCommandBuffer command = VK_NULL_HANDLE;
		bool ready = CheckVk(vkAllocateCommandBuffers(device, &allocateInfo, &command), action);
		if (ready)
		{
			VkCommandBufferBeginInfo begin {};
			begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			ready = CheckVk(vkBeginCommandBuffer(command, &begin), action);
			if (ready)
			{
				record(command);
				ready = CheckVk(vkEndCommandBuffer(command), action);
			}

			if (ready)
			{
				VkSubmitInfo submit {};
				submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submit.commandBufferCount = 1;
				submit.pCommandBuffers = &command;
				ready = CheckVk(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), action)
					&& CheckVk(vkQueueWaitIdle(queue), action);
			}
		}

		vkDestroyCommandPool(device, pool, nullptr);
		return ready;
	}

	[[nodiscard]] inline VkImageMemoryBarrier ImageBarrier(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
	{
		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = srcAccess;
		barrier.dstAccessMask = dstAccess;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		return barrier;
	}

}
