#include "ImGuiLayer.h"

#include "Lite/Core/Events/Event.h"
#include "Lite/Core/Logger.h"
#include "Lite/Renderer/Renderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Lite {

	ImGuiLayer::ImGuiLayer(GLFWwindow* window)
		: Layer("ImGui")
		, m_Window(window)
	{
	}

	ImGuiLayer::~ImGuiLayer() = default;

	void CheckImGuiVulkan(VkResult result)
	{
		if (result != VK_SUCCESS)
			LITE_ERROR("ImGui Vulkan call failed ({})", static_cast<int>(result));
	}

	void ImGuiLayer::OnAttach()
	{
		if (!Renderer::GetDevice())
		{
			LITE_ERROR("ImGui was not started because Vulkan is not ready");
			return;
		}

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(m_Window, true);

		ImGui_ImplVulkan_InitInfo info {};
		info.ApiVersion = VK_API_VERSION_1_3;
		info.Instance = Renderer::GetInstance();
		info.PhysicalDevice = Renderer::GetPhysicalDevice();
		info.Device = Renderer::GetDevice();
		info.QueueFamily = Renderer::GetGraphicsQueueFamily();
		info.Queue = Renderer::GetGraphicsQueue();
		info.DescriptorPoolSize = 128;
		info.MinImageCount = Renderer::GetMinImageCount();
		info.ImageCount = Renderer::GetImageCount();
		info.PipelineInfoMain.RenderPass = Renderer::GetRenderPass();
		info.PipelineInfoMain.Subpass = 0;
		info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		info.MinAllocationSize = 1024 * 1024;
		info.CheckVkResultFn = CheckImGuiVulkan;

		if (!ImGui_ImplVulkan_Init(&info))
		{
			LITE_ERROR("Failed to start the ImGui Vulkan backend");
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			return;
		}

		m_Ready = true;
	}

	void ImGuiLayer::OnDetach()
	{
		if (!m_Ready)
			return;

		vkDeviceWaitIdle(Renderer::GetDevice());
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		m_Ready = false;
	}

	void ImGuiLayer::OnEvent(Event& event)
	{
		const ImGuiIO& io = ImGui::GetIO();

		if (io.WantCaptureMouse && event.IsInCategory(EventCategory::Mouse))
			event.Handled = true;

		if (io.WantCaptureKeyboard && event.IsInCategory(EventCategory::Keyboard))
			event.Handled = true;
	}

	void ImGuiLayer::Begin()
	{
		if (!m_Ready)
			return;

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBackground;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceId = ImGui::GetID("LiteEditor");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

		if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr)
		{
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGuiDockNodeFlags nodeFlags = static_cast<ImGuiDockNodeFlags>(
				static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode) | static_cast<int>(ImGuiDockNodeFlags_DockSpace));
			ImGui::DockBuilderAddNode(dockspaceId, nodeFlags);
			ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

			ImGuiID center = dockspaceId;
			ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
			ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22f, nullptr, &center);

			ImGui::DockBuilderDockWindow("Renderer", right);
			ImGui::DockBuilderDockWindow("Stats", bottom);
			ImGui::DockBuilderFinish(dockspaceId);
		}

		ImGui::End();
	}

	void ImGuiLayer::OnImGuiRender()
	{
		if (!m_Ready)
			return;

		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties(Renderer::GetPhysicalDevice(), &properties);
		VkExtent2D extent = Renderer::GetExtent();

		auto version = [](uint32_t value)
		{
			return std::format("{}.{}.{}", VK_VERSION_MAJOR(value), VK_VERSION_MINOR(value), VK_VERSION_PATCH(value));
		};

		const char* deviceType = "Other";
		switch (properties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: deviceType = "Discrete GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceType = "Integrated GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: deviceType = "Virtual GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: deviceType = "CPU"; break;
			default: break;
		}

		ImGui::Begin("Renderer");
		ImGui::SeparatorText("GPU");
		ImGui::LabelText("Name", "%s", properties.deviceName);
		ImGui::LabelText("Type", "%s", deviceType);
		ImGui::LabelText("API", "%s", version(properties.apiVersion).c_str());
		ImGui::LabelText("Driver", "%s", version(properties.driverVersion).c_str());
		ImGui::LabelText("Max texture", "%u", properties.limits.maxImageDimension2D);

		ImGui::SeparatorText("Swapchain");
		ImGui::LabelText("Extent", "%u x %u", extent.width, extent.height);
		ImGui::LabelText("Images", "%u", Renderer::GetImageCount());
		ImGui::LabelText("Min images", "%u", Renderer::GetMinImageCount());
		ImGui::LabelText("Frames in flight", "%u", VulkanSync::FramesInFlight);
		ImGui::LabelText("Queue family", "%u", Renderer::GetGraphicsQueueFamily());

		ImGui::SeparatorText("Draw");
		ImGui::LabelText("Frame active", "%s", Renderer::IsFrameActive() ? "yes" : "no");
		ImGui::LabelText("Index count", "%u", Renderer::GetIndexCount());
		ImGui::End();

		const ImGuiIO& io = ImGui::GetIO();
		ImGui::Begin("Stats");
		ImGui::LabelText("FPS", "%.1f", io.Framerate);
		ImGui::LabelText("Frame time", "%.3f ms", io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f);
		ImGui::End();
	}

	void ImGuiLayer::End()
	{
		if (!m_Ready)
			return;

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Renderer::GetCommandBuffer());
	}

}
