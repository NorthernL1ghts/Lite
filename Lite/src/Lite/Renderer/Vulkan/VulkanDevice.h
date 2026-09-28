#pragma once

#include "VulkanUtils.h"

#include <cstdint>

namespace Lite {

	class VulkanDevice
	{
	public:
		bool Create(VkPhysicalDevice physicalDevice, uint32_t queueFamily, bool validation);
		void Destroy();
		void WaitIdle() const;

		VkDevice Get() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		uint32_t GetQueueFamily() const { return m_QueueFamily; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		uint32_t m_QueueFamily = 0;
	};

}
