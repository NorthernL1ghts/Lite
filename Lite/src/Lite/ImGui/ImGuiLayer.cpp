#include "ImGuiLayer.h"

#include "Lite/Core/Events/Event.h"
#include "Lite/Core/Logger.h"
#include "Lite/Platform/Vulkan/VulkanContext.h"
#include "Lite/Renderer/Renderer2D.h"

#include <imgui.h>
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
		auto& vulkan = Renderer2D::GetVulkanContext();
		if (!vulkan.GetDevice())
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
		info.Instance = vulkan.GetInstance();
		info.PhysicalDevice = vulkan.GetPhysicalDevice();
		info.Device = vulkan.GetDevice();
		info.QueueFamily = vulkan.GetGraphicsQueueFamily();
		info.Queue = vulkan.GetGraphicsQueue();
		info.DescriptorPoolSize = 128;
		info.MinImageCount = vulkan.GetMinImageCount();
		info.ImageCount = vulkan.GetImageCount();
		info.PipelineInfoMain.RenderPass = vulkan.GetRenderPass();
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

		vkDeviceWaitIdle(Renderer2D::GetVulkanContext().GetDevice());
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
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		ImGui::DockSpace(ImGui::GetID("LiteDockSpace"));
		ImGui::End();
	}

	void ImGuiLayer::End()
	{
		if (!m_Ready)
			return;

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Renderer2D::GetVulkanContext().GetCommandBuffer());
	}

}
