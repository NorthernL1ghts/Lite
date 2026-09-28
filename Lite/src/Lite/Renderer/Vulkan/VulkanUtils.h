#pragma once

#include "Lite/Core/Logger.h"

#include <cstdint>
#include <vulkan/vulkan.h>

namespace Lite {

	inline bool CheckVk(VkResult result, const char* action)
	{
		if (result == VK_SUCCESS)
			return true;

		LITE_ERROR("Vulkan {} failed ({})", action, static_cast<int>(result));
		return false;
	}

	inline uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeBits, VkMemoryPropertyFlags properties)
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

}
