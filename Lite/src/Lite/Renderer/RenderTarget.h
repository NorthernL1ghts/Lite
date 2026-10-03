#pragma once

#include <Lite/Core/Base.h>
#include <Lite/Renderer/Vulkan/VulkanSync.h>

#include <array>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Lite {

	class LITE_API RenderTarget
	{
	public:
		RenderTarget() = default;
		~RenderTarget();

		RenderTarget(const RenderTarget&) = delete;
		RenderTarget& operator=(const RenderTarget&) = delete;

		bool Ensure(uint32_t width, uint32_t height);
		bool Begin();
		void End();

		VkDescriptorSet GetTexture() const;
		uint32_t GetWidth() const;
		uint32_t GetHeight() const;

	private:
		struct Slot
		{
			VkImage Image = VK_NULL_HANDLE;
			VkDeviceMemory Memory = VK_NULL_HANDLE;
			VkImageView View = VK_NULL_HANDLE;
			VkFramebuffer Framebuffer = VK_NULL_HANDLE;
			VkDescriptorSet Texture = VK_NULL_HANDLE;
			uint32_t Width = 0;
			uint32_t Height = 0;
		};

		void DestroySlot(Slot& slot);
		void DestroyPass();
		bool CreatePass(VkFormat format);
		bool CreateSlot(Slot& slot, uint32_t width, uint32_t height);

		std::array<Slot, VulkanSync::FramesInFlight> m_Slots {};
		VkRenderPass m_Pass = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
	};

}
