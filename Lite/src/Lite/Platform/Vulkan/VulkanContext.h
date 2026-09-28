#pragma once

#include "Lite/Renderer/RenderContext.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

struct GLFWwindow;

namespace Lite {

	class VulkanContext : public RenderContext
	{
	public:
		static constexpr uint32_t FramesInFlight = 2;

		VulkanContext() = default;
		~VulkanContext() override;

		void Init(void* window) override;
		void BeginFrame() override;
		void EndFrame() override;
		void OnResize(int width, int height) override;
		bool IsFrameActive() const override { return m_FrameActive; }

		VkInstance GetInstance() const { return m_Instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkCommandBuffer GetCommandBuffer() const;
		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_SwapchainImages.size()); }
		uint32_t GetMinImageCount() const { return m_MinImageCount; }

	private:
		struct Frame
		{
			VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
			VkSemaphore ImageAvailable = VK_NULL_HANDLE;
			VkSemaphore RenderFinished = VK_NULL_HANDLE;
			VkFence InFlight = VK_NULL_HANDLE;
		};

		void Shutdown();
		bool CreateInstance();
		bool CreateSurface();
		bool PickPhysicalDevice();
		bool CreateDevice();
		bool CreateSwapchain(VkSwapchainKHR oldSwapchain);
		bool CreateImageViews();
		bool CreateRenderPass();
		bool CreateFramebuffers();
		bool CreateCommands();
		bool CreateSync();
		bool RecreateSwapchain();
		void DestroySwapchainViews();

		GLFWwindow* m_Window = nullptr;
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		uint32_t m_GraphicsQueueFamily = 0;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent {};
		uint32_t m_ImageCount = 0;
		uint32_t m_MinImageCount = 2;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;
		std::vector<VkFence> m_ImageFences;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<Frame> m_Frames;
		uint32_t m_CurrentFrame = 0;
		uint32_t m_ImageIndex = 0;
		bool m_FrameActive = false;
		bool m_FramebufferResized = false;
		bool m_Validation = false;
	};

}
