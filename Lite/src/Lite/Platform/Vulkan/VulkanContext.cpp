#include "VulkanContext.h"

#include "Lite/Core/Logger.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace {

	constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

	bool Check(VkResult result, const char* action)
	{
		if (result == VK_SUCCESS)
			return true;

		LITE_ERROR("Vulkan {} failed ({})", action, static_cast<int>(result));
		return false;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* data,
		void*)
	{
		if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			LITE_ERROR("Vulkan: {}", data->pMessage);
		else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			LITE_WARN("Vulkan: {}", data->pMessage);

		return VK_FALSE;
	}

	bool HasValidationLayer()
	{
		uint32_t count = 0;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		std::vector<VkLayerProperties> layers(count);
		vkEnumerateInstanceLayerProperties(&count, layers.data());

		return std::any_of(layers.begin(), layers.end(), [](const VkLayerProperties& layer)
		{
			return std::strcmp(layer.layerName, ValidationLayer) == 0;
		});
	}

	bool SupportsSwapchain(VkPhysicalDevice device)
	{
		uint32_t count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
		std::vector<VkExtensionProperties> extensions(count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());

		return std::any_of(extensions.begin(), extensions.end(), [](const VkExtensionProperties& extension)
		{
			return std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
		});
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

	VulkanContext::~VulkanContext()
	{
		Shutdown();
	}

	VkCommandBuffer VulkanContext::GetCommandBuffer() const
	{
		return m_Frames[m_CurrentFrame].CommandBuffer;
	}

	void VulkanContext::Init(void* window)
	{
		m_Window = static_cast<GLFWwindow*>(window);

		if (!glfwVulkanSupported())
		{
			LITE_ERROR("Vulkan is not supported");
			return;
		}

		if (!CreateInstance() || !CreateSurface() || !PickPhysicalDevice() || !CreateDevice())
			return;

		if (!CreateSwapchain(VK_NULL_HANDLE) || !CreateImageViews() || !CreateRenderPass() || !CreateFramebuffers())
			return;

		if (!CreateCommands() || !CreateSync())
			return;

		LITE_INFO("Vulkan render context ready ({} images, {}x{})", m_SwapchainImages.size(), m_Extent.width, m_Extent.height);
	}

	void VulkanContext::BeginFrame()
	{
		m_FrameActive = false;
		if (!m_Swapchain)
			return;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(m_Window, &width, &height);
		if (width == 0 || height == 0)
		{
			m_FramebufferResized = true;
			return;
		}

		if (m_FramebufferResized)
		{
			m_FramebufferResized = false;
			if (!RecreateSwapchain())
			{
				m_FramebufferResized = true;
				return;
			}
		}

		Frame& frame = m_Frames[m_CurrentFrame];
		if (!Check(vkWaitForFences(m_Device, 1, &frame.InFlight, VK_TRUE, UINT64_MAX), "wait for frame fence"))
			return;

		VkResult acquire = vkAcquireNextImageKHR(
			m_Device,
			m_Swapchain,
			UINT64_MAX,
			frame.ImageAvailable,
			VK_NULL_HANDLE,
			&m_ImageIndex);

		if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_FramebufferResized = true;
			return;
		}

		if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
		{
			Check(acquire, "acquire swapchain image");
			return;
		}

		if (m_ImageFences[m_ImageIndex] != VK_NULL_HANDLE)
			vkWaitForFences(m_Device, 1, &m_ImageFences[m_ImageIndex], VK_TRUE, UINT64_MAX);
		m_ImageFences[m_ImageIndex] = frame.InFlight;

		vkResetFences(m_Device, 1, &frame.InFlight);
		vkResetCommandBuffer(frame.CommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (!Check(vkBeginCommandBuffer(frame.CommandBuffer, &beginInfo), "begin command buffer"))
			return;

		VkClearValue clear {};
		clear.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo passInfo {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passInfo.renderPass = m_RenderPass;
		passInfo.framebuffer = m_Framebuffers[m_ImageIndex];
		passInfo.renderArea.extent = m_Extent;
		passInfo.clearValueCount = 1;
		passInfo.pClearValues = &clear;
		vkCmdBeginRenderPass(frame.CommandBuffer, &passInfo, VK_SUBPASS_CONTENTS_INLINE);

		m_FrameActive = true;
		if (acquire == VK_SUBOPTIMAL_KHR)
			m_FramebufferResized = true;
	}

	void VulkanContext::EndFrame()
	{
		if (!m_FrameActive)
			return;

		Frame& frame = m_Frames[m_CurrentFrame];
		vkCmdEndRenderPass(frame.CommandBuffer);
		m_FrameActive = false;

		if (!Check(vkEndCommandBuffer(frame.CommandBuffer), "end command buffer"))
			return;

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &frame.ImageAvailable;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame.CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frame.RenderFinished;

		if (!Check(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, frame.InFlight), "submit frame"))
			return;

		VkPresentInfoKHR presentInfo {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frame.RenderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_Swapchain;
		presentInfo.pImageIndices = &m_ImageIndex;

		VkResult present = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
		if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
			m_FramebufferResized = true;
		else
			Check(present, "present frame");

		m_CurrentFrame = (m_CurrentFrame + 1) % FramesInFlight;
	}

	void VulkanContext::OnResize(int, int)
	{
		m_FramebufferResized = true;
	}

	void VulkanContext::Shutdown()
	{
		if (m_Device)
			vkDeviceWaitIdle(m_Device);

		for (auto& frame : m_Frames)
		{
			if (frame.ImageAvailable)
				vkDestroySemaphore(m_Device, frame.ImageAvailable, nullptr);
			if (frame.RenderFinished)
				vkDestroySemaphore(m_Device, frame.RenderFinished, nullptr);
			if (frame.InFlight)
				vkDestroyFence(m_Device, frame.InFlight, nullptr);
		}
		m_Frames.clear();
		m_ImageFences.clear();

		if (m_CommandPool)
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

		DestroySwapchainViews();
		if (m_Swapchain)
			vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		if (m_RenderPass)
			vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
		if (m_Device)
			vkDestroyDevice(m_Device, nullptr);
		if (m_Surface)
			vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

		if (m_DebugMessenger)
		{
			auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));
			if (destroy)
				destroy(m_Instance, m_DebugMessenger, nullptr);
		}

		if (m_Instance)
			vkDestroyInstance(m_Instance, nullptr);

		m_CommandPool = VK_NULL_HANDLE;
		m_Swapchain = VK_NULL_HANDLE;
		m_RenderPass = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_DebugMessenger = VK_NULL_HANDLE;
		m_Instance = VK_NULL_HANDLE;
		m_PhysicalDevice = VK_NULL_HANDLE;
		m_GraphicsQueue = VK_NULL_HANDLE;
		m_FrameActive = false;
	}

	bool VulkanContext::CreateInstance()
	{
#ifndef NDEBUG
		m_Validation = HasValidationLayer();
		if (!m_Validation)
			LITE_WARN("Vulkan validation layers were not found");
#endif

		uint32_t extensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		if (!glfwExtensions)
		{
			LITE_ERROR("GLFW did not report Vulkan instance extensions");
			return false;
		}

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionCount);
		if (m_Validation)
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		VkApplicationInfo application {};
		application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application.pApplicationName = "Lite";
		application.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		application.pEngineName = "Lite";
		application.engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		application.apiVersion = VK_API_VERSION_1_3;

		VkDebugUtilsMessengerCreateInfoEXT debugInfo {};
		debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugInfo.pfnUserCallback = DebugCallback;

		VkInstanceCreateInfo instanceInfo {};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &application;
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();
		if (m_Validation)
		{
			instanceInfo.enabledLayerCount = 1;
			instanceInfo.ppEnabledLayerNames = &ValidationLayer;
			instanceInfo.pNext = &debugInfo;
		}

		if (!Check(vkCreateInstance(&instanceInfo, nullptr, &m_Instance), "create instance"))
			return false;

		if (!m_Validation)
			return true;

		auto createDebug = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
		if (!createDebug || !Check(createDebug(m_Instance, &debugInfo, nullptr, &m_DebugMessenger), "create debug messenger"))
			return false;

		return true;
	}

	bool VulkanContext::CreateSurface()
	{
		return Check(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface), "create window surface");
	}

	bool VulkanContext::PickPhysicalDevice()
	{
		uint32_t count = 0;
		vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
		if (count == 0)
		{
			LITE_ERROR("No Vulkan physical devices were found");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

		VkPhysicalDevice fallback = VK_NULL_HANDLE;
		uint32_t fallbackFamily = 0;

		for (auto device : devices)
		{
			if (!SupportsSwapchain(device))
				continue;

			uint32_t family = 0;
			if (!FindGraphicsQueue(device, m_Surface, family))
				continue;

			VkPhysicalDeviceProperties properties {};
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_PhysicalDevice = device;
				m_GraphicsQueueFamily = family;
				LITE_INFO("Vulkan device {}", properties.deviceName);
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
			LITE_ERROR("No Vulkan device can present to the window");
			return false;
		}

		m_PhysicalDevice = fallback;
		m_GraphicsQueueFamily = fallbackFamily;

		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
		LITE_INFO("Vulkan device {}", properties.deviceName);
		return true;
	}

	bool VulkanContext::CreateDevice()
	{
		float priority = 1.0f;
		VkDeviceQueueCreateInfo queueInfo {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = m_GraphicsQueueFamily;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &priority;

		const char* swapchainExtension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		VkPhysicalDeviceFeatures features {};

		VkDeviceCreateInfo deviceInfo {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.enabledExtensionCount = 1;
		deviceInfo.ppEnabledExtensionNames = &swapchainExtension;
		deviceInfo.pEnabledFeatures = &features;
		if (m_Validation)
		{
			deviceInfo.enabledLayerCount = 1;
			deviceInfo.ppEnabledLayerNames = &ValidationLayer;
		}

		if (!Check(vkCreateDevice(m_PhysicalDevice, &deviceInfo, nullptr, &m_Device), "create device"))
			return false;

		vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
		return true;
	}

	bool VulkanContext::CreateSwapchain(VkSwapchainKHR oldSwapchain)
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
		if (m_SwapchainFormat == VK_FORMAT_UNDEFINED)
			m_SwapchainFormat = surfaceFormat.format;
		else
			surfaceFormat.format = m_SwapchainFormat;

		m_Extent = ChooseExtent(capabilities, m_Window);
		if (m_Extent.width == 0 || m_Extent.height == 0)
			return false;

		if (m_ImageCount == 0)
		{
			m_ImageCount = capabilities.minImageCount + 1;
			if (m_ImageCount < 2)
				m_ImageCount = 2;
			if (capabilities.maxImageCount > 0 && m_ImageCount > capabilities.maxImageCount)
				m_ImageCount = capabilities.maxImageCount;

			m_MinImageCount = capabilities.minImageCount > 2 ? capabilities.minImageCount : 2;
			if (m_MinImageCount > m_ImageCount)
				m_MinImageCount = m_ImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainInfo {};
		swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainInfo.surface = m_Surface;
		swapchainInfo.minImageCount = m_ImageCount;
		swapchainInfo.imageFormat = m_SwapchainFormat;
		swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainInfo.imageExtent = m_Extent;
		swapchainInfo.imageArrayLayers = 1;
		swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainInfo.preTransform = capabilities.currentTransform;
		swapchainInfo.compositeAlpha = ChooseCompositeAlpha(capabilities);
		swapchainInfo.presentMode = ChoosePresentMode(presentModes);
		swapchainInfo.clipped = VK_TRUE;
		swapchainInfo.oldSwapchain = oldSwapchain;

		return Check(vkCreateSwapchainKHR(m_Device, &swapchainInfo, nullptr, &m_Swapchain), "create swapchain");
	}

	bool VulkanContext::CreateImageViews()
	{
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

		m_ImageViews.resize(imageCount);
		for (uint32_t index = 0; index < imageCount; ++index)
		{
			VkImageViewCreateInfo viewInfo {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_SwapchainImages[index];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = m_SwapchainFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.layerCount = 1;

			if (!Check(vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageViews[index]), "create image view"))
				return false;
		}

		m_ImageFences.assign(imageCount, VK_NULL_HANDLE);
		return true;
	}

	bool VulkanContext::CreateRenderPass()
	{
		VkAttachmentDescription color {};
		color.format = m_SwapchainFormat;
		color.samples = VK_SAMPLE_COUNT_1_BIT;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorRef {};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;

		VkSubpassDependency dependency {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo passInfo {};
		passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passInfo.attachmentCount = 1;
		passInfo.pAttachments = &color;
		passInfo.subpassCount = 1;
		passInfo.pSubpasses = &subpass;
		passInfo.dependencyCount = 1;
		passInfo.pDependencies = &dependency;

		return Check(vkCreateRenderPass(m_Device, &passInfo, nullptr, &m_RenderPass), "create render pass");
	}

	bool VulkanContext::CreateFramebuffers()
	{
		m_Framebuffers.resize(m_ImageViews.size());
		for (size_t index = 0; index < m_ImageViews.size(); ++index)
		{
			VkImageView attachment = m_ImageViews[index];
			VkFramebufferCreateInfo framebufferInfo {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &attachment;
			framebufferInfo.width = m_Extent.width;
			framebufferInfo.height = m_Extent.height;
			framebufferInfo.layers = 1;

			if (!Check(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[index]), "create framebuffer"))
				return false;
		}

		return true;
	}

	bool VulkanContext::CreateCommands()
	{
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;
		if (!Check(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool), "create command pool"))
			return false;

		m_Frames.resize(FramesInFlight);
		std::vector<VkCommandBuffer> buffers(FramesInFlight);

		VkCommandBufferAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = FramesInFlight;
		if (!Check(vkAllocateCommandBuffers(m_Device, &allocateInfo, buffers.data()), "allocate command buffers"))
			return false;

		for (uint32_t index = 0; index < FramesInFlight; ++index)
			m_Frames[index].CommandBuffer = buffers[index];

		return true;
	}

	bool VulkanContext::CreateSync()
	{
		VkSemaphoreCreateInfo semaphoreInfo {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (auto& frame : m_Frames)
		{
			if (!Check(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.ImageAvailable), "create image semaphore"))
				return false;
			if (!Check(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &frame.RenderFinished), "create render semaphore"))
				return false;
			if (!Check(vkCreateFence(m_Device, &fenceInfo, nullptr, &frame.InFlight), "create frame fence"))
				return false;
		}

		return true;
	}

	bool VulkanContext::RecreateSwapchain()
	{
		vkDeviceWaitIdle(m_Device);
		DestroySwapchainViews();

		VkSwapchainKHR oldSwapchain = m_Swapchain;
		m_Swapchain = VK_NULL_HANDLE;
		if (!CreateSwapchain(oldSwapchain))
		{
			m_Swapchain = oldSwapchain;
			return false;
		}

		vkDestroySwapchainKHR(m_Device, oldSwapchain, nullptr);
		return CreateImageViews() && CreateFramebuffers();
	}

	void VulkanContext::DestroySwapchainViews()
	{
		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
		m_Framebuffers.clear();

		for (auto view : m_ImageViews)
			vkDestroyImageView(m_Device, view, nullptr);
		m_ImageViews.clear();
		m_SwapchainImages.clear();
	}

}
