#include <Lite/Renderer/Vulkan/VulkanPhysicalDevice.h>

#include <cstring>
#include <vector>

namespace {

	bool SupportsSwapchain(VkPhysicalDevice device)
	{
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

		for (const auto& extension : extensions)
		{
			if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
				return true;
		}

		return false;
	}

	bool FindGraphicsQueue(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& family)
	{
		uint32_t count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
		std::vector<VkQueueFamilyProperties> families(count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

		for (uint32_t index = 0; index < count; ++index)
		{
			if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
				continue;

			VkBool32 present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &present);
			if (present)
			{
				family = index;
				return true;
			}
		}

		return false;
	}

}

namespace Lite {

	bool VulkanPhysicalDevice::Pick(VkInstance instance, VkSurfaceKHR surface)
	{
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(instance, &count, nullptr);
		if (count == 0)
		{
			LITE_ERROR("No Vulkan physical devices were found");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(instance, &count, devices.data());

		VkPhysicalDevice fallback = VK_NULL_HANDLE;
		uint32_t fallbackFamily = 0;

		for (auto device : devices)
		{
			if (!SupportsSwapchain(device))
				continue;

			uint32_t family = 0;
			if (!FindGraphicsQueue(device, surface, family))
				continue;

			VkPhysicalDeviceProperties properties {};
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_Device = device;
				m_QueueFamily = family;
				LITE_INFO("Vulkan GPU {}", properties.deviceName);
				return true;
			}

			if (!fallback)
			{
				fallback = device;
				fallbackFamily = family;
			}
		}

		if (!fallback)
		{
			LITE_ERROR("No Vulkan GPU can present to the window");
			return false;
		}

		m_Device = fallback;
		m_QueueFamily = fallbackFamily;

		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties(m_Device, &properties);
		LITE_INFO("Vulkan GPU {}", properties.deviceName);
		return true;
	}

	void VulkanPhysicalDevice::Clear()
	{
		m_Device = VK_NULL_HANDLE;
		m_QueueFamily = 0;
	}

}
