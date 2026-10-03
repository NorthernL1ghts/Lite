#include <Lite/Renderer/Renderer.h>

#include <Lite/Renderer/Resources/Framebuffer.h>

#include <Lite/Core/Log/Logger.h>
#include <Lite/Core/Profile/Profiler.h>

#include <GLFW/glfw3.h>

#include <cstdint>

namespace {

	struct RendererState
	{
		Lite::VulkanInstance Instance;
		Lite::VulkanPhysicalDevice PhysicalDevice;
		Lite::VulkanDevice Device;
		Lite::VulkanSwapchain Swapchain;
		Lite::VulkanRenderPass RenderPass;
		Lite::Framebuffer Frames;
		Lite::VulkanCommandBuffer Commands;
		Lite::VulkanSync Sync;
		GLFWwindow* Window = nullptr;
		uint32_t CurrentFrame = 0;
		uint32_t ImageIndex = 0;
		bool FrameActive = false;
		bool PassOpen = false;
		bool FramebufferResized = false;
		VkExtent2D DrawExtent {};
		bool ContextReady = false;
		bool Ready = false;
		uint32_t DrawCalls = 0;
		uint32_t QuadCount = 0;
		uint32_t TriangleCount = 0;
		uint32_t IndexCount = 0;
		Lite::Mat4 ViewProjection = Lite::Mat4::Identity();
		Lite::Mat4 Model = Lite::Mat4::Identity();
	};

	RendererState s_Renderer;

