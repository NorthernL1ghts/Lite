#include "VulkanRenderPass.h"

namespace Lite {

	bool VulkanRenderPass::Create(VkDevice device, VkFormat format)
	{
		m_Device = device;
		m_Format = format;
		m_UseDynamicRendering = false;

		VkAttachmentDescription color {};
		color.format = format;
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

		if (!CheckVk(vkCreateRenderPass(device, &passInfo, nullptr, &m_RenderPass), "create render pass"))
			return false;

		LITE_INFO("Vulkan render pass ready");
		return true;
	}

	void VulkanRenderPass::Destroy()
	{
		if (m_RenderPass)
		{
			vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
			m_RenderPass = VK_NULL_HANDLE;
		}
	}

	void VulkanRenderPass::Begin(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkImageView imageView, VkExtent2D extent)
	{
		if (m_UseDynamicRendering)
			BeginDynamic(commandBuffer, imageView, extent);
		else
			BeginRenderPass(commandBuffer, framebuffer, extent);
	}

	void VulkanRenderPass::End(VkCommandBuffer commandBuffer)
	{
		if (m_UseDynamicRendering)
			EndDynamic(commandBuffer);
		else
			vkCmdEndRenderPass(commandBuffer);
	}

	void VulkanRenderPass::BeginRenderPass(VkCommandBuffer commandBuffer, VkFramebuffer framebuffer, VkExtent2D extent)
	{
		VkClearValue clear {};
		clear.color.float32[0] = 0.10f;
		clear.color.float32[1] = 0.12f;
		clear.color.float32[2] = 0.16f;
		clear.color.float32[3] = 1.0f;

		VkRenderPassBeginInfo info {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = m_RenderPass;
		info.framebuffer = framebuffer;
		info.renderArea.extent = extent;
		info.clearValueCount = 1;
		info.pClearValues = &clear;
		vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRenderPass::BeginDynamic(VkCommandBuffer commandBuffer, VkImageView imageView, VkExtent2D extent)
	{
		VkClearValue clear {};
		clear.color.float32[0] = 0.10f;
		clear.color.float32[1] = 0.12f;
		clear.color.float32[2] = 0.16f;
		clear.color.float32[3] = 1.0f;

		VkRenderingAttachmentInfo color {};
		color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color.imageView = imageView;
		color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.clearValue = clear;

		VkRenderingInfo rendering {};
		rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering.renderArea.extent = extent;
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = 1;
		rendering.pColorAttachments = &color;
		vkCmdBeginRendering(commandBuffer, &rendering);
	}

	void VulkanRenderPass::EndDynamic(VkCommandBuffer commandBuffer)
	{
		vkCmdEndRendering(commandBuffer);
	}

}
