#include "Renderer.h"

#include "Lite/Core/Logger.h"

#include <GLFW/glfw3.h>

#include <cstdint>

namespace {

	struct QuadVertex
	{
		float x;
		float y;
		float r;
		float g;
		float b;
	};

	struct RendererState
	{
		Lite::VulkanInstance Instance;
		Lite::VulkanPhysicalDevice PhysicalDevice;
		Lite::VulkanDevice Device;
		Lite::VulkanSwapchain Swapchain;
		Lite::VulkanRenderPass RenderPass;
		Lite::VulkanFramebuffer Framebuffers;
		Lite::VulkanPipeline Pipeline;
		Lite::VulkanBuffer VertexBuffer;
		Lite::VulkanBuffer IndexBuffer;
		Lite::VulkanCommandBuffer Commands;
		Lite::VulkanSync Sync;
		GLFWwindow* Window = nullptr;
		uint32_t CurrentFrame = 0;
		uint32_t ImageIndex = 0;
		uint32_t IndexCount = 0;
		bool FrameActive = false;
		bool FramebufferResized = false;
		bool Ready = false;
	};

	RendererState s_Renderer;

	bool CreateGeometry()
	{
		const QuadVertex vertices[] = {
			{ -0.5f, -0.5f, 0.90f, 0.25f, 0.30f },
			{  0.5f, -0.5f, 0.20f, 0.75f, 0.40f },
			{  0.5f,  0.5f, 0.20f, 0.40f, 0.90f },
			{ -0.5f,  0.5f, 0.95f, 0.80f, 0.25f }
		};
		const uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

		auto device = s_Renderer.Device.Get();
		auto physical = s_Renderer.PhysicalDevice.Get();

		if (!s_Renderer.VertexBuffer.Create(device, physical, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
			return false;
		s_Renderer.VertexBuffer.Upload(vertices, sizeof(vertices));

		if (!s_Renderer.IndexBuffer.Create(device, physical, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
			return false;
		s_Renderer.IndexBuffer.Upload(indices, sizeof(indices));

		s_Renderer.IndexCount = static_cast<uint32_t>(sizeof(indices) / sizeof(uint16_t));
		return true;
	}

	bool RecreateSwapchain()
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(s_Renderer.Window, &width, &height);
		if (width == 0 || height == 0)
			return false;

		s_Renderer.Device.WaitIdle();
		if (!s_Renderer.Swapchain.Recreate())
			return false;

		s_Renderer.Framebuffers.Destroy();
		if (!s_Renderer.Framebuffers.Create(
			s_Renderer.Device.Get(),
			s_Renderer.RenderPass.Get(),
			s_Renderer.Swapchain.GetImageViews(),
			s_Renderer.Swapchain.GetExtent()))
			return false;

		s_Renderer.Sync.ResetImages(s_Renderer.Swapchain.GetImageCount());
		return true;
	}

}

namespace Lite {

	void Renderer::Init(void* window)
	{
		Shutdown();
		s_Renderer.Window = static_cast<GLFWwindow*>(window);

		if (!s_Renderer.Instance.Create())
			return;
		if (!s_Renderer.Swapchain.CreateSurface(s_Renderer.Instance.Get(), s_Renderer.Window))
			return;
		if (!s_Renderer.PhysicalDevice.Pick(s_Renderer.Instance.Get(), s_Renderer.Swapchain.GetSurface()))
			return;
		if (!s_Renderer.Device.Create(
			s_Renderer.PhysicalDevice.Get(),
			s_Renderer.PhysicalDevice.GetQueueFamily(),
			s_Renderer.Instance.ValidationEnabled()))
			return;
		if (!s_Renderer.Swapchain.Create(s_Renderer.Device.Get(), s_Renderer.PhysicalDevice.Get()))
			return;
		if (!s_Renderer.RenderPass.Create(s_Renderer.Device.Get(), s_Renderer.Swapchain.GetFormat()))
			return;
		if (!s_Renderer.Framebuffers.Create(
			s_Renderer.Device.Get(),
			s_Renderer.RenderPass.Get(),
			s_Renderer.Swapchain.GetImageViews(),
			s_Renderer.Swapchain.GetExtent()))
			return;
		if (!s_Renderer.Pipeline.Create(s_Renderer.Device.Get(), s_Renderer.RenderPass.Get()))
			return;
		if (!CreateGeometry())
			return;
		if (!s_Renderer.Commands.Create(s_Renderer.Device.Get(), s_Renderer.Device.GetQueueFamily(), VulkanSync::FramesInFlight))
			return;
		if (!s_Renderer.Sync.Create(s_Renderer.Device.Get(), VulkanSync::FramesInFlight, s_Renderer.Swapchain.GetImageCount()))
			return;

		s_Renderer.Ready = true;
		LITE_INFO("Renderer ready");
	}

	void Renderer::Shutdown()
	{
		if (s_Renderer.Device.Get())
			s_Renderer.Device.WaitIdle();

		s_Renderer.Sync.Destroy();
		s_Renderer.Commands.Destroy();
		s_Renderer.IndexBuffer.Destroy();
		s_Renderer.VertexBuffer.Destroy();
		s_Renderer.Pipeline.Destroy();
		s_Renderer.Framebuffers.Destroy();
		s_Renderer.RenderPass.Destroy();
		s_Renderer.Swapchain.Destroy();
		s_Renderer.Device.Destroy();
		s_Renderer.Swapchain.DestroySurface();
		s_Renderer.PhysicalDevice.Clear();
		s_Renderer.Instance.Destroy();

		s_Renderer.Window = nullptr;
		s_Renderer.CurrentFrame = 0;
		s_Renderer.ImageIndex = 0;
		s_Renderer.IndexCount = 0;
		s_Renderer.FrameActive = false;
		s_Renderer.FramebufferResized = false;
		s_Renderer.Ready = false;
	}

	void Renderer::BeginFrame()
	{
		s_Renderer.FrameActive = false;
		if (!s_Renderer.Ready)
			return;

		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(s_Renderer.Window, &width, &height);
		if (width == 0 || height == 0)
		{
			s_Renderer.FramebufferResized = true;
			return;
		}

		if (s_Renderer.FramebufferResized)
		{
			if (!RecreateSwapchain())
			{
				s_Renderer.FramebufferResized = true;
				return;
			}
			s_Renderer.FramebufferResized = false;
		}

		uint32_t frame = s_Renderer.CurrentFrame;
		if (!s_Renderer.Sync.Wait(frame))
			return;

		VkSemaphore imageAvailable = s_Renderer.Sync.ImageAvailable(frame);
		VkResult acquire = vkAcquireNextImageKHR(
			s_Renderer.Device.Get(),
			s_Renderer.Swapchain.Get(),
			UINT64_MAX,
			imageAvailable,
			VK_NULL_HANDLE,
			&s_Renderer.ImageIndex);

		if (acquire == VK_ERROR_OUT_OF_DATE_KHR)
		{
			s_Renderer.FramebufferResized = true;
			return;
		}

		if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR)
		{
			CheckVk(acquire, "acquire swapchain image");
			return;
		}

		s_Renderer.Sync.WaitImage(s_Renderer.ImageIndex);
		s_Renderer.Sync.TrackImage(s_Renderer.ImageIndex, frame);
		s_Renderer.Sync.Reset(frame);

		VkCommandBuffer commandBuffer = s_Renderer.Commands.Begin(frame);
		s_Renderer.RenderPass.Begin(
			commandBuffer,
			s_Renderer.Framebuffers.Get(s_Renderer.ImageIndex),
			s_Renderer.Swapchain.GetImageView(s_Renderer.ImageIndex),
			s_Renderer.Swapchain.GetExtent());

		s_Renderer.FrameActive = true;
		if (acquire == VK_SUBOPTIMAL_KHR)
			s_Renderer.FramebufferResized = true;
	}

	void Renderer::EndFrame()
	{
		if (!s_Renderer.FrameActive)
			return;

		uint32_t frame = s_Renderer.CurrentFrame;
		VkCommandBuffer commandBuffer = s_Renderer.Commands.Get(frame);
		s_Renderer.RenderPass.End(commandBuffer);
		s_Renderer.FrameActive = false;
		s_Renderer.Commands.End(frame);

		VkSemaphore imageAvailable = s_Renderer.Sync.ImageAvailable(frame);
		VkSemaphore renderFinished = s_Renderer.Sync.RenderFinished(frame);
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailable;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinished;

		if (!CheckVk(vkQueueSubmit(s_Renderer.Device.GetGraphicsQueue(), 1, &submitInfo, s_Renderer.Sync.InFlight(frame)), "submit frame"))
			return;

		VkSwapchainKHR swapchain = s_Renderer.Swapchain.Get();
		VkPresentInfoKHR presentInfo {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &s_Renderer.ImageIndex;

		VkResult present = vkQueuePresentKHR(s_Renderer.Device.GetGraphicsQueue(), &presentInfo);
		if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR || s_Renderer.FramebufferResized)
			s_Renderer.FramebufferResized = true;
		else
			CheckVk(present, "present frame");

		s_Renderer.CurrentFrame = (frame + 1) % VulkanSync::FramesInFlight;
	}

	void Renderer::OnResize(int, int)
	{
		s_Renderer.FramebufferResized = true;
	}

	bool Renderer::IsFrameActive()
	{
		return s_Renderer.FrameActive;
	}

	VkCommandBuffer Renderer::GetCommandBuffer()
	{
		return s_Renderer.Commands.Get(s_Renderer.CurrentFrame);
	}

	VkInstance Renderer::GetInstance()
	{
		return s_Renderer.Instance.Get();
	}

	VkPhysicalDevice Renderer::GetPhysicalDevice()
	{
		return s_Renderer.PhysicalDevice.Get();
	}

	VkDevice Renderer::GetDevice()
	{
		return s_Renderer.Device.Get();
	}

	VkQueue Renderer::GetGraphicsQueue()
	{
		return s_Renderer.Device.GetGraphicsQueue();
	}

	uint32_t Renderer::GetGraphicsQueueFamily()
	{
		return s_Renderer.Device.GetQueueFamily();
	}

	VkRenderPass Renderer::GetRenderPass()
	{
		return s_Renderer.RenderPass.Get();
	}

	uint32_t Renderer::GetImageCount()
	{
		return s_Renderer.Swapchain.GetImageCount();
	}

	uint32_t Renderer::GetMinImageCount()
	{
		return s_Renderer.Swapchain.GetMinImageCount();
	}

	uint32_t Renderer::GetIndexCount()
	{
		return s_Renderer.IndexCount;
	}

	VkExtent2D Renderer::GetExtent()
	{
		return s_Renderer.Swapchain.GetExtent();
	}

	VulkanPipeline& Renderer::GetPipeline()
	{
		return s_Renderer.Pipeline;
	}

	VulkanBuffer& Renderer::GetVertexBuffer()
	{
		return s_Renderer.VertexBuffer;
	}

	VulkanBuffer& Renderer::GetIndexBuffer()
	{
		return s_Renderer.IndexBuffer;
	}

}
