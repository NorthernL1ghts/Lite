#include "VulkanFramebuffer.h"

namespace Lite {

	bool VulkanFramebuffer::Create(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& views, VkExtent2D extent)
	{
		Destroy();
		m_Device = device;
		m_Framebuffers.resize(views.size());

		for (size_t index = 0; index < views.size(); ++index)
		{
			VkImageView attachment = views[index];
			VkFramebufferCreateInfo info {};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass = renderPass;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.width = extent.width;
			info.height = extent.height;
			info.layers = 1;

			if (!CheckVk(vkCreateFramebuffer(device, &info, nullptr, &m_Framebuffers[index]), "create framebuffer"))
				return false;
		}

		return true;
	}

	void VulkanFramebuffer::Destroy()
	{
		for (auto framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

		m_Framebuffers.clear();
	}

}
