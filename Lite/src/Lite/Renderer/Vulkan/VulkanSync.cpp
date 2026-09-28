#include <Lite/Renderer/Vulkan/VulkanSync.h>

namespace Lite {

	bool VulkanSync::Create(VkDevice device, uint32_t frames, uint32_t imageCount)
	{
		m_Device = device;
		m_ImageAvailable.resize(frames);
		m_RenderFinished.resize(imageCount);
		m_InFlight.resize(frames);
		m_ImageFences.assign(imageCount, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t index = 0; index < frames; ++index)
		{
			if (!CheckVk(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailable[index]), "create image semaphore"))
				return false;
			if (!CheckVk(vkCreateFence(device, &fenceInfo, nullptr, &m_InFlight[index]), "create frame fence"))
				return false;
		}

		for (uint32_t index = 0; index < imageCount; ++index)
		{
			if (!CheckVk(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinished[index]), "create render semaphore"))
				return false;
		}

		return true;
	}

	void VulkanSync::Destroy()
	{
		for (auto semaphore : m_ImageAvailable)
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		for (auto semaphore : m_RenderFinished)
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		for (auto fence : m_InFlight)
			vkDestroyFence(m_Device, fence, nullptr);

		m_ImageAvailable.clear();
		m_RenderFinished.clear();
		m_InFlight.clear();
		m_ImageFences.clear();
	}

	bool VulkanSync::ResetImages(uint32_t imageCount)
	{
		m_ImageFences.assign(imageCount, VK_NULL_HANDLE);

		for (auto semaphore : m_RenderFinished)
			vkDestroySemaphore(m_Device, semaphore, nullptr);

		m_RenderFinished.assign(imageCount, VK_NULL_HANDLE);
		VkSemaphoreCreateInfo semaphoreInfo {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (uint32_t index = 0; index < imageCount; ++index)
		{
			if (!CheckVk(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinished[index]), "recreate render semaphore"))
				return false;
		}

		return true;
	}

	bool VulkanSync::Wait(uint32_t frame) const
	{
		return CheckVk(vkWaitForFences(m_Device, 1, &m_InFlight[frame], VK_TRUE, UINT64_MAX), "wait for frame fence");
	}

	bool VulkanSync::Reset(uint32_t frame) const
	{
		return CheckVk(vkResetFences(m_Device, 1, &m_InFlight[frame]), "reset frame fence");
	}

	bool VulkanSync::WaitImage(uint32_t imageIndex) const
	{
		if (m_ImageFences[imageIndex] == VK_NULL_HANDLE)
			return true;

		return CheckVk(vkWaitForFences(m_Device, 1, &m_ImageFences[imageIndex], VK_TRUE, UINT64_MAX), "wait for image fence");
	}

	void VulkanSync::TrackImage(uint32_t imageIndex, uint32_t frame)
	{
		m_ImageFences[imageIndex] = m_InFlight[frame];
	}

}
