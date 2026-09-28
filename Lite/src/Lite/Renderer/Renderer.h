#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Math/Math.h>
#include <Lite/Renderer/Vulkan/VulkanInstance.h>
#include <Lite/Renderer/Vulkan/VulkanPhysicalDevice.h>
#include <Lite/Renderer/Vulkan/VulkanDevice.h>
#include <Lite/Renderer/Vulkan/VulkanSwapchain.h>
#include <Lite/Renderer/Vulkan/VulkanRenderPass.h>
#include <Lite/Renderer/Vulkan/VulkanCommandBuffer.h>
#include <Lite/Renderer/Vulkan/VulkanSync.h>

namespace Lite {

	class LITE_API Renderer
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static void SetViewProjection(const Mat4& viewProjection);
		static const Mat4& GetViewProjection();
		static void SetTransform(const Transform& transform);
		static void SetModel(const Mat4& model);
		static const Mat4& GetModel();

		static void BeginFrame();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();

		static VkCommandBuffer GetCommandBuffer();
		static VkInstance GetInstance();
		static VkPhysicalDevice GetPhysicalDevice();
		static VkDevice GetDevice();
		static VkQueue GetGraphicsQueue();
		static uint32_t GetGraphicsQueueFamily();
		static VkRenderPass GetRenderPass();
		static uint32_t GetImageCount();
		static uint32_t GetMinImageCount();
		static void RecordDraw(uint32_t indexCount);
		static uint32_t GetDrawCalls();
		static uint32_t GetQuadCount();
		static uint32_t GetTriangleCount();
		static uint32_t GetIndexCount();
		static VkExtent2D GetExtent();
		static VkFormat GetSwapchainFormat();
		static VkColorSpaceKHR GetColorSpace();
		static VkPresentModeKHR GetPresentMode();
		static uint32_t GetImageIndex();
		static uint32_t GetFrameIndex();
	};

}
