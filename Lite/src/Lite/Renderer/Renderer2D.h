#pragma once

#include "Lite/Core/Base.h"
#include "Renderer.h"
#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanFramebuffer.h"
#include "Vulkan/VulkanPipeline.h"
#include "Vulkan/VulkanBuffer.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanSync.h"

namespace Lite {

	class Shader;

	class LITE_API Renderer2D
	{
	public:
		static void Init(void* window);
		static void Shutdown();
		static bool SetShaders(const Shader& vertex, const Shader& fragment);

		static void BeginFrame();
		static void EndFrame();
		static void OnResize(int width, int height);
		static bool IsFrameActive();
	};

}