	bool PresentRecovered(uint32_t frame, VkFence fence)
	{
		VkSemaphore imageAvailable = s_Renderer.Sync.ImageAvailable(frame);
		VkSemaphore renderFinished = s_Renderer.Sync.RenderFinished(s_Renderer.ImageIndex);
		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.waitSemaphoreCount = 1;
		submit.pWaitSemaphores = &imageAvailable;
		submit.pWaitDstStageMask = &waitStage;
		submit.signalSemaphoreCount = 1;
		submit.pSignalSemaphores = &renderFinished;
		if (!Lite::CheckVk(vkQueueSubmit(s_Renderer.Device.GetGraphicsQueue(), 1, &submit, fence), "recover frame"))
			return false;

		VkSwapchainKHR swapchain = s_Renderer.Swapchain.Get();
		VkPresentInfoKHR presentInfo {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &s_Renderer.ImageIndex;

		VkResult present = vkQueuePresentKHR(s_Renderer.Device.GetGraphicsQueue(), &presentInfo);
		if (present == VK_ERROR_OUT_OF_DATE_KHR || present == VK_SUBOPTIMAL_KHR || !Lite::CheckVk(present, "present recovered frame"))
			s_Renderer.FramebufferResized = true;

		s_Renderer.CurrentFrame = (frame + 1) % Lite::VulkanSync::FramesInFlight;
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

		VkExtent2D extent = s_Renderer.Swapchain.GetExtent();
		if (extent.width == 0 || extent.height == 0)
			return false;

		s_Renderer.Frames.Destroy();
		if (!s_Renderer.Frames.Create(
			s_Renderer.Device.Get(),
			s_Renderer.RenderPass.Get(),
			s_Renderer.Swapchain.GetImageViews(),
			extent))
			return false;

		if (!s_Renderer.Sync.ResetImages(s_Renderer.Swapchain.GetImageCount()))
			return false;

		LITE_INFO("Swapchain resized ({}x{})", extent.width, extent.height);
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

	void Renderer::SetTransform(const Transform& transform)
	{
		s_Renderer.Model = transform.GetMatrix();
	}

	void Renderer::SetModel(const Mat4& model)
	{
		s_Renderer.Model = model;
	}

	const Mat4& Renderer::GetModel()
	{
		return s_Renderer.Model;
	}

	void Renderer::Shutdown()
	{
		if (s_Renderer.Device.Get())
			s_Renderer.Device.WaitIdle();

		s_Renderer.Sync.Destroy();
		s_Renderer.Commands.Destroy();
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
		s_Renderer.PassOpen = false;
		s_Renderer.FramebufferResized = false;
		s_Renderer.DrawExtent = {};
		s_Renderer.ContextReady = false;
		s_Renderer.Ready = false;
		s_Renderer.ViewProjection = Mat4::Identity();
		s_Renderer.Model = Mat4::Identity();
	}

	void Renderer::BeginFrame()
	{
		LITE_PROFILE_SCOPE("Acquire");
		s_Renderer.FrameActive = false;
		s_Renderer.DrawCalls = 0;
		s_Renderer.QuadCount = 0;
		s_Renderer.TriangleCount = 0;
		s_Renderer.IndexCount = 0;
		s_Renderer.Model = Mat4::Identity();
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
			if (!CheckVk(acquire, "acquire swapchain image"))
				return;
		}

		s_Renderer.Sync.WaitImage(s_Renderer.ImageIndex);
		s_Renderer.Sync.TrackImage(s_Renderer.ImageIndex, frame);
		if (!s_Renderer.Sync.Reset(frame))
		{
			PresentRecovered(frame, VK_NULL_HANDLE);
			return;
		}

		VkCommandBuffer commandBuffer = s_Renderer.Commands.Begin(frame);
		if (!commandBuffer)
		{
			PresentRecovered(frame, s_Renderer.Sync.InFlight(frame));
			return;
		}

		s_Renderer.FrameActive = true;
		s_Renderer.PassOpen = false;
		s_Renderer.DrawExtent = {};
		if (acquire == VK_SUBOPTIMAL_KHR)
			s_Renderer.FramebufferResized = true;
	}

	void Renderer::EndPass()
	{
		if (!s_Renderer.PassOpen)
			return;

		s_Renderer.RenderPass.End(s_Renderer.Commands.Get(s_Renderer.CurrentFrame));
		s_Renderer.PassOpen = false;
		s_Renderer.DrawExtent = {};
	}

	bool Renderer::BeginSwapchain()
	{
		if (!s_Renderer.FrameActive || s_Renderer.PassOpen)
			return s_Renderer.PassOpen;

		s_Renderer.DrawExtent = {};
		s_Renderer.RenderPass.Begin(
			s_Renderer.Commands.Get(s_Renderer.CurrentFrame),
			s_Renderer.Frames.Get(s_Renderer.ImageIndex),
			s_Renderer.Swapchain.GetImageView(s_Renderer.ImageIndex),
			s_Renderer.Swapchain.GetExtent());
		s_Renderer.PassOpen = true;
		return true;
	}

	bool Renderer::IsPassOpen()
	{
		return s_Renderer.PassOpen;
	}

	void Renderer::SetDrawExtent(VkExtent2D extent)
	{
		s_Renderer.DrawExtent = extent;
	}

	void Renderer::EndFrame()
	{
		LITE_PROFILE_SCOPE("Submit");
		if (!s_Renderer.FrameActive)
			return;

		EndPass();
		uint32_t frame = s_Renderer.CurrentFrame;
		VkCommandBuffer commandBuffer = s_Renderer.Commands.Get(frame);
		s_Renderer.FrameActive = false;
		if (!s_Renderer.Commands.End(frame))
		{
			PresentRecovered(frame, s_Renderer.Sync.InFlight(frame));
			return;
		}

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
		else if (!CheckVk(present, "present frame"))
			s_Renderer.FramebufferResized = true;

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

	void Renderer::RecordDraw(uint32_t indexCount)
	{
		uint32_t triangles = indexCount / 3u;
		s_Renderer.DrawCalls += 1;
		s_Renderer.TriangleCount += triangles;
		s_Renderer.QuadCount += triangles / 2u;
		s_Renderer.IndexCount += indexCount;
	}

	uint32_t Renderer::GetDrawCalls()
	{
		return s_Renderer.DrawCalls;
	}

	uint32_t Renderer::GetQuadCount()
	{
		return s_Renderer.QuadCount;
	}

	uint32_t Renderer::GetTriangleCount()
	{
		return s_Renderer.TriangleCount;
	}

	uint32_t Renderer::GetIndexCount()
	{
		return s_Renderer.IndexCount;
	}

	VkExtent2D Renderer::GetExtent()
	{
		if (s_Renderer.DrawExtent.width > 0 && s_Renderer.DrawExtent.height > 0)
			return s_Renderer.DrawExtent;
		return s_Renderer.Swapchain.GetExtent();
	}

	VkFormat Renderer::GetSwapchainFormat()
	{
		return s_Renderer.Swapchain.GetFormat();
	}

	VkColorSpaceKHR Renderer::GetColorSpace()
	{
		return s_Renderer.Swapchain.GetColorSpace();
	}

	VkPresentModeKHR Renderer::GetPresentMode()
	{
		return s_Renderer.Swapchain.GetPresentMode();
	}

	uint32_t Renderer::GetImageIndex()
	{
		return s_Renderer.ImageIndex;
	}

	uint32_t Renderer::GetFrameIndex()
	{
		return s_Renderer.CurrentFrame;
	}

}
