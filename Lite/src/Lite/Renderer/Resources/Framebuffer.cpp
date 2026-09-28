#include <Lite/Renderer/Resources/Framebuffer.h>

namespace Lite {

	bool Framebuffer::Create(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView>& views, VkExtent2D extent)
	{
		return m_Frames.Create(device, renderPass, views, extent);
	}

	void Framebuffer::Destroy()
	{
		m_Frames.Destroy();
	}

}
