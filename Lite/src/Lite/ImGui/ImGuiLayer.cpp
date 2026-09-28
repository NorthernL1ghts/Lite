#include "ImGuiLayer.h"

#include "Lite/Core/Events/Event.h"
#include "Lite/Core/Events/KeyEvent.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/Profiler.h"
#include "Lite/Core/Time.h"
#include "Lite/Input/KeyCodes.h"
#include "Lite/Renderer/Renderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <format>
#include <string>
#include <string_view>

namespace Lite {

	namespace {

		const ImVec4 kLabelColor { 0.62f, 0.68f, 0.76f, 1.0f };
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

		bool BeginRows(const char* id)
		{
			if (!ImGui::BeginTable(id, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingStretchProp))
				return false;

			ImGui::TableSetupColumn("Field", ImGuiTableColumnFlags_WidthStretch, 0.46f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.54f);
			return true;
		}

		void Row(const char* label, std::string_view value)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::PushStyleColor(ImGuiCol_Text, kLabelColor);
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(value.data(), value.data() + value.size());
		}

		const char* FormatName(VkFormat format)
		{
			switch (format)
			{
			case VK_FORMAT_B8G8R8A8_SRGB: return "B8G8R8A8 sRGB";
			case VK_FORMAT_R8G8B8A8_SRGB: return "R8G8B8A8 sRGB";
			case VK_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8 UNORM";
			case VK_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8 UNORM";
			default: return "Other";
			}
		}

		const char* ColorSpaceName(VkColorSpaceKHR space)
		{
			switch (space)
			{
			case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "sRGB nonlinear";
			default: return "Other";
			}
		}

		const char* PresentModeName(VkPresentModeKHR mode)
		{
			switch (mode)
			{
			case VK_PRESENT_MODE_MAILBOX_KHR: return "Mailbox";
			case VK_PRESENT_MODE_FIFO_KHR: return "FIFO";
			case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "FIFO relaxed";
			case VK_PRESENT_MODE_IMMEDIATE_KHR: return "Immediate";
			default: return "Other";
			}
		}

