#pragma once

#include "VulkanUtils.h"

namespace Lite {

	class VulkanRenderPass
	{
	public:
		bool Create(VkDevice device, VkFormat format);
		void Destroy();

		void Begin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkImageView imageView, VkExtent2D extent);
		void End(VkCommandBuffer commandBuffer);

		VkRenderPass Get() const { return m_RenderPass; }
		bool UsesDynamicRendering() const { return m_UseDynamicRendering; }

	private:
		void BeginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkExtent2D extent);
		void BeginDynamic(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent);
		void EndDynamic(VkCommandBuffer commandBuffer);

		VkDevice m_Device = VK_NULL_HANDLE;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		bool m_UseDynamicRendering = false;
	};

}
