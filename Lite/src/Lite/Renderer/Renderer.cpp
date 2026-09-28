#include "Renderer.h"

#include "Framebuffer.h"
#include "VertexArray.h"

#include "Lite/Core/Logger.h"

#include <GLFW/glfw3.h>

#include <cstdint>

namespace {

	struct TriangleVertex
	{
		float x;
		float y;
		float baryX;
		float baryY;
		float baryZ;
	};

	struct RendererState
	{
		Lite::VulkanInstance Instance;
		Lite::VulkanPhysicalDevice PhysicalDevice;
		Lite::VulkanDevice Device;
		Lite::VulkanSwapchain Swapchain;
		Lite::VulkanRenderPass RenderPass;
		Lite::Framebuffer Frames;
		Lite::VertexArray Geometry;
		Lite::VulkanCommandBuffer Commands;
		Lite::VulkanSync Sync;
		GLFWwindow* Window = nullptr;
		uint32_t CurrentFrame = 0;
		uint32_t ImageIndex = 0;
		bool FrameActive = false;
		bool FramebufferResized = false;
		bool ContextReady = false;
		bool Ready = false;
		Lite::Mat4 ViewProjection = Lite::Mat4::Identity();
	};

	RendererState s_Renderer;

	bool CreateGeometry()
	{
		const TriangleVertex vertices[] = {
			{  0.00f, -0.72f, 1.0f, 0.0f, 0.0f },
			{ -0.78f,  0.58f, 0.0f, 1.0f, 0.0f },
			{  0.78f,  0.58f, 0.0f, 0.0f, 1.0f }
		};
		const uint16_t indices[] = { 0, 1, 2 };

		return s_Renderer.Geometry.Create(vertices, sizeof(vertices), indices, static_cast<uint32_t>(sizeof(indices) / sizeof(uint16_t)));
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

		s_Renderer.Frames.Destroy();
		if (!s_Renderer.Frames.Create(
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
		if (!s_Renderer.Frames.Create(
			s_Renderer.Device.Get(),
			s_Renderer.RenderPass.Get(),
			s_Renderer.Swapchain.GetImageViews(),
			s_Renderer.Swapchain.GetExtent()))
			return;
		if (!CreateGeometry())
			return;
		if (!s_Renderer.Commands.Create(s_Renderer.Device.Get(), s_Renderer.Device.GetQueueFamily(), VulkanSync::FramesInFlight))
			return;
		if (!s_Renderer.Sync.Create(s_Renderer.Device.Get(), VulkanSync::FramesInFlight, s_Renderer.Swapchain.GetImageCount()))
			return;

		s_Renderer.ContextReady = true;
		s_Renderer.Ready = true;
		LITE_INFO("Renderer context ready");
	}

	void Renderer::SetViewProjection(const Mat4& viewProjection)
	{
		s_Renderer.ViewProjection = viewProjection;
	}

	const Mat4& Renderer::GetViewProjection()
	{
		return s_Renderer.ViewProjection;
	}

	void Renderer::Shutdown()
	{
		if (s_Renderer.Device.Get())
			s_Renderer.Device.WaitIdle();

		s_Renderer.Sync.Destroy();
		s_Renderer.Commands.Destroy();
		s_Renderer.Geometry.Destroy();
		s_Renderer.Frames.Destroy();
		s_Renderer.RenderPass.Destroy();
		s_Renderer.Swapchain.Destroy();
		s_Renderer.Device.Destroy();
		s_Renderer.Swapchain.DestroySurface();
		s_Renderer.PhysicalDevice.Clear();
		s_Renderer.Instance.Destroy();

		s_Renderer.Window = nullptr;
		s_Renderer.CurrentFrame = 0;
		s_Renderer.ImageIndex = 0;
		s_Renderer.FrameActive = false;
		s_Renderer.FramebufferResized = false;
		s_Renderer.ContextReady = false;
		s_Renderer.Ready = false;
		s_Renderer.ViewProjection = Mat4::Identity();
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
			s_Renderer.Frames.Get(s_Renderer.ImageIndex),
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
		VkSemaphore renderFinished = s_Renderer.Sync.RenderFinished(s_Renderer.ImageIndex);
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
		return s_Renderer.Geometry.GetIndexCount();
	}

	VkExtent2D Renderer::GetExtent()
	{
		return s_Renderer.Swapchain.GetExtent();
	}

	uint32_t Renderer::GetFrameIndex()
	{
		return s_Renderer.CurrentFrame;
	}

	VertexArray& Renderer::GetVertexArray()
	{
		return s_Renderer.Geometry;
	}

}
