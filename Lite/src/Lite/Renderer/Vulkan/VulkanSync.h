#pragma once

#include "VulkanUtils.h"

#include <cstdint>
#include <vector>

namespace Lite {

	class VulkanSync
	{
	public:
		static constexpr uint32_t FramesInFlight = 2;

		bool Create(VkDevice device, uint32_t frames, uint32_t imageCount);
		void Destroy();
		void ResetImages(uint32_t imageCount);

		bool Wait(uint32_t frame) const;
		void Reset(uint32_t frame) const;
		bool WaitImage(uint32_t imageIndex) const;
		void TrackImage(uint32_t imageIndex, uint32_t frame);

		VkSemaphore ImageAvailable(uint32_t frame) const { return m_ImageAvailable[frame]; }
		VkSemaphore RenderFinished(uint32_t imageIndex) const { return m_RenderFinished[imageIndex]; }
		VkFence InFlight(uint32_t frame) const { return m_InFlight[frame]; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		std::vector<VkSemaphore> m_ImageAvailable;
		std::vector<VkSemaphore> m_RenderFinished;
		std::vector<VkFence> m_InFlight;
		std::vector<VkFence> m_ImageFences;
	};

}
