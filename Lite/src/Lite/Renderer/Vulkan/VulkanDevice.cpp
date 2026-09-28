#include <Lite/Renderer/Vulkan/VulkanDevice.h>

namespace {

	constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

}

namespace Lite {

	bool VulkanDevice::Create(VkPhysicalDevice physicalDevice, uint32_t queueFamily, bool validation)
	{
		m_QueueFamily = queueFamily;

		float priority = 1.0f;
		VkDeviceQueueCreateInfo queueInfo {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = queueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &priority;

		const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

		VkPhysicalDeviceFeatures supported {};
		vkGetPhysicalDeviceFeatures(physicalDevice, &supported);

		VkPhysicalDeviceDescriptorIndexingFeatures indexingSupport {};
		indexingSupport.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		VkPhysicalDeviceFeatures2 supportedFeatures {};
		supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		supportedFeatures.pNext = &indexingSupport;
		vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures);

		VkPhysicalDeviceDescriptorIndexingFeatures indexing {};
		indexing.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
		indexing.shaderSampledImageArrayNonUniformIndexing = indexingSupport.shaderSampledImageArrayNonUniformIndexing;
		indexing.runtimeDescriptorArray = indexingSupport.runtimeDescriptorArray;

		VkPhysicalDeviceDynamicRenderingFeatures dynamicRendering {};
		dynamicRendering.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		dynamicRendering.pNext = &indexing;
		dynamicRendering.dynamicRendering = VK_TRUE;

		VkPhysicalDeviceFeatures features {};
		features.shaderSampledImageArrayDynamicIndexing = supported.shaderSampledImageArrayDynamicIndexing;
		VkDeviceCreateInfo deviceInfo {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pNext = &dynamicRendering;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = 1;
		deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
		deviceInfo.pEnabledFeatures = &features;
		if (validation)
		{
			deviceInfo.enabledLayerCount = 1;
			deviceInfo.ppEnabledLayerNames = &ValidationLayer;
		}

		if (!CheckVk(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &m_Device), "create logical device"))
			return false;

		vkGetDeviceQueue(m_Device, queueFamily, 0, &m_GraphicsQueue);
		LITE_INFO("Vulkan device and graphics queue ready");
		return true;
	}

	void VulkanDevice::Destroy()
	{
		if (m_Device)
		{
			vkDestroyDevice(m_Device, nullptr);
			m_Device = VK_NULL_HANDLE;
		}

		m_GraphicsQueue = VK_NULL_HANDLE;
	}

	void VulkanDevice::WaitIdle() const
	{
		if (m_Device)
			vkDeviceWaitIdle(m_Device);
	}

}
