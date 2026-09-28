#pragma once

#include "VulkanUtils.h"

namespace Lite {

	class VulkanInstance
	{
	public:
		bool Create();
		void Destroy();

		VkInstance Get() const { return m_Instance; }
		bool ValidationEnabled() const { return m_Validation; }

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		bool m_Validation = false;
	};

}
