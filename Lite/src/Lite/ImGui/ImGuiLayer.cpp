#include <Lite/ImGui/ImGuiLayer.h>

#include <Lite/Core/Events/Event.h>
#include <Lite/Core/Log/Logger.h>
#include <Lite/Renderer/Renderer.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace Lite {

	namespace {

		const ImVec4 kAccent { 0.36f, 0.56f, 0.86f, 1.0f };

		void ApplyStyle()
		{
			ImGui::StyleColorsDark();
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 8.0f;
			style.ChildRounding = 6.0f;
			style.FrameRounding = 5.0f;
			style.PopupRounding = 6.0f;
			style.ScrollbarRounding = 6.0f;
			style.GrabRounding = 5.0f;
			style.TabRounding = 6.0f;
			style.WindowPadding = ImVec2(14.0f, 12.0f);
			style.FramePadding = ImVec2(12.0f, 7.0f);
			style.ItemSpacing = ImVec2(10.0f, 7.0f);
			style.ItemInnerSpacing = ImVec2(8.0f, 4.0f);
			style.CellPadding = ImVec2(8.0f, 6.0f);
			style.WindowBorderSize = 0.0f;
			style.FrameBorderSize = 0.0f;
			style.TabBorderSize = 0.0f;
			style.TabBarBorderSize = 0.0f;
			style.TabBarOverlineSize = 2.0f;
			style.DockingSeparatorSize = 1.0f;

			ImVec4* colors = style.Colors;
			colors[ImGuiCol_Text] = ImVec4(0.93f, 0.94f, 0.96f, 1.0f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.15f, 0.96f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.12f, 0.15f, 0.0f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.13f, 0.16f, 0.98f);
			colors[ImGuiCol_Border] = ImVec4(0.24f, 0.28f, 0.34f, 0.70f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.21f, 1.0f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.32f, 0.42f, 1.0f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.13f, 0.17f, 1.0f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.10f, 0.12f, 1.0f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.28f, 0.32f, 0.40f, 1.0f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.42f, 0.52f, 1.0f);
			colors[ImGuiCol_ScrollbarGrabActive] = kAccent;
			colors[ImGuiCol_Header] = ImVec4(0.22f, 0.32f, 0.48f, 1.0f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.42f, 0.64f, 1.0f);
			colors[ImGuiCol_HeaderActive] = kAccent;
			colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.20f, 1.0f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.38f, 0.58f, 1.0f);
			colors[ImGuiCol_TabSelected] = kAccent;
			colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.78f, 0.88f, 1.0f, 1.0f);
			colors[ImGuiCol_TabDimmed] = ImVec4(0.13f, 0.14f, 0.17f, 1.0f);
			colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.22f, 0.30f, 0.42f, 1.0f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.17f, 0.21f, 1.0f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.035f);
			colors[ImGuiCol_TableBorderLight] = ImVec4(0.22f, 0.26f, 0.32f, 0.55f);
			colors[ImGuiCol_Separator] = ImVec4(0.24f, 0.28f, 0.34f, 1.0f);
			colors[ImGuiCol_DockingPreview] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.45f);
		}

	}

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

		ApplyStyle();

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
		if (!m_Ready || event.Handled)
			return;

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
	}

	void ImGuiLayer::End()
	{
		if (!m_Ready)
			return;

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), Renderer::GetCommandBuffer());
	}

}
