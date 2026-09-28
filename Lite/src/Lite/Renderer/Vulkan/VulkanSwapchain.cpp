#include <Lite/Renderer/Vulkan/VulkanSwapchain.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace {

	VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		return formats.front();
	}

	VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes)
	{
		if (std::find(modes.begin(), modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != modes.end())
			return VK_PRESENT_MODE_MAILBOX_KHR;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D extent {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return extent;
	}

	VkCompositeAlphaFlagBitsKHR ChooseCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		const VkCompositeAlphaFlagBitsKHR preferred[] = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
		};

		for (auto alpha : preferred)
		{
			if (capabilities.supportedCompositeAlpha & alpha)
				return alpha;
		}

		return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}

}

namespace Lite {

	bool VulkanSwapchain::CreateSurface(VkInstance instance, GLFWwindow* window)
	{
		m_Instance = instance;
		m_Window = window;

		if (!glfwVulkanSupported())
		{
			LITE_ERROR("Vulkan is not supported on this window");
			return false;
		}

		return CheckVk(glfwCreateWindowSurface(instance, window, nullptr, &m_Surface), "create surface");
	}

	bool VulkanSwapchain::Create(VkDevice device, VkPhysicalDevice physicalDevice)
	{
		m_Device = device;
		m_PhysicalDevice = physicalDevice;

		if (!CreateSwapchain(VK_NULL_HANDLE) || !CreateImageViews())
			return false;

		LITE_INFO("Vulkan swapchain ready ({} images, {}x{})", m_Images.size(), m_Extent.width, m_Extent.height);
		return true;
	}

	bool VulkanSwapchain::Recreate()
	{
		DestroyViews();

		VkSwapchainKHR oldSwapchain = m_Swapchain;
		m_Swapchain = VK_NULL_HANDLE;
		if (!CreateSwapchain(oldSwapchain))
		{
			m_Swapchain = oldSwapchain;
			return false;
		}

		if (oldSwapchain)
			vkDestroySwapchainKHR(m_Device, oldSwapchain, nullptr);

		return CreateImageViews();
	}

	void VulkanSwapchain::Destroy()
	{
		DestroyViews();

		if (m_Swapchain)
		{
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
			m_Swapchain = VK_NULL_HANDLE;
		}
	}

	void VulkanSwapchain::DestroySurface()
	{
		if (m_Surface)
		{
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
			m_Surface = VK_NULL_HANDLE;
		}
	}

	bool VulkanSwapchain::CreateSwapchain(VkSwapchainKHR oldSwapchain)
	{
		VkSurfaceCapabilitiesKHR capabilities {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());

		uint32_t presentCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentCount, nullptr);
		std::vector<VkPresentModeKHR> presentModes(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentCount, presentModes.data());

		if (formats.empty() || presentModes.empty())
		{
			LITE_ERROR("Vulkan surface has no formats or present modes");
			return false;
		}

		VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(formats);
		if (m_Format == VK_FORMAT_UNDEFINED)
			m_Format = surfaceFormat.format;
		else
			surfaceFormat.format = m_Format;
		m_ColorSpace = surfaceFormat.colorSpace;
		m_PresentMode = ChoosePresentMode(presentModes);

		m_Extent = ChooseExtent(capabilities, m_Window);
		if (m_Extent.width == 0 || m_Extent.height == 0)
			return false;

		if (m_ImageCount == 0)
		{
			m_ImageCount = std::max(capabilities.minImageCount + 1, 2u);
			if (capabilities.maxImageCount > 0 && m_ImageCount > capabilities.maxImageCount)
				m_ImageCount = capabilities.maxImageCount;

			m_MinImageCount = std::max(capabilities.minImageCount, 2u);
			if (m_MinImageCount > m_ImageCount)
				m_MinImageCount = m_ImageCount;
		}

		VkSwapchainCreateInfoKHR info {};
		info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		info.surface = m_Surface;
		info.minImageCount = m_ImageCount;
		info.imageFormat = m_Format;
		info.imageColorSpace = surfaceFormat.colorSpace;
		info.imageExtent = m_Extent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.preTransform = capabilities.currentTransform;
		info.compositeAlpha = ChooseCompositeAlpha(capabilities);
		info.presentMode = m_PresentMode;
		info.clipped = VK_TRUE;
		info.oldSwapchain = oldSwapchain;

		return CheckVk(vkCreateSwapchainKHR(m_Device, &info, nullptr, &m_Swapchain), "create swapchain");
	}

	bool VulkanSwapchain::CreateImageViews()
	{
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_Images.data());

		m_ImageViews.resize(imageCount);
		for (uint32_t index = 0; index < imageCount; ++index)
		{
			VkImageViewCreateInfo viewInfo {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_Images[index];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_Format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.layerCount = 1;

			if (!CheckVk(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageViews[index]), "create image view"))
				return false;
		}

		return true;
	}

	void VulkanSwapchain::DestroyViews()
	{
		for (auto view : m_ImageViews)
			vkDestroyImageView(m_Device, view, nullptr);

		m_ImageViews.clear();
		m_Images.clear();
	}

}
