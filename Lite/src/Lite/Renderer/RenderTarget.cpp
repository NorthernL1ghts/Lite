#include <Lite/Renderer/RenderTarget.h>

#include <Lite/Core/Log/Logger.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Vulkan/VulkanUtils.h>

#include <imgui_impl_vulkan.h>

namespace Lite {

	RenderTarget::~RenderTarget()
	{
		if (!m_Device)
			return;

		vkDeviceWaitIdle(m_Device);
		for (Slot& slot : m_Slots)
			DestroySlot(slot);
		DestroyPass();
	}

	bool RenderTarget::Ensure(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || !Renderer::GetDevice())
			return false;

		const VkFormat format = Renderer::GetSwapchainFormat();
		if (format == VK_FORMAT_UNDEFINED)
			return false;

		if (m_Pass != VK_NULL_HANDLE && m_Format != format)
		{
			vkDeviceWaitIdle(m_Device);
			for (Slot& slot : m_Slots)
				DestroySlot(slot);
			DestroyPass();
		}

		if (m_Pass == VK_NULL_HANDLE && !CreatePass(format))
			return false;

		const uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= m_Slots.size())
			return false;

		Slot& slot = m_Slots[frame];
		if (slot.Image != VK_NULL_HANDLE && slot.Width == width && slot.Height == height)
			return true;

		DestroySlot(slot);
		if (!CreateSlot(slot, width, height))
			return false;

		LITE_INFO("Viewport target resized ({}x{})", width, height);
		return true;
	}

	bool RenderTarget::Begin()
	{
		const uint32_t frame = Renderer::GetFrameIndex();
		if (m_Pass == VK_NULL_HANDLE || frame >= m_Slots.size())
			return false;

		const Slot& slot = m_Slots[frame];
		if (slot.Framebuffer == VK_NULL_HANDLE)
			return false;

		const VkExtent2D extent { slot.Width, slot.Height };
		Renderer::SetDrawExtent(extent);

		VkClearValue clear {};
		clear.color.float32[0] = 0.10f;
		clear.color.float32[1] = 0.12f;
		clear.color.float32[2] = 0.16f;
		clear.color.float32[3] = 1.0f;

		VkRenderPassBeginInfo info {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = m_Pass;
		info.framebuffer = slot.Framebuffer;
		info.renderArea.extent = extent;
		info.clearValueCount = 1;
		info.pClearValues = &clear;
		vkCmdBeginRenderPass(Renderer::GetCommandBuffer(), &info, VK_SUBPASS_CONTENTS_INLINE);
		return true;
	}

	void RenderTarget::End()
	{
		const uint32_t frame = Renderer::GetFrameIndex();
		if (m_Pass == VK_NULL_HANDLE || frame >= m_Slots.size() || m_Slots[frame].Framebuffer == VK_NULL_HANDLE)
			return;

		VkCommandBuffer command = Renderer::GetCommandBuffer();
		vkCmdEndRenderPass(command);

		VkImageMemoryBarrier barrier = ImageBarrier(
			m_Slots[frame].Image,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT);
		vkCmdPipelineBarrier(
			command,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		Renderer::SetDrawExtent({});
	}

	VkDescriptorSet RenderTarget::GetTexture() const
	{
		const uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= m_Slots.size())
			return VK_NULL_HANDLE;
		return m_Slots[frame].Texture;
	}

	uint32_t RenderTarget::GetWidth() const
	{
		const uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= m_Slots.size())
			return 0;
		return m_Slots[frame].Width;
	}

	uint32_t RenderTarget::GetHeight() const
	{
		const uint32_t frame = Renderer::GetFrameIndex();
		if (frame >= m_Slots.size())
			return 0;
		return m_Slots[frame].Height;
	}

	void RenderTarget::DestroySlot(Slot& slot)
	{
		if (slot.Texture != VK_NULL_HANDLE)
			ImGui_ImplVulkan_RemoveTexture(slot.Texture);
		if (m_Device != VK_NULL_HANDLE)
		{
			if (slot.Framebuffer != VK_NULL_HANDLE)
				vkDestroyFramebuffer(m_Device, slot.Framebuffer, nullptr);
			if (slot.View != VK_NULL_HANDLE)
				vkDestroyImageView(m_Device, slot.View, nullptr);
			if (slot.Image != VK_NULL_HANDLE)
				vkDestroyImage(m_Device, slot.Image, nullptr);
			if (slot.Memory != VK_NULL_HANDLE)
				vkFreeMemory(m_Device, slot.Memory, nullptr);
		}

		slot = {};
	}

	void RenderTarget::DestroyPass()
	{
		if (m_Pass != VK_NULL_HANDLE && m_Device != VK_NULL_HANDLE)
			vkDestroyRenderPass(m_Device, m_Pass, nullptr);

		m_Pass = VK_NULL_HANDLE;
		m_Format = VK_FORMAT_UNDEFINED;
	}

	bool RenderTarget::CreatePass(VkFormat format)
	{
		m_Device = Renderer::GetDevice();
		m_Format = format;

		VkAttachmentDescription color {};
		color.format = format;
		color.samples = VK_SAMPLE_COUNT_1_BIT;
		color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
		if (!CheckVk(vkCreateRenderPass(m_Device, &passInfo, nullptr, &m_Pass), "create viewport render pass"))
		{
			m_Pass = VK_NULL_HANDLE;
			m_Device = VK_NULL_HANDLE;
			return false;
		}

		return true;
	}

	bool RenderTarget::CreateSlot(Slot& slot, uint32_t width, uint32_t height)
	{
		VkImageCreateInfo imageInfo {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = m_Format;
		imageInfo.extent = { width, height, 1 };
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		slot.Image = VK_NULL_HANDLE;
		if (!CheckVk(vkCreateImage(m_Device, &imageInfo, nullptr, &slot.Image), "create viewport image"))
		{
			slot.Image = VK_NULL_HANDLE;
			return false;
		}

		VkMemoryRequirements requirements {};
		vkGetImageMemoryRequirements(m_Device, slot.Image, &requirements);
		slot.Memory = VK_NULL_HANDLE;
		if (!AllocateMemory(m_Device, Renderer::GetPhysicalDevice(), requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, slot.Memory, "allocate viewport image"))
		{
			slot.Memory = VK_NULL_HANDLE;
			DestroySlot(slot);
			return false;
		}
		if (!CheckVk(vkBindImageMemory(m_Device, slot.Image, slot.Memory, 0), "bind viewport image"))
		{
			DestroySlot(slot);
			return false;
		}

		VkImageViewCreateInfo viewInfo {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = slot.Image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_Format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.layerCount = 1;
		if (!CheckVk(vkCreateImageView(m_Device, &viewInfo, nullptr, &slot.View), "create viewport view"))
		{
			DestroySlot(slot);
			return false;
		}

		VkFramebufferCreateInfo framebufferInfo {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_Pass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &slot.View;
		framebufferInfo.width = width;
		framebufferInfo.height = height;
		framebufferInfo.layers = 1;
		if (!CheckVk(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &slot.Framebuffer), "create viewport framebuffer"))
		{
			DestroySlot(slot);
			return false;
		}

		slot.Texture = ImGui_ImplVulkan_AddTexture(slot.View, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (slot.Texture == VK_NULL_HANDLE)
		{
			DestroySlot(slot);
			return false;
		}

		slot.Width = width;
		slot.Height = height;
		return true;
	}

}
