#pragma once

#include "VulkanUtils.h"

#include <cstdint>

namespace Lite {

	class VulkanPhysicalDevice
	{
	public:
		bool Pick(VkInstance instance, VkSurfaceKHR surface);
		void Clear();

		VkPhysicalDevice Get() const { return m_Device; }
		uint32_t GetQueueFamily() const { return m_QueueFamily; }

	private:
		VkPhysicalDevice m_Device = VK_NULL_HANDLE;
		uint32_t m_QueueFamily = 0;
	};

}
