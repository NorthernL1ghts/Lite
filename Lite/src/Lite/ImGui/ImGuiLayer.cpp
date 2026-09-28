#include "ImGuiLayer.h"

#include "Lite/Core/Events/Event.h"
#include "Lite/Core/Logger.h"
#include "Lite/Core/Time.h"
#include "Lite/Renderer/Renderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <format>
#include <string_view>

namespace Lite {

	namespace {

		const ImVec4 kHeaderColor { 0.73f, 0.86f, 1.0f, 1.0f };
		const ImVec4 kLabelColor { 0.58f, 0.61f, 0.66f, 1.0f };

		void PanelHeader(const char* title)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, kHeaderColor);
			ImGui::TextUnformatted(title);
			ImGui::PopStyleColor();
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.0f, 4.0f));
		}

		void Property(const char* label, std::string_view value)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, kLabelColor);
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextUnformatted(value.data(), value.data() + value.size());
			ImGui::PopTextWrapPos();
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
		}

		void Stat(const char* label, std::string_view value)
		{
			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Text, kLabelColor);
			ImGui::TextUnformatted(label);
			ImGui::PopStyleColor();
			ImGui::TextUnformatted(value.data(), value.data() + value.size());
			ImGui::EndGroup();
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

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowRounding = 0.0f;
		style.ChildRounding = 4.0f;
		style.FrameRounding = 3.0f;
		style.GrabRounding = 3.0f;
		style.WindowPadding = ImVec2(14.0f, 12.0f);
		style.FramePadding = ImVec2(6.0f, 4.0f);
		style.ItemSpacing = ImVec2(8.0f, 4.0f);
		style.WindowBorderSize = 0.0f;
		style.DockingSeparatorSize = 1.0f;
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.32f, 0.38f, 1.0f);

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

		ImGuiID dockspaceId = ImGui::GetID("LiteEditorSides");
		ImGuiDockNodeFlags dockFlags = static_cast<ImGuiDockNodeFlags>(
			static_cast<int>(ImGuiDockNodeFlags_PassthruCentralNode) | static_cast<int>(ImGuiDockNodeFlags_AutoHideTabBar));
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockFlags);

		ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceId);
		if (node == nullptr || !node->IsSplitNode())
		{
			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGuiDockNodeFlags nodeFlags = static_cast<ImGuiDockNodeFlags>(
				static_cast<int>(dockFlags) | static_cast<int>(ImGuiDockNodeFlags_DockSpace));
			ImGui::DockBuilderAddNode(dockspaceId, nodeFlags);
			ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

			ImGuiID center = dockspaceId;
			ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.26f, nullptr, &center);
			ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
			ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.13f, nullptr, &center);
			ImGuiID leftBottom = ImGui::DockBuilderSplitNode(left, ImGuiDir_Down, 0.42f, nullptr, &left);

			ImGui::DockBuilderDockWindow("GPU", left);
			ImGui::DockBuilderDockWindow("Swapchain", leftBottom);
			ImGui::DockBuilderDockWindow("Draw", right);
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

		ImGui::Begin("GPU");
		PanelHeader("GPU");
		Property("Name", properties.deviceName);
		Property("Type", deviceType);
		Property("Vendor", std::format("{:04X}", properties.vendorID));
		Property("Device", std::format("{:04X}", properties.deviceID));
		Property("Memory", DeviceMemory());
		Property("API", version(properties.apiVersion));
		Property("Driver", version(properties.driverVersion));
		Property("Max texture", std::format("{}", properties.limits.maxImageDimension2D));
		Property("Max framebuffer", std::format("{} x {}", properties.limits.maxFramebufferWidth, properties.limits.maxFramebufferHeight));
		Property("Max uniform", std::format("{}", properties.limits.maxUniformBufferRange));
		Property("Max anisotropy", std::format("{:.0f}", properties.limits.maxSamplerAnisotropy));
		Property("Max samplers", std::format("{}", properties.limits.maxPerStageDescriptorSamplers));
		ImGui::End();

		ImGui::Begin("Swapchain");
		PanelHeader("Swapchain");
		Property("Extent", std::format("{} x {}", extent.width, extent.height));
		Property("Format", FormatName(Renderer::GetSwapchainFormat()));
		Property("Color space", ColorSpaceName(Renderer::GetColorSpace()));
		Property("Present", PresentModeName(Renderer::GetPresentMode()));
		Property("Images", std::format("{}", Renderer::GetImageCount()));
		Property("Min images", std::format("{}", Renderer::GetMinImageCount()));
		Property("Image index", std::format("{}", Renderer::GetImageIndex()));
		Property("Frame index", std::format("{}", Renderer::GetFrameIndex()));
		Property("Frames in flight", std::format("{}", VulkanSync::FramesInFlight));
		Property("Queue family", std::format("{}", Renderer::GetGraphicsQueueFamily()));
		ImGui::End();

		ImGui::Begin("Draw");
		PanelHeader("Draw");
		Property("Frame active", Renderer::IsFrameActive() ? "yes" : "no");
		Property("Draw calls", std::format("{}", Renderer::GetDrawCalls()));
		Property("Quads", std::format("{}", Renderer::GetQuadCount()));
		Property("Triangles", std::format("{}", Renderer::GetTriangleCount()));
		Property("Indices", std::format("{}", Renderer::GetIndexCount()));
		Property("Samples", "1");
		Property("Blend", "Premultiplied");
		ImGui::End();

		const ImGuiIO& io = ImGui::GetIO();
		float frameMs = io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f;
		float cap = Time::GetFPS();
		ImGui::Begin("Stats");
		PanelHeader("Stats");
		Stat("FPS", std::format("{:.1f}", io.Framerate));
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Frame", std::format("{:.2f} ms", frameMs));
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Delta", std::format("{:.2f} ms", Time::GetDeltaMilliseconds()));
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Elapsed", std::format("{:.1f} s", Time::GetElapsed()));
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Cap", cap > 0.0f ? std::format("{:.0f}", cap) : "off");
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Draws", std::format("{}", Renderer::GetDrawCalls()));
		ImGui::SameLine(0.0f, 22.0f);
		Stat("Quads", std::format("{}", Renderer::GetQuadCount()));
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
