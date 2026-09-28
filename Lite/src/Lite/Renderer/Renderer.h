#pragma once

#include "Lite/Core/Base.h"
#include "Lite/Math/Math.h"
#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanSync.h"

namespace Lite {

	class LITE_API Renderer
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static void SetViewProjection(const Mat4& viewProjection);
		static const Mat4& GetViewProjection();

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
		static void SetIndexCount(uint32_t count);
		static uint32_t GetIndexCount();
		static VkExtent2D GetExtent();
		static uint32_t GetFrameIndex();
	};

}