		std::string DeviceMemory()
		{
			VkPhysicalDeviceMemoryProperties memory {};
			vkGetPhysicalDeviceMemoryProperties(Renderer::GetPhysicalDevice(), &memory);

			VkDeviceSize local = 0;
			for (uint32_t index = 0; index < memory.memoryHeapCount; ++index)
			{
				if (memory.memoryHeaps[index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
					local += memory.memoryHeaps[index].size;
			}

			double gigabytes = static_cast<double>(local) / (1024.0 * 1024.0 * 1024.0);
			return std::format("{:.1f} GB", gigabytes);
		}

		bool InfoDockNeedsBuild(ImGuiID dockspaceId)
		{
			ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
			if (node == nullptr || !node->IsSplitNode())
				return true;

			const char* names[] = { "GPU", "Swapchain", "Draw", "Profile" };
			for (const char* name : names)
			{
				ImGuiWindow* window = ImGui::FindWindowByName(name);
				if (window == nullptr || window->DockNode == nullptr)
					continue;

				ImGuiDockNode* root = ImGui::DockNodeGetRootNode(window->DockNode);
				if (root == nullptr || root->ID != dockspaceId)
					return true;
			}

			return false;
		}

		void RetireOldDockspaces()
		{
			const char* retired[] = { "LiteInfo", "LiteInfoTabs", "LiteEditorSides" };
			for (const char* name : retired)
				ImGui::DockBuilderRemoveNode(ImGui::GetID(name));
		}

		void BuildInfoDock(ImGuiID dockspaceId, ImVec2 size)
		{
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGuiDockNodeFlags nodeFlags = static_cast<ImGuiDockNodeFlags>(
				static_cast<int>(ImGuiDockNodeFlags_DockSpace) | static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode));
			ImGui::DockBuilderAddNode(dockspaceId, nodeFlags);
			ImGui::DockBuilderSetNodeSize(dockspaceId, size);

			ImGuiID center = dockspaceId;
			ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.28f, nullptr, &center);
			ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.30f, nullptr, &center);
			ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.24f, nullptr, &center);
			ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.48f, nullptr, &left);

			ImGui::DockBuilderDockWindow("GPU", left);
			ImGui::DockBuilderDockWindow("Swapchain", leftBottom);
			ImGui::DockBuilderDockWindow("Draw", right);
			ImGui::DockBuilderDockWindow("Profile", bottom);
			ImGui::DockBuilderFinish(dockspaceId);
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
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& key)
		{
			if (key.GetKeyCode() != Key::I || key.IsRepeat())
				return false;

			m_ShowInfo = !m_ShowInfo;
			LITE_INFO("Information view {}", m_ShowInfo ? "shown" : "hidden");
			return true;
		});

		if (event.Handled)
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

		if (!m_ShowInfo)
			return;

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

		ImGuiID dockspaceId = ImGui::GetID("LiteInfoSections");
		static bool retired = false;
		if (!retired)
		{
			RetireOldDockspaces();
			retired = true;
		}

		if (InfoDockNeedsBuild(dockspaceId))
			BuildInfoDock(dockspaceId, viewport->WorkSize);

		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::End();
	}

	void ImGuiLayer::OnImGuiRender()
	{
		LITE_PROFILE_SCOPE("ImGui Panels");
		if (!m_Ready || !m_ShowInfo)
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

		ImGui::Begin("Profile");
		const ImGuiIO& io = ImGui::GetIO();
		float cap = Time::GetFPS();
		if (BeginRows("##session"))
		{
			Row("FPS", std::format("{:.1f}", io.Framerate));
			Row("Elapsed", std::format("{:.1f} s", Time::GetElapsed()));
			Row("Cap", cap > 0.0f ? std::format("{:.0f}", cap) : "off");
			ImGui::EndTable();
		}

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		const std::vector<ProfileSample>& samples = Profiler::GetSamples();
		if (samples.empty())
		{
			ImGui::TextDisabled("Waiting for a completed frame");
		}
		else if (ImGui::BeginTable("##profile", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Scope", ImGuiTableColumnFlags_WidthStretch, 0.72f);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthStretch, 0.28f);
			ImGui::TableHeadersRow();

			for (const ProfileSample& sample : samples)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + static_cast<float>(sample.Depth) * 14.0f);
				ImGui::TextUnformatted(sample.Name);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(std::format("{:.3f} ms", sample.Milliseconds).c_str());
			}

			ImGui::EndTable();
		}
		ImGui::Spacing();
		ImGui::TextDisabled("Press I to hide");
		ImGui::End();

		ImGui::Begin("Draw");
		if (BeginRows("##draw"))
		{
			Row("Frame active", Renderer::IsFrameActive() ? "yes" : "no");
			Row("Draw calls", std::format("{}", Renderer::GetDrawCalls()));
			Row("Quads", std::format("{}", Renderer::GetQuadCount()));
			Row("Triangles", std::format("{}", Renderer::GetTriangleCount()));
			Row("Indices", std::format("{}", Renderer::GetIndexCount()));
			Row("Samples", "1");
			Row("Blend", "Premultiplied");
			ImGui::EndTable();
		}
		ImGui::End();

		ImGui::Begin("GPU");
		if (BeginRows("##gpu"))
		{
			Row("Name", properties.deviceName);
			Row("Type", deviceType);
			Row("Vendor", std::format("{:04X}", properties.vendorID));
			Row("Device", std::format("{:04X}", properties.deviceID));
			Row("Memory", DeviceMemory());
			Row("API", version(properties.apiVersion));
			Row("Driver", version(properties.driverVersion));
			Row("Max texture", std::format("{}", properties.limits.maxImageDimension2D));
			Row("Max framebuffer", std::format("{} x {}", properties.limits.maxFramebufferWidth, properties.limits.maxFramebufferHeight));
			Row("Max uniform", std::format("{}", properties.limits.maxUniformBufferRange));
			Row("Max anisotropy", std::format("{:.0f}", properties.limits.maxSamplerAnisotropy));
			Row("Max samplers", std::format("{}", properties.limits.maxPerStageDescriptorSamplers));
			ImGui::EndTable();
		}
		ImGui::End();

		ImGui::Begin("Swapchain");
		if (BeginRows("##swapchain"))
		{
			Row("Extent", std::format("{} x {}", extent.width, extent.height));
			Row("Format", FormatName(Renderer::GetSwapchainFormat()));
			Row("Color space", ColorSpaceName(Renderer::GetColorSpace()));
			Row("Present", PresentModeName(Renderer::GetPresentMode()));
			Row("Images", std::format("{}", Renderer::GetImageCount()));
			Row("Min images", std::format("{}", Renderer::GetMinImageCount()));
			Row("Image index", std::format("{}", Renderer::GetImageIndex()));
			Row("Frame index", std::format("{}", Renderer::GetFrameIndex()));
			Row("Frames in flight", std::format("{}", VulkanSync::FramesInFlight));
			Row("Queue family", std::format("{}", Renderer::GetGraphicsQueueFamily()));
			ImGui::EndTable();
		}
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
