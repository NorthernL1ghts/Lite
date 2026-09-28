#pragma once

#include <Lite/Renderer/Vulkan/VulkanUtils.h>

#include <cstdint>
#include <vector>

namespace Lite {

	class VulkanFramebuffer
	{
	public:
		bool Create(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& views, VkExtent2D extent);
		void Destroy();

		VkFramebuffer Get(uint32_t index) const { return m_Framebuffers[index]; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> m_Framebuffers;
	};

}
