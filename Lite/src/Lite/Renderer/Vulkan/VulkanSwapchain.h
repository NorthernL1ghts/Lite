#pragma once

#include <Lite/Renderer/Vulkan/VulkanUtils.h>

#include <cstdint>
#include <vector>

struct GLFWwindow;

namespace Lite {

	class VulkanSwapchain
	{
	public:
		bool CreateSurface(VkInstance instance, GLFWwindow* window);
		bool Create(VkDevice device, VkPhysicalDevice physicalDevice);
		bool Recreate();
		void Destroy();
		void DestroySurface();

		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkSwapchainKHR Get() const { return m_Swapchain; }
		VkFormat GetFormat() const { return m_Format; }
		VkColorSpaceKHR GetColorSpace() const { return m_ColorSpace; }
		VkPresentModeKHR GetPresentMode() const { return m_PresentMode; }
		VkExtent2D GetExtent() const { return m_Extent; }
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
		uint32_t GetMinImageCount() const { return m_MinImageCount; }
		const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }
		VkImageView GetImageView(uint32_t index) const { return m_ImageViews[index]; }

	private:
		bool CreateSwapchain(VkSwapchainKHR oldSwapchain);
		bool CreateImageViews();
		void DestroyViews();

		GLFWwindow* m_Window = nullptr;
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkExtent2D m_Extent {};
		uint32_t m_ImageCount = 0;
		uint32_t m_MinImageCount = 2;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
	};

}
