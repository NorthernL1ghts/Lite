#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Renderer/Vulkan/VulkanFramebuffer.h>

#include <cstdint>
#include <vector>

namespace Lite {

	class LITE_API Framebuffer
	{
	public:
		bool Create(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& views, VkExtent2D extent);
		void Destroy();

		VkFramebuffer Get(uint32_t index) const { return m_Frames.Get(index); }

	private:
		VulkanFramebuffer m_Frames;
	};

}
