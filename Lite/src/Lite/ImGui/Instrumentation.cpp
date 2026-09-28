#include <Lite/ImGui/Instrumentation.h>

#include <Lite/Core/Profile/Profiler.h>
#include <Lite/Core/Time.h>
#include <Lite/Renderer/Renderer.h>
#include <Lite/Renderer/Vulkan/VulkanSync.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace Lite {

	namespace {

		const ImVec4 kLabelColor { 0.62f, 0.68f, 0.76f, 1.0f };

		void BeginDocked(const char* beside, const char* name, bool place)
		{
			if (place)
			{
				ImGuiWindow* host = ImGui::FindWindowByName(beside);
				if (host != nullptr && host->DockId != 0)
					ImGui::SetNextWindowDockID(host->DockId, ImGuiCond_Always);
			}

			ImGui::Begin(name);
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

		std::string VersionText(uint32_t value)
		{
			return std::format("{}.{}.{}", VK_VERSION_MAJOR(value), VK_VERSION_MINOR(value), VK_VERSION_PATCH(value));
		}

	}

	void DrawInstrumentation(bool place)
	{
		LITE_PROFILE_SCOPE("ImGui Panels");

		VkPhysicalDeviceProperties properties {};
		vkGetPhysicalDeviceProperties(Renderer::GetPhysicalDevice(), &properties);
		VkExtent2D extent = Renderer::GetExtent();

		const char* deviceType = "Other";
		switch (properties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: deviceType = "Discrete GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: deviceType = "Integrated GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: deviceType = "Virtual GPU"; break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU: deviceType = "CPU"; break;
			default: break;
		}

		BeginDocked("Console", "Profile", place);
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

		BeginDocked("Inspector", "Draw", place);
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

		BeginDocked("Scene", "GPU", place);
		if (BeginRows("##gpu"))
		{
			Row("Name", properties.deviceName);
			Row("Type", deviceType);
			Row("Vendor", std::format("{:04X}", properties.vendorID));
			Row("Device", std::format("{:04X}", properties.deviceID));
			Row("Memory", DeviceMemory());
			Row("API", VersionText(properties.apiVersion));
			Row("Driver", VersionText(properties.driverVersion));
			Row("Max texture", std::format("{}", properties.limits.maxImageDimension2D));
			Row("Max framebuffer", std::format("{} x {}", properties.limits.maxFramebufferWidth, properties.limits.maxFramebufferHeight));
			Row("Max uniform", std::format("{}", properties.limits.maxUniformBufferRange));
			Row("Max anisotropy", std::format("{:.0f}", properties.limits.maxSamplerAnisotropy));
			Row("Max samplers", std::format("{}", properties.limits.maxPerStageDescriptorSamplers));
			ImGui::EndTable();
		}
		ImGui::End();

		BeginDocked("Scene", "Swapchain", place);
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

}
